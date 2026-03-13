#pragma once
#include "Autocomplete.hpp"
#include "Game/Forms.hpp"
#include "Game/NVSEInterfaces.hpp"
#include <vector>
#include <string>

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
	extern std::vector<std::string> g_List;
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

void CollectQuestVars(const char* questEditorID, std::vector<std::string>& out);
