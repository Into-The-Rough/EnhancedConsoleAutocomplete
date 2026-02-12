#pragma once
#include "Types.hpp"

struct PluginInfo {
	enum { kInfoVersion = 1 };
	UInt32 infoVersion;
	const char* name;
	UInt32 version;
};

struct NVSEInterface {
	UInt32 nvseVersion;
	UInt32 runtimeVersion;
	UInt32 editorVersion;
	UInt32 isEditor;
	void* RegisterCommand;
	void* SetOpcodeBase;
	void* (*QueryInterface)(UInt32 id);
	UInt32 (*GetPluginHandle)(void);
};

struct NVSEMessagingInterface {
	struct Message {
		const char* sender;
		UInt32 type;
		UInt32 dataLen;
		void* data;
	};
	typedef void (*EventCallback)(Message* msg);
	enum { kMessage_PostPostLoad = 9 };
	UInt32 version;
	bool (*RegisterListener)(UInt32 listener, const char* sender, EventCallback handler);
};

struct NVSEDataInterface {
	UInt32 version;
	enum { kNVSEData_DIHookControl = 1 };
	void* (*GetSingleton)(UInt32 singletonID);
};

enum {
	kInterface_Serialization = 0,
	kInterface_Console = 1,
	kInterface_Messaging = 2,
	kInterface_CommandTable = 3,
	kInterface_StringVar = 4,
	kInterface_ArrayVar = 5,
	kInterface_Script = 6,
	kInterface_Data = 7,
};

struct ParamInfo {
	const char* typeStr;
	UInt32 typeID;
	UInt32 isOptional;
};

struct CommandInfo {
	const char* longName;
	const char* shortName;
	UInt32 opcode;
	const char* helpText;
	UInt16 needsParent;
	UInt16 numParams;
	ParamInfo* params;
	void* execute;
	void* parse;
	void* eval;
	UInt32 flags;
};

struct NVSECommandTableInterface {
	UInt32 version;
	const CommandInfo* (*Start)(void);
	const CommandInfo* (*End)(void);
	const CommandInfo* (*GetByOpcode)(UInt32 opcode);
	const CommandInfo* (*GetByName)(const char* name);
	UInt32 (*GetReturnType)(const CommandInfo* cmd);
	UInt32 (*GetRequiredNVSEVersion)(const CommandInfo* cmd);
	void* (*GetParentPlugin)(const CommandInfo* cmd);
};
