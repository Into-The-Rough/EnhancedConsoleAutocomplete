#pragma once
#include "Types.hpp"

constexpr UInt8 kFormType_GMST = 3;
constexpr UInt8 kFormType_GLOB = 6;
constexpr UInt8 kFormType_Cell = 57;
constexpr UInt8 kFormType_REFR = 58;
constexpr UInt8 kFormType_ACHR = 59;
constexpr UInt8 kFormType_ACRE = 60;
constexpr UInt8 kFormType_LAND = 66;
constexpr UInt8 kFormType_NAVM = 67;
constexpr UInt8 kFormType_NAVI = 68;
constexpr UInt8 kFormType_DIAL = 69;
constexpr UInt8 kFormType_INFO = 70;
constexpr UInt8 kFormType_Quest = 71;
constexpr UInt8 kFormType_BGSPerk = 105;

struct TESForm {
	void* vtbl;
	UInt8 typeID;
	UInt8 pad05[3];
	UInt32 flags;
	UInt32 refID;
};

struct BGSQuestObjective {
	void* vtbl;
	UInt32 objectiveId;
	char* displayText;
	UInt32 displayTextLen;
	void* quest;
};

constexpr UInt32 kVtbl_BGSQuestObjective = 0x1047088;

struct StageInfo {
	UInt8 stage;
	UInt8 unk01;
	UInt8 pad[2];
};

struct TESQuest {
	UInt8 pad00[0x44];
	ListNode<StageInfo> stages;
	ListNode<void> lVarOrObjectives;
};

struct Setting {
	void* vtbl;
	union { UInt32 uint; int i; float f; char* str; } data;
	const char* name;
};

struct GameSettingCollection {
	void* vtbl;
	UInt8 pad004[0x108];
	NiTMapBase<const char*, Setting*> settingMap;
};

typedef char* (*_GetActorValueName)(UInt32 actorValueCode);

TESForm* LookupFormByEditorID(const char* editorID);
