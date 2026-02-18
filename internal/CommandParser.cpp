#include "CommandParser.hpp"
#include "Game/NVSEInterfaces.hpp"
#include <cstring>
#include <cctype>

extern NVSECommandTableInterface* GetCommandTable();
extern const CommandInfo* GetCommandInfoByName(const char* name);

enum ParamType {
	kParamType_String = 0x00,
	kParamType_Integer = 0x01,
	kParamType_Float = 0x02,
	kParamType_ObjectID = 0x03,
	kParamType_ObjectRef = 0x04,
	kParamType_ActorValue = 0x05,
	kParamType_Actor = 0x06,
	kParamType_SpellItem = 0x07,
	kParamType_Axis = 0x08,
	kParamType_Cell = 0x09,
	kParamType_AnimationGroup = 0x0A,
	kParamType_MagicItem = 0x0B,
	kParamType_Sound = 0x0C,
	kParamType_Topic = 0x0D,
	kParamType_Quest = 0x0E,
	kParamType_Race = 0x0F,
	kParamType_Class = 0x10,
	kParamType_Faction = 0x11,
	kParamType_Sex = 0x12,
	kParamType_Global = 0x13,
	kParamType_Furniture = 0x14,
	kParamType_TESObject = 0x15,
	kParamType_VariableName = 0x16,
	kParamType_QuestStage = 0x17,
	kParamType_MapMarker = 0x18,
	kParamType_ActorBase = 0x19,
	kParamType_Container = 0x1A,
	kParamType_WorldSpace = 0x1B,
	kParamType_CrimeType = 0x1C,
	kParamType_AIPackage = 0x1D,
	kParamType_CombatStyle = 0x1E,
	kParamType_MagicEffect = 0x1F,
	kParamType_FormType = 0x20,
	kParamType_WeatherID = 0x21,
	kParamType_NPC = 0x22,
	kParamType_Owner = 0x23,
	kParamType_EffectShader = 0x24,
	kParamType_FormList = 0x25,
	kParamType_MenuIcon = 0x26,
	kParamType_Perk = 0x27,
	kParamType_Note = 0x28,
	kParamType_MiscellaneousStat = 0x29,
	kParamType_ImageSpaceModifier = 0x2A,
	kParamType_ImageSpace = 0x2B,
};

static char g_CmdNameBuf[128];

static const char* CopyCmdName(const char* src, size_t len) {
	if (len >= sizeof(g_CmdNameBuf)) len = sizeof(g_CmdNameBuf) - 1;
	memcpy(g_CmdNameBuf, src, len);
	g_CmdNameBuf[len] = '\0';
	return g_CmdNameBuf;
}

CommandType InferTypeFromCommand(const char* cmdName, int paramIndex) {
	if (!cmdName || !cmdName[0]) return CommandType::None;

	const CommandInfo* cmd = GetCommandInfoByName(cmdName);
	if (!cmd) return CommandType::None;
	if (cmd->numParams <= paramIndex) return CommandType::None;
	if (!cmd->params) return CommandType::None;

	UInt32 paramType = cmd->params[paramIndex].typeID;
	switch (paramType) {
		case kParamType_Cell:               return CommandType::Coc;
		case kParamType_ActorValue:         return CommandType::ActorValue;
		case kParamType_Quest:              return CommandType::Quest;
		case kParamType_QuestStage:         return CommandType::QuestStage;
		case kParamType_SpellItem:          return CommandType::Spell;
		case kParamType_MagicItem:          return CommandType::Spell;
		case kParamType_Perk:               return CommandType::Perk;
		case kParamType_Note:               return CommandType::Note;
		case kParamType_Faction:            return CommandType::Faction;
		case kParamType_Sound:              return CommandType::Sound;
		case kParamType_ImageSpaceModifier: return CommandType::ImageSpaceModifier;
		case kParamType_WeatherID:          return CommandType::Weather;
		case kParamType_WorldSpace:         return CommandType::WorldSpace;
		case kParamType_FormList:           return CommandType::FormList;
		default:                            return CommandType::None;
	}
}

CommandType InferTypeForSecondParam(const char* cmdName) {
	if (!cmdName || !cmdName[0]) return CommandType::None;

	const CommandInfo* cmd = GetCommandInfoByName(cmdName);
	if (!cmd) return CommandType::None;
	if (cmd->numParams < 2) return CommandType::None;
	if (!cmd->params) return CommandType::None;

	UInt32 paramType = cmd->params[1].typeID;
	if (paramType == kParamType_QuestStage) return CommandType::QuestStage;
	if (paramType == kParamType_Integer && cmd->params[0].typeID == kParamType_Quest)
		return CommandType::QuestObjective;
	return CommandType::None;
}

struct RefCommand {
	const char* name;
	CommandType type;
};

static const RefCommand g_RefCommands[] = {
	{ "setav ", CommandType::ActorValue },
	{ "setactorvalue ", CommandType::ActorValue },
	{ "getav ", CommandType::ActorValue },
	{ "getactorvalue ", CommandType::ActorValue },
	{ "modav ", CommandType::ActorValue },
	{ "modactorvalue ", CommandType::ActorValue },
	{ "forceav ", CommandType::ActorValue },
	{ "forceactorvalue ", CommandType::ActorValue },
	{ "addperk ", CommandType::Perk },
	{ "removeperk ", CommandType::Perk },
	{ "hasperk ", CommandType::Perk },
	{ "addnote ", CommandType::Note },
	{ "addnotens ", CommandType::Note },
	{ "addfaction ", CommandType::Faction },
	{ "removefaction ", CommandType::Faction },
	{ "setfactionrank ", CommandType::Faction },
	{ "getfactionrank ", CommandType::Faction },
	{ "modfactionrank ", CommandType::Faction },
	{ "getfactionreaction ", CommandType::Faction },
	{ "setfactionreaction ", CommandType::Faction },
	{ "setally ", CommandType::Faction },
	{ "setenemy ", CommandType::Faction },
	{ "playsound ", CommandType::Sound },
	{ "playsound3d ", CommandType::Sound },
	{ "imod ", CommandType::ImageSpaceModifier },
	{ "rimod ", CommandType::ImageSpaceModifier },
	{ "additem ", CommandType::InventoryItem },
	{ "removeitem ", CommandType::InventoryItem },
	{ "drop ", CommandType::InventoryItem },
	{ "equipitem ", CommandType::EquippableItem },
	{ "unequipitem ", CommandType::EquippableItem },
	{ "placeatme ", CommandType::PlaceableForm },
	{ "placeleveled ", CommandType::PlaceableForm },
	{ "playidle ", CommandType::Idle },
	{ "forceplayidle ", CommandType::Idle },
	{ "isinlist ", CommandType::FormList },
	{ "listaddreference ", CommandType::FormList },
	{ "listaddref ", CommandType::FormList },
	{ "addspell ", CommandType::Spell },
	{ "addspellns ", CommandType::Spell },
	{ "removespell ", CommandType::Spell },
	{ "cast ", CommandType::Spell },
	{ "castimmediate ", CommandType::Spell },
	{ "castimmediateonself ", CommandType::Spell },
	{ "dispel ", CommandType::Spell },
	{ "evaluatespellconditions ", CommandType::Spell },
	{ "getspellusagenum ", CommandType::Spell },
	{ "isspelltarget ", CommandType::Spell },
	{ "isspelltargetalt ", CommandType::Spell },
	{ "isspelltargetlist ", CommandType::Spell },
};

CommandMatch ParseCommand(const char* s) {
	CommandMatch result = { CommandType::None, nullptr, nullptr, nullptr };
	if (!s) return result;
	while (*s && isspace(*s)) s++;

	if (*s == '!') {
		result.type = CommandType::Alias;
		result.arg = s + 1;
		return result;
	}

	const char* refPrefix = nullptr;
	size_t refPrefixLen = 0;
	const char* cmdStart = s;

	const char* dot = strchr(s, '.');
	if (dot && dot < s + 64) {
		const char* space = strchr(s, ' ');
		if (!space || dot < space) {
			refPrefix = s;
			refPrefixLen = dot - s + 1;
			cmdStart = dot + 1;
		}
	}

	for (const auto& rc : g_RefCommands) {
		size_t cmdLen = strlen(rc.name);
		if (_strnicmp(cmdStart, rc.name, cmdLen) == 0) {
			result.type = rc.type;
			if (refPrefix) {
				memcpy(g_CmdNameBuf, refPrefix, refPrefixLen);
				memcpy(g_CmdNameBuf + refPrefixLen, cmdStart, cmdLen - 1);
				g_CmdNameBuf[refPrefixLen + cmdLen - 1] = '\0';
			} else {
				memcpy(g_CmdNameBuf, cmdStart, cmdLen - 1);
				g_CmdNameBuf[cmdLen - 1] = '\0';
			}
			result.cmdName = g_CmdNameBuf;
			s = cmdStart + cmdLen;
			goto found;
		}
	}

	if (_strnicmp(s, "coc ", 4) == 0) {
		result.type = CommandType::Coc;
		result.cmdName = CopyCmdName(s, 3);
		s += 4;
	} else if (_strnicmp(s, "centeroncell ", 13) == 0) {
		result.type = CommandType::Coc;
		result.cmdName = CopyCmdName(s, 12);
		s += 13;
	} else if (_strnicmp(s, "cow ", 4) == 0) {
		result.type = CommandType::WorldSpace;
		result.cmdName = CopyCmdName(s, 3);
		s += 4;
	} else if (_strnicmp(s, "centeronworld ", 14) == 0) {
		result.type = CommandType::WorldSpace;
		result.cmdName = CopyCmdName(s, 13);
		s += 14;
	} else if (_strnicmp(s, "fw ", 3) == 0) {
		result.type = CommandType::Weather;
		result.cmdName = CopyCmdName(s, 2);
		s += 3;
	} else if (_strnicmp(s, "forceweather ", 13) == 0) {
		result.type = CommandType::Weather;
		result.cmdName = CopyCmdName(s, 12);
		s += 13;
	} else if (_strnicmp(s, "sw ", 3) == 0) {
		result.type = CommandType::Weather;
		result.cmdName = CopyCmdName(s, 2);
		s += 3;
	} else if (_strnicmp(s, "setweather ", 11) == 0) {
		result.type = CommandType::Weather;
		result.cmdName = CopyCmdName(s, 10);
		s += 11;
	} else if (_strnicmp(s, "playmusic ", 10) == 0) {
		result.type = CommandType::Music;
		result.cmdName = CopyCmdName(s, 9);
		s += 10;
	} else if (_strnicmp(s, "ismusicplaying ", 15) == 0) {
		result.type = CommandType::Music;
		result.cmdName = CopyCmdName(s, 14);
		s += 15;
	} else if (_strnicmp(s, "addformtoformlist ", 18) == 0) {
		result.type = CommandType::FormList;
		result.cmdName = CopyCmdName(s, 17);
		s += 18;
	} else if (_strnicmp(s, "removeformfromformlist ", 22) == 0) {
		result.type = CommandType::FormList;
		result.cmdName = CopyCmdName(s, 21);
		s += 22;
	} else if (_strnicmp(s, "dumpformlist ", 13) == 0) {
		result.type = CommandType::FormList;
		result.cmdName = CopyCmdName(s, 12);
		s += 13;
	} else if (_strnicmp(s, "flistdump ", 10) == 0) {
		result.type = CommandType::FormList;
		result.cmdName = CopyCmdName(s, 9);
		s += 10;
	} else if (_strnicmp(s, "setgs ", 6) == 0) {
		result.type = CommandType::GameSetting;
		result.cmdName = CopyCmdName(s, 5);
		s += 6;
	} else if (_strnicmp(s, "getgs ", 6) == 0) {
		result.type = CommandType::GameSetting;
		result.cmdName = CopyCmdName(s, 5);
		s += 6;
	} else if (_strnicmp(s, "setgamesetting ", 15) == 0) {
		result.type = CommandType::GameSetting;
		result.cmdName = CopyCmdName(s, 14);
		s += 15;
	} else if (_strnicmp(s, "getgamesetting ", 15) == 0) {
		result.type = CommandType::GameSetting;
		result.cmdName = CopyCmdName(s, 14);
		s += 15;
	} else if (_strnicmp(s, "search ", 7) == 0) {
		// Pattern: search "somestring" <formtype>
		const char* afterSearch = s + 7;
		while (*afterSearch && isspace(*afterSearch)) afterSearch++;

		if (*afterSearch == '"') {
			const char* closeQuote = strchr(afterSearch + 1, '"');
			if (closeQuote) {
				const char* afterQuote = closeQuote + 1;
				while (*afterQuote && isspace(*afterQuote)) afterQuote++;

				result.type = CommandType::FormType;
				static char searchCmdBuf[256];
				size_t quotedLen = closeQuote - afterSearch + 1;
				if (7 + quotedLen < sizeof(searchCmdBuf)) {
					memcpy(searchCmdBuf, s, 7);
					memcpy(searchCmdBuf + 7, afterSearch, quotedLen);
					searchCmdBuf[7 + quotedLen] = '\0';
					result.cmdName = searchCmdBuf;
				}
				result.arg = afterQuote;
				return result;
			}
		}
	}

found:

	if (result.type != CommandType::None) {
		while (*s && isspace(*s)) s++;
		result.arg = s;

		if ((result.type == CommandType::QuestObjective || result.type == CommandType::QuestStage) && result.arg) {
			const char* space = result.arg;
			while (*space && !isspace(*space)) space++;
			if (*space) {
				static char questBuf[128];
				size_t len = space - result.arg;
				if (len < sizeof(questBuf)) {
					memcpy(questBuf, result.arg, len);
					questBuf[len] = '\0';
					result.arg = questBuf;
					while (*space && isspace(*space)) space++;
					result.arg2 = space;
				}
			}
		}
	} else {
		while (*cmdStart && isspace(*cmdStart)) cmdStart++;
		const char* space = strchr(cmdStart, ' ');
		if (space) {
			size_t cmdLen = space - cmdStart;
			static char inferCmdBuf[128];
			if (cmdLen < sizeof(inferCmdBuf)) {
				memcpy(inferCmdBuf, cmdStart, cmdLen);
				inferCmdBuf[cmdLen] = '\0';

				const char* argStart = space;
				while (*argStart && isspace(*argStart)) argStart++;

				const char* arg2Start = strchr(argStart, ' ');
				if (arg2Start) {
					while (*arg2Start && isspace(*arg2Start)) arg2Start++;

					CommandType secondType = InferTypeForSecondParam(inferCmdBuf);
					if (secondType != CommandType::None) {
						result.type = secondType;
						if (refPrefix && refPrefixLen + cmdLen < sizeof(g_CmdNameBuf)) {
							memcpy(g_CmdNameBuf, refPrefix, refPrefixLen);
							memcpy(g_CmdNameBuf + refPrefixLen, cmdStart, cmdLen);
							g_CmdNameBuf[refPrefixLen + cmdLen] = '\0';
							result.cmdName = g_CmdNameBuf;
						} else {
							result.cmdName = CopyCmdName(cmdStart, cmdLen);
						}

						static char questBuf[128];
						const char* argEnd = argStart;
						while (*argEnd && !isspace(*argEnd)) argEnd++;
						size_t argLen = argEnd - argStart;
						if (argLen < sizeof(questBuf)) {
							memcpy(questBuf, argStart, argLen);
							questBuf[argLen] = '\0';
							result.arg = questBuf;
							while (*argEnd && isspace(*argEnd)) argEnd++;
							result.arg2 = argEnd;
						}
						return result;
					}
				}

				result.type = InferTypeFromCommand(inferCmdBuf);
				if (result.type != CommandType::None) {
					if (refPrefix && refPrefixLen + cmdLen < sizeof(g_CmdNameBuf)) {
						memcpy(g_CmdNameBuf, refPrefix, refPrefixLen);
						memcpy(g_CmdNameBuf + refPrefixLen, cmdStart, cmdLen);
						g_CmdNameBuf[refPrefixLen + cmdLen] = '\0';
						result.cmdName = g_CmdNameBuf;
					} else {
						result.cmdName = CopyCmdName(cmdStart, cmdLen);
					}
					result.arg = argStart;
				}
			}
		} else if (*cmdStart) {
			result.type = CommandType::CommandName;
			result.arg = cmdStart;
			if (refPrefix) {
				static char refBuf[128];
				if (refPrefixLen < sizeof(refBuf)) {
					memcpy(refBuf, refPrefix, refPrefixLen);
					refBuf[refPrefixLen] = '\0';
					result.cmdName = refBuf;
				}
			} else {
				result.cmdName = nullptr;
			}
		}
	}

	//quest.variable: dot prefix with no command match
	if (result.type == CommandType::None && refPrefix) {
		static char questNameBuf[128];
		size_t nameLen = refPrefixLen - 1; //strip the dot
		if (nameLen > 0 && nameLen < sizeof(questNameBuf)) {
			memcpy(questNameBuf, refPrefix, nameLen);
			questNameBuf[nameLen] = '\0';
			result.type = CommandType::QuestVariable;
			result.cmdName = questNameBuf;
			result.arg = cmdStart;
		}
	}

	return result;
}
