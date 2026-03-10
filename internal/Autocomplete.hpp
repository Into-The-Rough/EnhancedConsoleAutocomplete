#pragma once
#include "Game/Types.hpp"
#include <vector>
#include <string>

enum class CommandType { None, CommandName, Coc, GameSetting, ActorValue, Quest, QuestObjective, QuestStage, Perk, FormType, InventoryItem, EquippableItem, PlaceableForm, Note, Faction, Sound, ImageSpaceModifier, Weather, WorldSpace, Idle, Music, FormList, Spell, QuestVariable, Alias };

enum class BaseFormCategory { All, Inventory, Equippable, Placeable };

struct BaseFormEntry {
	std::string editorID;
	char type[4];
};

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
