#pragma once
#include "Game/Types.hpp"
#include "Game/Forms.hpp"
#include "Game/NVSEInterfaces.hpp"
#include <vector>
#include <string>

enum class CommandType { None, CommandName, Coc, GameSetting, ActorValue, Quest, QuestObjective, QuestStage, Perk, FormType, InventoryItem, EquippableItem, PlaceableForm, Note, Faction, Sound, ImageSpaceModifier, Weather, WorldSpace, Idle, Music, FormList, Spell, QuestVariable, Alias };

enum class BaseFormCategory { All, Inventory, Equippable, Placeable };

struct BaseFormEntry {
	std::string editorID;
	char type[4];
};

const char* CommandTypeToRecordType(CommandType type);

namespace FormCache {
	const std::vector<std::string>& Get(const char* type4);
}

namespace GameSettings {
	extern std::vector<std::string> g_List;
	void Build();
}

namespace ActorValues {
	extern std::vector<std::string> g_List;
	void Build();
}

namespace CommandNames {
	extern std::vector<std::string> g_List;
	void Build();
}

namespace FormTypes {
	extern std::vector<const char*> g_List;
	void Build();
}

namespace BaseForms {
	extern std::vector<BaseFormEntry> g_List;
	void Build();
	bool MatchesCategory(const char* type, BaseFormCategory category);
}

namespace QuestObjectives {
	extern std::vector<UInt32> g_List;
	extern std::string g_LastQuestID;
	void Build(const char* questEditorID);
}

namespace QuestStages {
	extern std::vector<UInt32> g_List;
	extern std::string g_LastQuestID;
	void Build(const char* questEditorID);
}

namespace Autocomplete {
	extern std::string g_LastInput;
	extern CommandType g_LastType;

	void FindForms(const char* type4, const char* prefix);
	void FindSettings(const char* prefix);
	void FindActorValues(const char* prefix);
	void FindCommands(const char* prefix);
	void FindFormTypes(const char* prefix);
	void FindAliases(const char* prefix);
	void FindBaseForms(const char* prefix, BaseFormCategory category);
	void FindQuestVariables(const char* questEditorID, const char* prefix);
	void FindObjectives(const char* questID, const char* prefix);
	void FindStages(const char* questID, const char* prefix);

	const char* Current();
	const char* GetMatch(int i);
	int GetIndex();
	void Next();
	void Prev();
	int Count();
}

namespace Aliases {
	void Load(const char* iniPath);
	const char* Lookup(const char* name);
	extern std::vector<std::string> g_Names;
}

void SetCommandTable(NVSECommandTableInterface* table);
NVSECommandTableInterface* GetCommandTable();
const CommandInfo* GetCommandInfoByName(const char* name);
std::string FormatCommandParams(const CommandInfo* cmd);

Setting* LookupGameSetting(const char* name);
const char* FormatSettingValue(Setting* s, char* buf, size_t sz);

UInt32 GetActorValueCode(const char* name);
float GetActorValueForRef(void* ref, UInt32 avCode);
void* GetConsoleSelectedRef();
void* GetPlayerRef();
