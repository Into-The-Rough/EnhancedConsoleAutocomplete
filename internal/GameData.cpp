#include "GameData.hpp"
#include "Cache.hpp"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <cctype>

static constexpr UInt32 kActorValueMax = 76;

static auto** g_GameSettingCollection = reinterpret_cast<GameSettingCollection**>(0x11C8048);

static const _GetActorValueName GetActorValueName = (_GetActorValueName)0x66EAC0;

static NVSECommandTableInterface* g_CmdTable = nullptr;

static void CollectSettings(std::vector<std::string>& out) {
	auto* coll = *g_GameSettingCollection;
	if (!coll) return;
	auto& map = coll->settingMap;
	for (UInt32 b = 0; b < map.numBuckets; b++) {
		for (auto* e = map.buckets[b]; e; e = e->next) {
			if (e->data && e->data->name && *e->data->name)
				out.push_back(e->data->name);
		}
	}
}

Setting* LookupGameSetting(const char* name) {
	auto* coll = *g_GameSettingCollection;
	if (!coll || !name || !*name) return nullptr;
	auto& map = coll->settingMap;
	UInt32 hash = 0;
	for (const char* p = name; *p; p++)
		hash = tolower((unsigned char)*p) + 33 * hash;
	UInt32 bucket = hash % map.numBuckets;
	for (auto* e = map.buckets[bucket]; e; e = e->next) {
		if (e->data && e->data->name && _stricmp(e->data->name, name) == 0)
			return e->data;
	}
	return nullptr;
}

const char* FormatSettingValue(Setting* s, char* buf, size_t sz) {
	if (!s || !s->name) return nullptr;
	char prefix = s->name[0];
	if (prefix == 'f')
		snprintf(buf, sz, "%s = %.4f", s->name, s->data.f);
	else if (prefix == 'i' || prefix == 'b')
		snprintf(buf, sz, "%s = %d", s->name, s->data.i);
	else if (prefix == 'u')
		snprintf(buf, sz, "%s = %u", s->name, s->data.uint);
	else if (prefix == 's' && s->data.str)
		snprintf(buf, sz, "%s = \"%s\"", s->name, s->data.str);
	else
		return nullptr;
	return buf;
}

UInt32 GetActorValueCode(const char* name) {
	if (!name || !*name) return 0xFFFFFFFF;
	for (UInt32 i = 0; i <= kActorValueMax; i++) {
		char* avName = GetActorValueName(i);
		if (avName && _stricmp(avName, name) == 0)
			return i;
	}
	return 0xFFFFFFFF;
}

void* GetConsoleSelectedRef() {
	void* im = *(void**)0x11D8A80;
	if (!im) return nullptr;
	return *(void**)((UInt8*)im + 0xF0);
}

void* GetPlayerRef() {
	return *(void**)0x11DEA3C;
}

float GetActorValueForRef(void* ref, UInt32 avCode) {
	if (!ref) return 0;
	UInt8 typeID = ((TESForm*)ref)->typeID;
	if (typeID != kFormType_ACHR && typeID != kFormType_ACRE) return 0;
	void* avOwner = (void*)((UInt8*)ref + 0xA4);
	void** vtbl = *(void***)avOwner;
	if (!vtbl) return 0;
	typedef float (__thiscall *GetAV_t)(void*, UInt32);
	auto GetAV = (GetAV_t)vtbl[3];
	return GetAV(avOwner, avCode);
}

TESForm* LookupFormByEditorID(const char* editorID) {
	if (!editorID || !*editorID) return nullptr;
	return CdeclCall<TESForm*>(0x483A1B, editorID);
}

void SetCommandTable(NVSECommandTableInterface* table) {
	g_CmdTable = table;
}

NVSECommandTableInterface* GetCommandTable() {
	return g_CmdTable;
}

const char* CommandTypeToRecordType(CommandType type) {
	switch (type) {
		case CommandType::Coc: return "CELL";
		case CommandType::Quest: return "QUST";
		case CommandType::Perk: return "PERK";
		case CommandType::Note: return "NOTE";
		case CommandType::Faction: return "FACT";
		case CommandType::Sound: return "SOUN";
		case CommandType::ImageSpaceModifier: return "IMAD";
		case CommandType::Weather: return "WTHR";
		case CommandType::WorldSpace: return "WRLD";
		case CommandType::Idle: return "IDLE";
		case CommandType::Music: return "MUSC";
		case CommandType::FormList: return "FLST";
		case CommandType::Spell: return "SPEL";
		default: return nullptr;
	}
}

namespace FormCache {
	static std::unordered_map<std::string, std::vector<std::string>> g_ByType;

	const std::vector<std::string>& Get(const char* type4) {
		Cache::Build();
		std::string key(type4, 4);
		auto it = g_ByType.find(key);
		if (it != g_ByType.end()) return it->second;

		auto& list = g_ByType[key];
		for (const auto& f : Cache::GetAll()) {
			if (memcmp(f.type, type4, 4) == 0)
				list.push_back(f.editorID);
		}
		std::sort(list.begin(), list.end(), [](const std::string& a, const std::string& b) {
			return _stricmp(a.c_str(), b.c_str()) < 0;
		});
		list.erase(std::unique(list.begin(), list.end(), [](const std::string& a, const std::string& b) {
			return _stricmp(a.c_str(), b.c_str()) == 0;
		}), list.end());
		return list;
	}
}

namespace GameSettings {
	std::vector<std::string> g_List;
	static bool g_Built = false;

	void Build() {
		if (g_Built) return;
		CollectSettings(g_List);
		std::sort(g_List.begin(), g_List.end(), [](const std::string& a, const std::string& b) {
			return _stricmp(a.c_str(), b.c_str()) < 0;
		});
		g_Built = true;
	}
}

namespace ActorValues {
	std::vector<std::string> g_List;
	static bool g_Built = false;

	void Build() {
		if (g_Built) return;
		for (UInt32 i = 0; i <= kActorValueMax; i++) {
			char* name = GetActorValueName(i);
			if (name && *name)
				g_List.push_back(std::string(name));
		}
		std::sort(g_List.begin(), g_List.end(), [](const std::string& a, const std::string& b) {
			return _stricmp(a.c_str(), b.c_str()) < 0;
		});
		g_Built = true;
	}
}

const CommandInfo* GetCommandInfoByName(const char* name) {
	if (!name || !*name || !g_CmdTable) return nullptr;
	const CommandInfo* cmd = g_CmdTable->GetByName(name);
	if (cmd) return cmd;
	for (auto* c = g_CmdTable->Start(); c < g_CmdTable->End(); c++) {
		if (c->shortName && _stricmp(c->shortName, name) == 0)
			return c;
	}
	return nullptr;
}

static bool ReadParamInfo(const CommandInfo* cmd, int index, const char** outTypeStr, bool* outOptional) {
	if (!cmd->params) return false;
	ParamInfo* p = &cmd->params[index];
	if (!p->typeStr || !*p->typeStr) return false;
	*outTypeStr = p->typeStr;
	*outOptional = p->isOptional != 0;
	return true;
}

std::string FormatCommandParams(const CommandInfo* cmd) {
	if (!cmd || cmd->numParams == 0) return "";
	std::string result;
	for (int i = 0; i < cmd->numParams; i++) {
		const char* typeStr = nullptr;
		bool isOptional = false;
		if (!ReadParamInfo(cmd, i, &typeStr, &isOptional)) continue;
		std::string cleanType = typeStr;
		size_t optPos = cleanType.find(" (Optional)");
		if (optPos != std::string::npos) cleanType = cleanType.substr(0, optPos);
		cleanType.erase(std::remove(cleanType.begin(), cleanType.end(), ' '), cleanType.end());
		if (isOptional)
			result += " (" + cleanType + ")";
		else
			result += " <" + cleanType + ">";
	}
	return result;
}

namespace CommandNames {
	std::vector<std::string> g_List;
	static bool g_Built = false;

	void Build() {
		if (g_Built) return;
		if (!g_CmdTable) return;
		std::unordered_set<std::string> seen;
		for (auto* cmd = g_CmdTable->Start(); cmd < g_CmdTable->End(); cmd++) {
			for (const char* name : { cmd->longName, cmd->shortName }) {
				if (!name || !*name) continue;
				std::string lower = name;
				std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
				if (seen.find(lower) == seen.end()) {
					seen.insert(lower);
					g_List.push_back(name);
				}
			}
		}
		std::sort(g_List.begin(), g_List.end(), [](const std::string& a, const std::string& b) {
			return _stricmp(a.c_str(), b.c_str()) < 0;
		});
		g_Built = true;
	}
}

namespace FormTypes {
	std::vector<std::string> g_List;
	static bool g_Built = false;

	static auto* g_FormEnumString = (UInt8*)0x1187000;

	void Build() {
		if (g_Built) return;
		for (int i = 0; i < 121; i++) {
			UInt32 code = *(UInt32*)(g_FormEnumString + i * 0xC + 0x8);
			if (!code) continue;
			char buf[5];
			memcpy(buf, &code, 4);
			buf[4] = '\0';
			g_List.push_back(buf);
		}
		std::sort(g_List.begin(), g_List.end(), [](const std::string& a, const std::string& b) {
			return _stricmp(a.c_str(), b.c_str()) < 0;
		});
		g_Built = true;
	}
}

namespace BaseForms {
	std::vector<BaseFormEntry> g_List;
	static bool g_Built = false;

	static bool eq4(const char* a, const char* b) { return memcmp(a, b, 4) == 0; }

	static bool IsExcludedType(const char* t) {
		return eq4(t, "REFR") || eq4(t, "ACHR") || eq4(t, "ACRE") ||
		       eq4(t, "CELL") || eq4(t, "GMST") || eq4(t, "GLOB") ||
		       eq4(t, "DIAL") || eq4(t, "INFO") || eq4(t, "LAND") ||
		       eq4(t, "NAVM") || eq4(t, "NAVI") || eq4(t, "QUST") ||
		       eq4(t, "PERK") || eq4(t, "TES4") || eq4(t, "GRUP");
	}

	static bool IsInventoryType(const char* t) {
		return eq4(t, "WEAP") || eq4(t, "ARMO") || eq4(t, "AMMO") ||
		       eq4(t, "ALCH") || eq4(t, "MISC") || eq4(t, "NOTE") ||
		       eq4(t, "KEYM") || eq4(t, "BOOK") || eq4(t, "IMOD") ||
		       eq4(t, "CHIP") || eq4(t, "CMNY") || eq4(t, "CCRD");
	}

	static bool IsEquippableType(const char* t) {
		return eq4(t, "WEAP") || eq4(t, "ARMO");
	}

	static bool IsPlaceableType(const char* t) {
		return eq4(t, "NPC_") || eq4(t, "CREA") || eq4(t, "WEAP") ||
		       eq4(t, "ARMO") || eq4(t, "AMMO") || eq4(t, "ALCH") ||
		       eq4(t, "MISC") || eq4(t, "CONT") || eq4(t, "ACTI") ||
		       eq4(t, "FURN") || eq4(t, "STAT") || eq4(t, "MSTT") ||
		       eq4(t, "DOOR") || eq4(t, "LIGH") || eq4(t, "FLOR") ||
		       eq4(t, "TREE") || eq4(t, "NOTE") || eq4(t, "KEYM") ||
		       eq4(t, "BOOK") || eq4(t, "TACT") || eq4(t, "TERM") ||
		       eq4(t, "PROJ") || eq4(t, "LVLI") || eq4(t, "LVLC") ||
		       eq4(t, "LVLN");
	}

	bool MatchesCategory(const char* type, BaseFormCategory category) {
		switch (category) {
			case BaseFormCategory::Inventory:  return IsInventoryType(type);
			case BaseFormCategory::Equippable: return IsEquippableType(type);
			case BaseFormCategory::Placeable:  return IsPlaceableType(type);
			case BaseFormCategory::All:
			default:                           return true;
		}
	}

	void Build() {
		if (g_Built) return;
		Cache::Build();
		for (const auto& f : Cache::GetAll()) {
			if (!IsExcludedType(f.type))
				g_List.push_back({ f.editorID, { f.type[0], f.type[1], f.type[2], f.type[3] } });
		}
		std::sort(g_List.begin(), g_List.end(), [](const BaseFormEntry& a, const BaseFormEntry& b) {
			return _stricmp(a.editorID.c_str(), b.editorID.c_str()) < 0;
		});
		g_Built = true;
	}
}

namespace QuestObjectives {
	std::vector<UInt32> g_List;
	std::string g_LastQuestID;

	void Build(const char* questEditorID) {
		g_List.clear();
		g_LastQuestID.clear();
		if (!questEditorID) return;

		TESForm* form = LookupFormByEditorID(questEditorID);
		if (!form || form->typeID != kFormType_Quest) return;

		g_LastQuestID = questEditorID;
		TESQuest* quest = (TESQuest*)form;

		auto* node = &quest->lVarOrObjectives;
		while (node) {
			if (node->data) {
				auto* obj = (BGSQuestObjective*)node->data;
				UInt32 vtbl = *(UInt32*)obj;
				if (vtbl == 0x1047088)
					g_List.push_back(obj->objectiveId);
			}
			node = node->next;
		}
		std::sort(g_List.begin(), g_List.end());
	}
}

namespace QuestStages {
	std::vector<UInt32> g_List;
	std::string g_LastQuestID;

	void Build(const char* questEditorID) {
		g_List.clear();
		g_LastQuestID.clear();
		if (!questEditorID) return;

		TESForm* form = LookupFormByEditorID(questEditorID);
		if (!form || form->typeID != kFormType_Quest) return;

		g_LastQuestID = questEditorID;
		TESQuest* quest = (TESQuest*)form;

		auto* node = &quest->stages;
		while (node) {
			if (node->data)
				g_List.push_back(node->data->stage);
			node = node->next;
		}
		std::sort(g_List.begin(), g_List.end());
	}
}

namespace Aliases {
	static std::unordered_map<std::string, std::string> g_Map;
	std::vector<std::string> g_Names;

	void Load(const char* iniPath) {
		char buf[8192];
		DWORD len = GetPrivateProfileSectionA("Aliases", buf, sizeof(buf), iniPath);
		if (len == 0) return;

		const char* p = buf;
		while (*p) {
			const char* eq = strchr(p, '=');
			if (eq) {
				std::string name(p, eq - p);
				std::string cmd(eq + 1);
				std::string lower = name;
				for (auto& c : lower) c = (char)tolower(c);
				g_Map[lower] = cmd;
				g_Names.push_back(name);
			}
			p += strlen(p) + 1;
		}
		std::sort(g_Names.begin(), g_Names.end(), [](const std::string& a, const std::string& b) {
			return _stricmp(a.c_str(), b.c_str()) < 0;
		});
	}

	const char* Lookup(const char* name) {
		if (!name) return nullptr;
		std::string lower(name);
		for (auto& c : lower) c = (char)tolower(c);
		auto it = g_Map.find(lower);
		if (it != g_Map.end()) return it->second.c_str();
		return nullptr;
	}
}

void CollectQuestVars(const char* questEditorID, std::vector<std::string>& out) {
	TESForm* form = LookupFormByEditorID(questEditorID);
	if (!form || form->typeID != kFormType_Quest) return;

	UInt8* script = *(UInt8**)((UInt8*)form + 0x1C);
	if (!script) return;

	struct VarNode { void* data; VarNode* next; };
	VarNode* node = (VarNode*)(script + 0x4C);

	while (node) {
		if (node->data) {
			char* varName = *(char**)((UInt8*)node->data + 0x18);
			if (varName && *varName)
				out.push_back(varName);
		}
		node = node->next;
	}
}
