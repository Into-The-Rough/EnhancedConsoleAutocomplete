#include "Autocomplete.hpp"
#include "GameData.hpp"
#include "Cache.hpp"
#include <algorithm>
#include <unordered_set>

namespace Autocomplete {
	static std::vector<std::string> g_Matches;
	std::string g_LastInput;
	static int g_MatchIndex = -1;

	//move existing match to front, or insert if not present
	static void BoostOrInsert(const char* name) {
		for (size_t i = 0; i < g_Matches.size(); i++) {
			if (_stricmp(g_Matches[i].c_str(), name) == 0) {
				if (i > 0) {
					std::string tmp = std::move(g_Matches[i]);
					g_Matches.erase(g_Matches.begin() + i);
					g_Matches.insert(g_Matches.begin(), std::move(tmp));
				}
				g_MatchIndex = 0;
				return;
			}
		}
		g_Matches.insert(g_Matches.begin(), name);
		g_MatchIndex = 0;
	}
	CommandType g_LastType = CommandType::None;

	template<typename Container, typename GetStr>
	void FindInList(const Container& list, const char* prefix, GetStr getStr) {
		g_Matches.clear();
		g_MatchIndex = -1;
		if (!prefix) return;

		std::string lower = prefix;
		std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

		std::vector<std::string> prefixMatches, substringMatches;
		for (const auto& item : list) {
			const char* s = getStr(item);
			if (!s) continue;

			if (lower.empty()) {
				prefixMatches.push_back(s);
				continue;
			}

			std::string nameLower = s;
			std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);

			size_t pos = nameLower.find(lower);
			if (pos == 0) prefixMatches.push_back(s);
			else if (pos != std::string::npos) substringMatches.push_back(s);
		}

		g_Matches = std::move(prefixMatches);
		g_Matches.insert(g_Matches.end(), substringMatches.begin(), substringMatches.end());
		if (!g_Matches.empty()) g_MatchIndex = 0;
	}

	void FindForms(const char* type4, const char* prefix) {
		FindInList(FormCache::Get(type4), prefix, [](const std::string& s) { return s.c_str(); });
	}

	void FindSettings(const char* prefix) {
		FindInList(GameSettings::g_List, prefix, [](const std::string& s) { return s.c_str(); });
	}

	void FindActorValues(const char* prefix) {
		FindInList(ActorValues::g_List, prefix, [](const std::string& s) { return s.c_str(); });
	}

	static const std::unordered_set<std::string>& GetRefCommandNames() {
		static std::unordered_set<std::string> s;
		if (s.empty()) {
			const char* names[] = {
				"setav","setactorvalue","getav","getactorvalue","modav","modactorvalue",
				"forceav","forceactorvalue","getavinfo","addperk","removeperk","hasperk",
				"addnote","addnotens","addfaction","removefaction","setfactionrank",
				"getfactionrank","modfactionrank","getfactionreaction","setfactionreaction",
				"setally","setenemy","playsound","playsound3d","imod","rimod",
				"additem","removeitem","drop","equipitem","unequipitem",
				"placeatme","placeleveled","playidle","forceplayidle",
				"isinlist","listaddreference","listaddref",
				"addspell","addspellns","removespell","cast","castimmediate",
				"castimmediateonself","dispel","evaluatespellconditions",
				"getspellusagenum","isspelltarget","isspelltargetalt","isspelltargetlist"
			};
			for (auto n : names) s.insert(n);
		}
		return s;
	}

	static bool IsRefCommand(const std::string& name) {
		std::string lower = name;
		std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
		return GetRefCommandNames().count(lower) > 0;
	}

	void FindCommands(const char* prefix, bool hasRef) {
		g_Matches.clear();
		g_MatchIndex = -1;
		if (!prefix) return;

		std::string lower = prefix;
		std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

		std::vector<std::string> prefixRef, prefixOther, subRef, subOther;
		for (const auto& item : CommandNames::g_List) {
			const char* s = item.c_str();
			std::string nameLower = s;
			std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);

			bool isPrefix = false, isSub = false;
			if (lower.empty()) {
				isPrefix = true;
			} else {
				size_t pos = nameLower.find(lower);
				if (pos == 0) isPrefix = true;
				else if (pos != std::string::npos) isSub = true;
			}

			if (isPrefix) {
				if (IsRefCommand(item)) prefixRef.push_back(s);
				else prefixOther.push_back(s);
			} else if (isSub) {
				if (IsRefCommand(item)) subRef.push_back(s);
				else subOther.push_back(s);
			}
		}

		if (hasRef) {
			g_Matches = std::move(prefixRef);
			g_Matches.insert(g_Matches.end(), prefixOther.begin(), prefixOther.end());
			g_Matches.insert(g_Matches.end(), subRef.begin(), subRef.end());
			g_Matches.insert(g_Matches.end(), subOther.begin(), subOther.end());
		} else {
			g_Matches = std::move(prefixOther);
			g_Matches.insert(g_Matches.end(), prefixRef.begin(), prefixRef.end());
			g_Matches.insert(g_Matches.end(), subOther.begin(), subOther.end());
			g_Matches.insert(g_Matches.end(), subRef.begin(), subRef.end());
		}
		if (!g_Matches.empty()) g_MatchIndex = 0;

		//boost common commands when they match
		if (!hasRef && _strnicmp("qqq", prefix, strlen(prefix)) == 0)
			BoostOrInsert("QQQ");
		if (!hasRef && _strnicmp("player", prefix, strlen(prefix)) == 0)
			BoostOrInsert("player");
	}

	void FindFormTypes(const char* prefix) {
		FindInList(FormTypes::g_List, prefix, [](const std::string& s) { return s.c_str(); });
	}

	void FindAliases(const char* prefix) {
		FindInList(Aliases::g_Names, prefix, [](const std::string& s) { return s.c_str(); });
	}

	void FindBaseForms(const char* prefix, BaseFormCategory category) {
		std::vector<const char*> filtered;
		for (const auto& entry : BaseForms::g_List) {
			if (BaseForms::MatchesCategory(entry.type, category) && !entry.editorID.empty())
				filtered.push_back(entry.editorID.c_str());
		}
		FindInList(filtered, prefix, [](const char* s) { return s; });
	}

	void FindQuestVariables(const char* questEditorID, const char* prefix) {
		g_Matches.clear();
		g_MatchIndex = -1;
		if (!questEditorID || !*questEditorID) return;

		std::vector<std::string> varNames;
		CollectQuestVars(questEditorID, varNames);
		if (varNames.empty()) return;

		std::sort(varNames.begin(), varNames.end(), [](const std::string& a, const std::string& b) {
			return _stricmp(a.c_str(), b.c_str()) < 0;
		});

		FindInList(varNames, prefix, [](const std::string& s) { return s.c_str(); });
	}

	void FindObjectives(const char* questID, const char* prefix) {
		g_Matches.clear();
		g_MatchIndex = -1;

		if (_stricmp(questID, QuestObjectives::g_LastQuestID.c_str()) != 0)
			QuestObjectives::Build(questID);

		std::string prefixLower = prefix ? prefix : "";
		std::transform(prefixLower.begin(), prefixLower.end(), prefixLower.begin(), ::tolower);

		for (UInt32 id : QuestObjectives::g_List) {
			char buf[16];
			snprintf(buf, sizeof(buf), "%u", id);
			if (prefixLower.empty() || strncmp(buf, prefixLower.c_str(), prefixLower.length()) == 0)
				g_Matches.push_back(buf);
		}

		if (!g_Matches.empty()) g_MatchIndex = 0;
	}

	void FindStages(const char* questID, const char* prefix) {
		g_Matches.clear();
		g_MatchIndex = -1;

		if (_stricmp(questID, QuestStages::g_LastQuestID.c_str()) != 0)
			QuestStages::Build(questID);

		std::string prefixLower = prefix ? prefix : "";
		std::transform(prefixLower.begin(), prefixLower.end(), prefixLower.begin(), ::tolower);

		for (UInt32 id : QuestStages::g_List) {
			char buf[16];
			snprintf(buf, sizeof(buf), "%u", id);
			if (prefixLower.empty() || strncmp(buf, prefixLower.c_str(), prefixLower.length()) == 0)
				g_Matches.push_back(buf);
		}

		if (!g_Matches.empty()) g_MatchIndex = 0;
	}

	const char* Current() {
		return (g_MatchIndex >= 0 && g_MatchIndex < (int)g_Matches.size()) ? g_Matches[g_MatchIndex].c_str() : nullptr;
	}

	const char* GetMatch(int i) {
		return (i >= 0 && i < (int)g_Matches.size()) ? g_Matches[i].c_str() : nullptr;
	}

	int GetIndex() { return g_MatchIndex; }

	void Next() {
		if (!g_Matches.empty())
			g_MatchIndex = (g_MatchIndex + 1) % g_Matches.size();
	}

	void Prev() {
		if (!g_Matches.empty())
			g_MatchIndex = (g_MatchIndex - 1 + g_Matches.size()) % g_Matches.size();
	}

	int Count() { return (int)g_Matches.size(); }
}
