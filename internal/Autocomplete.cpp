#include "Autocomplete.hpp"
#include "GameData.hpp"
#include "Cache.hpp"
#include <algorithm>

namespace Autocomplete {
	static std::vector<std::string> g_Matches;
	std::string g_LastInput;
	static int g_MatchIndex = -1;
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

	void FindCommands(const char* prefix) {
		FindInList(CommandNames::g_List, prefix, [](const std::string& s) { return s.c_str(); });
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
