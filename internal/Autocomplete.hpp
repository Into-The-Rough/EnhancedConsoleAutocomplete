#pragma once
#include "Game/Types.hpp"
#include "Game/Forms.hpp"
#include "Game/NVSEInterfaces.hpp"
#include <vector>
#include <string>

enum class CommandType { None, CommandName, Coc, GameSetting, ActorValue, Quest, QuestObjective, QuestStage, Perk, FormType, InventoryItem, EquippableItem, PlaceableForm, Note, Faction, Sound, ImageSpaceModifier, Weather, WorldSpace, Idle, Music, FormList, Spell, QuestVariable, Alias };

namespace Cells {
	extern std::vector<std::string> g_List;
	void Build(bool force = false);
	bool IsBuilt();
}

namespace GameSettings {
	extern std::vector<std::string> g_List;
	void Build();
}

namespace ActorValues {
	extern std::vector<std::string> g_List;
	void Build();
}

namespace Quests {
	extern std::vector<std::string> g_List;
	void Build();
}

namespace Notes {
	extern std::vector<std::string> g_List;
	void Build();
}

namespace Factions {
	extern std::vector<std::string> g_List;
	void Build();
}

namespace Sounds {
	extern std::vector<std::string> g_List;
	void Build();
}

namespace ImageSpaceModifiers {
	extern std::vector<std::string> g_List;
	void Build();
}

namespace Weathers {
	extern std::vector<std::string> g_List;
	void Build();
}

namespace WorldSpaces {
	extern std::vector<std::string> g_List;
	void Build();
}

namespace Idles {
	extern std::vector<std::string> g_List;
	void Build();
}

namespace MusicTypes {
	extern std::vector<std::string> g_List;
	void Build();
}

namespace FormLists {
	extern std::vector<std::string> g_List;
	void Build();
}

namespace Spells {
	extern std::vector<std::string> g_List;
	void Build();
}

namespace Perks {
	extern std::vector<std::string> g_List;
	void Build();
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

namespace CommandNames {
	extern std::vector<std::string> g_List;
	void Build();
}

namespace FormTypes {
	extern std::vector<const char*> g_List;
	void Build();
}

enum class BaseFormCategory { All, Inventory, Equippable, Placeable };

struct BaseFormEntry {
	std::string editorID;
	UInt8 typeID;
};

namespace BaseForms {
	extern std::vector<BaseFormEntry> g_List;
	void Build(bool force = false);
	bool IsBuilt();
}

namespace Autocomplete {
	extern std::string g_LastInput;
	extern CommandType g_LastType;

	void FindCells(const char* prefix);
	void FindSettings(const char* prefix);
	void FindActorValues(const char* prefix);
	void FindQuests(const char* prefix);
	void FindNotes(const char* prefix);
	void FindFactions(const char* prefix);
	void FindSounds(const char* prefix);
	void FindImageSpaceModifiers(const char* prefix);
	void FindWeathers(const char* prefix);
	void FindWorldSpaces(const char* prefix);
	void FindIdles(const char* prefix);
	void FindMusicTypes(const char* prefix);
	void FindFormLists(const char* prefix);
	void FindSpells(const char* prefix);
	void FindQuestVariables(const char* questEditorID, const char* prefix);
	void FindAliases(const char* prefix);
	void FindPerks(const char* prefix);
	void FindCommands(const char* prefix);
	void FindObjectives(const char* questID, const char* prefix);
	void FindStages(const char* questID, const char* prefix);
	void FindFormTypes(const char* prefix);
	void FindBaseForms(const char* prefix, BaseFormCategory category);

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
