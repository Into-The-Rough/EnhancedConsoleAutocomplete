#include "Autocomplete.hpp"
#include "Cache.hpp"
#include "Game/Forms.hpp"
#include <algorithm>
#include <unordered_map>

static constexpr UInt32 kActorValueMax = 76;

static auto** g_EditorIDMap = reinterpret_cast<NiTMapBase<const char*, TESForm*>**>(0x11C54C8);
static auto** g_DataHandler = reinterpret_cast<void**>(0x11C3F2C);
static auto** g_GameSettingCollection = reinterpret_cast<GameSettingCollection**>(0x11C8048);

static const _GetActorValueName GetActorValueName = (_GetActorValueName)0x66EAC0;

static NVSECommandTableInterface* g_CmdTable = nullptr;


static void CollectEditorIDs(UInt8 filterType, std::vector<std::string>& out) {
	auto* map = *g_EditorIDMap;
	if (!map) return;
	for (UInt32 b = 0; b < map->numBuckets; b++) {
		for (auto* e = map->buckets[b]; e; e = e->next) {
			if (e->data && e->data->typeID == filterType && e->key && *e->key)
				out.push_back(e->key);
		}
	}
}

static void CollectAllEditorIDs(bool (*excludeFunc)(UInt8), std::vector<BaseFormEntry>& out) {
	auto* map = *g_EditorIDMap;
	if (!map) return;
	for (UInt32 b = 0; b < map->numBuckets; b++) {
		for (auto* e = map->buckets[b]; e; e = e->next) {
			if (e->data && e->key && *e->key) {
				UInt8 type = e->data->typeID;
				if (!excludeFunc || !excludeFunc(type))
					out.push_back({ e->key, type });
			}
		}
	}
}

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
	for (UInt32 b = 0; b < map.numBuckets; b++) {
		for (auto* e = map.buckets[b]; e; e = e->next) {
			if (e->data && e->data->name && _stricmp(e->data->name, name) == 0)
				return e->data;
		}
	}
	return nullptr;
}

const char* FormatSettingValue(Setting* s, char* buf, size_t sz) {
	if (!s || !s->name) return nullptr;
	char prefix = s->name[0];
	if (prefix == 'f')
		snprintf(buf, sz, "%s = %.4f", s->name, s->data.f);
	else if (prefix == 'i' || prefix == 'b' || prefix == 'u')
		snprintf(buf, sz, "%s = %d", s->name, s->data.i);
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
	auto* map = *g_EditorIDMap;
	if (!map || !editorID) return nullptr;
	for (UInt32 b = 0; b < map->numBuckets; b++) {
		for (auto* e = map->buckets[b]; e; e = e->next) {
			if (e->key && _stricmp(e->key, editorID) == 0)
				return e->data;
		}
	}
	return nullptr;
}

static void BuildRefToEditorIDMap(std::unordered_map<UInt32, const char*>& out) {
	auto* map = *g_EditorIDMap;
	if (!map) return;
	for (UInt32 b = 0; b < map->numBuckets; b++) {
		for (auto* e = map->buckets[b]; e; e = e->next) {
			if (e->data && e->key && *e->key)
				out[e->data->refID] = e->key;
		}
	}
}

static void CollectPerks(void* dh, const std::unordered_map<UInt32, const char*>& refMap, std::vector<std::string>& out) {
	ListNode<void>* node = (ListNode<void>*)((UInt8*)dh + 0x178);
	while (node) {
		if (node->data) {
			TESForm* form = (TESForm*)node->data;
			auto it = refMap.find(form->refID);
			if (it != refMap.end())
				out.push_back(it->second);
		}
		node = node->next;
	}
}

void SetCommandTable(NVSECommandTableInterface* table) {
	g_CmdTable = table;
}

NVSECommandTableInterface* GetCommandTable() {
	return g_CmdTable;
}

namespace Cells {
	std::vector<std::string> g_List;
	static bool g_Built = false;

	bool IsBuilt() { return g_Built; }

	void Build(bool force) {
		if (g_Built && !force) return;
		if (force) g_List.clear();

		std::vector<std::string> temp;
		Cache::BuildCellList(temp);
		CollectEditorIDs(kFormType_Cell, temp);

		std::sort(temp.begin(), temp.end(), [](const std::string& a, const std::string& b) {
			return _stricmp(a.c_str(), b.c_str()) < 0;
		});
		temp.erase(std::unique(temp.begin(), temp.end(), [](const std::string& a, const std::string& b) {
			return _stricmp(a.c_str(), b.c_str()) == 0;
		}), temp.end());

		g_List = std::move(temp);
		g_Built = true;
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

	//GetByName doesn't check shortName
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

		if (isOptional) {
			result += " (" + cleanType + ")";
		} else {
			result += " <" + cleanType + ">";
		}
	}

	return result;
}

namespace CommandNames {
	std::vector<std::string> g_List;
	static bool g_Built = false;

	void Build() {
		if (g_Built) return;
		if (!g_CmdTable) return;

		std::unordered_map<std::string, bool> seen;
		for (auto* cmd = g_CmdTable->Start(); cmd < g_CmdTable->End(); cmd++) {
			for (const char* name : { cmd->longName, cmd->shortName }) {
				if (!name || !*name) continue;
				std::string lower = name;
				std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
				if (seen.find(lower) == seen.end()) {
					seen[lower] = true;
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
	std::vector<const char*> g_List;
	static bool g_Built = false;

	static const char* g_AllFormTypes[] = {
		"ACHR", "ACRE", "ACTI", "ADDN", "ALCH", "ALOC", "AMEF", "AMMO", "ANIO", "ARMA",
		"ARMO", "ASPC", "AVIF", "BOOK", "BPTD", "CAMS", "CCRD", "CDCK", "CELL", "CHAL",
		"CHIP", "CLAS", "CLMT", "CLOT", "CMNY", "COBJ", "CONT", "CPTH", "CREA", "CSNO",
		"CSTY", "DEBR", "DEHY", "DIAL", "DOBJ", "DOOR", "ECZN", "EFSH", "ENCH", "EXPL",
		"EYES", "FACT", "FLOR", "FLST", "FURN", "GLOB", "GMST", "GRAS", "GRUP", "HAIR",
		"HDPT", "HUNG", "IDLE", "IDLM", "IMAD", "IMGS", "IMOD", "INFO", "INGR", "IPCT",
		"IPDS", "KEYM", "LAND", "LGTM", "LIGH", "LSCR", "LSCT", "LTEX", "LVLC", "LVLI",
		"LVLN", "LVSP", "MESG", "MGEF", "MICN", "MISC", "MSET", "MSTT", "MUSC", "NAVI",
		"NAVM", "NONE", "NOTE", "PACK", "PBEA", "PCBE", "PERK", "PFLA", "PGRE", "PMIS",
		"PROJ", "PWAT", "QUST", "RACE", "RADS", "RCCT", "RCPE", "REFR", "REGN", "REPU",
		"RGDL", "SCOL", "SCPT", "SKIL", "SLPD", "SOUN", "SPEL", "STAT", "TACT", "TERM",
		"TES4", "TLOD", "TOFT", "TREE", "TXST", "VTYP", "WATR", "WEAP", "WRLD", "WTHR",
		nullptr
	};

	void Build() {
		if (g_Built) return;

		for (const char** p = g_AllFormTypes; *p; ++p)
			g_List.push_back(*p);

		g_Built = true;
	}
}

namespace BaseForms {
	std::vector<BaseFormEntry> g_List;
	static bool g_Built = false;

	enum FormTypeID : UInt8 {
		kType_ACTI = 21,
		kType_TACT = 22,
		kType_TERM = 23,
		kType_ARMO = 24,
		kType_BOOK = 25,
		kType_CONT = 27,
		kType_DOOR = 28,
		kType_LIGH = 30,
		kType_MISC = 31,
		kType_STAT = 32,
		kType_MSTT = 34,
		kType_GRAS = 36,
		kType_TREE = 37,
		kType_FLOR = 38,
		kType_FURN = 39,
		kType_WEAP = 40,
		kType_AMMO = 41,
		kType_NPC_ = 43,
		kType_CREA = 44,
		kType_KEYM = 46,
		kType_ALCH = 47,
		kType_NOTE = 49,
		kType_PROJ = 51,
		kType_LVLI = 52,
		kType_LVLC = 68,
		kType_LVLN = 69,
		kType_IMOD = 103,
		kType_CHIP = 116,
		kType_CMNY = 117,
		kType_CCRD = 118,
	};

	bool IsBuilt() { return g_Built; }

	static bool IsExcludedType(UInt8 type) {
		if (type == kFormType_REFR || type == kFormType_ACHR || type == kFormType_ACRE)
			return true;
		if (type == kFormType_Cell)
			return true;
		if (type == kFormType_GMST || type == kFormType_GLOB)
			return true;
		if (type == kFormType_DIAL || type == kFormType_INFO)
			return true;
		if (type == kFormType_LAND || type == kFormType_NAVM || type == kFormType_NAVI)
			return true;
		if (type == kFormType_Quest || type == kFormType_BGSPerk)
			return true;
		return false;
	}

	static bool IsInventoryType(UInt8 type) {
		return type == kType_WEAP || type == kType_ARMO || type == kType_AMMO ||
		       type == kType_ALCH || type == kType_MISC || type == kType_NOTE ||
		       type == kType_KEYM || type == kType_BOOK || type == kType_IMOD ||
		       type == kType_CHIP || type == kType_CMNY || type == kType_CCRD;
	}

	static bool IsEquippableType(UInt8 type) {
		return type == kType_WEAP || type == kType_ARMO;
	}

	static bool IsPlaceableType(UInt8 type) {
		return type == kType_NPC_ || type == kType_CREA || type == kType_WEAP ||
		       type == kType_ARMO || type == kType_AMMO || type == kType_ALCH ||
		       type == kType_MISC || type == kType_CONT || type == kType_ACTI ||
		       type == kType_FURN || type == kType_STAT || type == kType_MSTT ||
		       type == kType_DOOR || type == kType_LIGH || type == kType_FLOR ||
		       type == kType_TREE || type == kType_NOTE || type == kType_KEYM ||
		       type == kType_BOOK || type == kType_TACT || type == kType_TERM ||
		       type == kType_PROJ || type == kType_LVLI || type == kType_LVLC ||
		       type == kType_LVLN;
	}

	bool MatchesCategory(UInt8 type, BaseFormCategory category) {
		switch (category) {
			case BaseFormCategory::Inventory:  return IsInventoryType(type);
			case BaseFormCategory::Equippable: return IsEquippableType(type);
			case BaseFormCategory::Placeable:  return IsPlaceableType(type);
			case BaseFormCategory::All:
			default:                           return true;
		}
	}

	void Build(bool force) {
		if (g_Built && !force) return;
		if (force) g_List.clear();

		std::vector<BaseFormEntry> temp;
		CollectAllEditorIDs(IsExcludedType, temp);

		std::sort(temp.begin(), temp.end(), [](const BaseFormEntry& a, const BaseFormEntry& b) {
			return _stricmp(a.editorID.c_str(), b.editorID.c_str()) < 0;
		});

		g_List = std::move(temp);
		g_Built = true;
	}
}

namespace Quests {
	std::vector<std::string> g_List;
	static bool g_Built = false;

	void Build() {
		if (g_Built) return;

		CollectEditorIDs(kFormType_Quest, g_List);

		std::sort(g_List.begin(), g_List.end(), [](const std::string& a, const std::string& b) {
			return _stricmp(a.c_str(), b.c_str()) < 0;
		});

		g_Built = true;
	}
}

namespace Notes {
	std::vector<std::string> g_List;
	static bool g_Built = false;

	void Build() {
		if (g_Built) return;

		CollectEditorIDs(49, g_List); //kFormType_Note

		std::sort(g_List.begin(), g_List.end(), [](const std::string& a, const std::string& b) {
			return _stricmp(a.c_str(), b.c_str()) < 0;
		});

		g_Built = true;
	}
}

namespace Factions {
	std::vector<std::string> g_List;
	static bool g_Built = false;

	void Build() {
		if (g_Built) return;

		CollectEditorIDs(8, g_List); //kFormType_Faction

		std::sort(g_List.begin(), g_List.end(), [](const std::string& a, const std::string& b) {
			return _stricmp(a.c_str(), b.c_str()) < 0;
		});

		g_Built = true;
	}
}

namespace Sounds {
	std::vector<std::string> g_List;
	static bool g_Built = false;

	void Build() {
		if (g_Built) return;

		CollectEditorIDs(13, g_List); //kFormType_Sound

		std::sort(g_List.begin(), g_List.end(), [](const std::string& a, const std::string& b) {
			return _stricmp(a.c_str(), b.c_str()) < 0;
		});

		g_Built = true;
	}
}

namespace ImageSpaceModifiers {
	std::vector<std::string> g_List;
	static bool g_Built = false;

	void Build() {
		if (g_Built) return;

		CollectEditorIDs(84, g_List); //kFormType_ImageSpaceModifier

		std::sort(g_List.begin(), g_List.end(), [](const std::string& a, const std::string& b) {
			return _stricmp(a.c_str(), b.c_str()) < 0;
		});

		g_Built = true;
	}
}

namespace Weathers {
	std::vector<std::string> g_List;
	static bool g_Built = false;

	void Build() {
		if (g_Built) return;

		CollectEditorIDs(0x35, g_List); //kFormType_TESWeather

		std::sort(g_List.begin(), g_List.end(), [](const std::string& a, const std::string& b) {
			return _stricmp(a.c_str(), b.c_str()) < 0;
		});

		g_Built = true;
	}
}

namespace WorldSpaces {
	std::vector<std::string> g_List;
	static bool g_Built = false;

	void Build() {
		if (g_Built) return;

		CollectEditorIDs(0x41, g_List); //kFormType_TESWorldSpace

		std::sort(g_List.begin(), g_List.end(), [](const std::string& a, const std::string& b) {
			return _stricmp(a.c_str(), b.c_str()) < 0;
		});

		g_Built = true;
	}
}

namespace Idles {
	std::vector<std::string> g_List;
	static bool g_Built = false;

	void Build() {
		if (g_Built) return;

		CollectEditorIDs(0x48, g_List); //kFormType_TESIdleForm

		std::sort(g_List.begin(), g_List.end(), [](const std::string& a, const std::string& b) {
			return _stricmp(a.c_str(), b.c_str()) < 0;
		});

		g_Built = true;
	}
}

namespace MusicTypes {
	std::vector<std::string> g_List;
	static bool g_Built = false;

	void Build() {
		if (g_Built) return;

		CollectEditorIDs(0x66, g_List); //kFormType_BGSMusicType

		std::sort(g_List.begin(), g_List.end(), [](const std::string& a, const std::string& b) {
			return _stricmp(a.c_str(), b.c_str()) < 0;
		});

		g_Built = true;
	}
}

namespace FormLists {
	std::vector<std::string> g_List;
	static bool g_Built = false;

	void Build() {
		if (g_Built) return;

		CollectEditorIDs(0x55, g_List); //kFormType_BGSListForm

		std::sort(g_List.begin(), g_List.end(), [](const std::string& a, const std::string& b) {
			return _stricmp(a.c_str(), b.c_str()) < 0;
		});

		g_Built = true;
	}
}

namespace Spells {
	std::vector<std::string> g_List;
	static bool g_Built = false;

	void Build() {
		if (g_Built) return;

		CollectEditorIDs(0x14, g_List); //kFormType_SpellItem

		std::sort(g_List.begin(), g_List.end(), [](const std::string& a, const std::string& b) {
			return _stricmp(a.c_str(), b.c_str()) < 0;
		});

		g_Built = true;
	}
}

namespace Perks {
	std::vector<std::string> g_List;
	static bool g_Built = false;

	void Build() {
		if (g_Built) return;

		void* dh = *g_DataHandler;
		if (!dh) return;

		std::unordered_map<UInt32, const char*> refToEditorID;
		BuildRefToEditorIDMap(refToEditorID);
		CollectPerks(dh, refToEditorID, g_List);

		std::sort(g_List.begin(), g_List.end(), [](const std::string& a, const std::string& b) {
			return _stricmp(a.c_str(), b.c_str()) < 0;
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

static void CollectQuestVars(const char* questEditorID, std::vector<std::string>& out) {
	TESForm* form = LookupFormByEditorID(questEditorID);
	if (!form || form->typeID != kFormType_Quest) return;

	//quest+0x1C = Script* (TESScriptableForm::pScript)
	UInt8* script = *(UInt8**)((UInt8*)form + 0x1C);
	if (!script) return;

	//script+0x4C = BSSimpleList<ScriptVariable*> (first node inline)
	struct VarNode { void* data; VarNode* next; };
	VarNode* node = (VarNode*)(script + 0x4C);

	while (node) {
		if (node->data) {
			//ScriptVariable+0x18 = BSString (char* at +0x00)
			char* varName = *(char**)((UInt8*)node->data + 0x18);
			if (varName && *varName)
				out.push_back(varName);
		}
		node = node->next;
	}
}

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

	void FindCells(const char* prefix) {
		FindInList(Cells::g_List, prefix, [](const std::string& s) { return s.c_str(); });
	}

	void FindSettings(const char* prefix) {
		FindInList(GameSettings::g_List, prefix, [](const std::string& s) { return s.c_str(); });
	}

	void FindActorValues(const char* prefix) {
		FindInList(ActorValues::g_List, prefix, [](const std::string& s) { return s.c_str(); });
	}

	void FindQuests(const char* prefix) {
		FindInList(Quests::g_List, prefix, [](const std::string& s) { return s.c_str(); });
	}

	void FindNotes(const char* prefix) {
		FindInList(Notes::g_List, prefix, [](const std::string& s) { return s.c_str(); });
	}

	void FindFactions(const char* prefix) {
		FindInList(Factions::g_List, prefix, [](const std::string& s) { return s.c_str(); });
	}

	void FindSounds(const char* prefix) {
		FindInList(Sounds::g_List, prefix, [](const std::string& s) { return s.c_str(); });
	}

	void FindImageSpaceModifiers(const char* prefix) {
		FindInList(ImageSpaceModifiers::g_List, prefix, [](const std::string& s) { return s.c_str(); });
	}

	void FindWeathers(const char* prefix) {
		FindInList(Weathers::g_List, prefix, [](const std::string& s) { return s.c_str(); });
	}

	void FindWorldSpaces(const char* prefix) {
		FindInList(WorldSpaces::g_List, prefix, [](const std::string& s) { return s.c_str(); });
	}

	void FindIdles(const char* prefix) {
		FindInList(Idles::g_List, prefix, [](const std::string& s) { return s.c_str(); });
	}

	void FindMusicTypes(const char* prefix) {
		FindInList(MusicTypes::g_List, prefix, [](const std::string& s) { return s.c_str(); });
	}

	void FindFormLists(const char* prefix) {
		FindInList(FormLists::g_List, prefix, [](const std::string& s) { return s.c_str(); });
	}

	void FindSpells(const char* prefix) {
		FindInList(Spells::g_List, prefix, [](const std::string& s) { return s.c_str(); });
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

	void FindAliases(const char* prefix) {
		FindInList(Aliases::g_Names, prefix, [](const std::string& s) { return s.c_str(); });
	}

	void FindPerks(const char* prefix) {
		FindInList(Perks::g_List, prefix, [](const std::string& s) { return s.c_str(); });
	}

	void FindCommands(const char* prefix) {
		FindInList(CommandNames::g_List, prefix, [](const std::string& s) { return s.c_str(); });
	}

	void FindFormTypes(const char* prefix) {
		FindInList(FormTypes::g_List, prefix, [](const char* s) { return s; });
	}

	void FindBaseForms(const char* prefix, BaseFormCategory category) {
		g_Matches.clear();
		g_MatchIndex = -1;
		if (!prefix) prefix = "";

		std::string lower = prefix;
		std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

		std::vector<std::string> prefixMatches, substringMatches;
		for (const auto& entry : BaseForms::g_List) {
			if (!BaseForms::MatchesCategory(entry.typeID, category))
				continue;

			const char* s = entry.editorID.c_str();
			if (!*s) continue;

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
