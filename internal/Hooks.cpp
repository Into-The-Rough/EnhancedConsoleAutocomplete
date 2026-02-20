#include "Hooks.hpp"
#include "Autocomplete.hpp"
#include "Cache.hpp"
#include "HistorySearch.hpp"
#include "CommandParser.hpp"
#include "Game/ConsoleManager.hpp"
#include "Game/Types.hpp"
#include <windows.h>
#include <cstring>
#include <cstdio>
#include <string>

static void* g_DIHookCtrl = nullptr;
static UInt32 g_OriginalHandler = 0;
static UInt32 g_OriginalPrint = 0;
static std::string g_CmdSuggestion;
static std::string g_CmdSuggestionInput;
static bool g_InHook = false;

enum {
	kSpclChar_LeftArrow = 0x80000001,
	kSpclChar_RightArrow = 0x80000002,
	kSpclChar_Enter = 0x80000008,
};

void SetDIHookCtrl(void* ctrl) {
	g_DIHookCtrl = ctrl;
}

static void WriteRelCall(UInt32 addr, UInt32 dest) {
	DWORD oldProtect;
	VirtualProtect((void*)addr, 5, PAGE_EXECUTE_READWRITE, &oldProtect);
	*(UInt8*)addr = 0xE8;
	*(UInt32*)(addr + 1) = dest - addr - 5;
	VirtualProtect((void*)addr, 5, oldProtect, &oldProtect);
}

static void SafeWriteBuf(UInt32 addr, const void* data, UInt32 len) {
	DWORD oldProtect;
	VirtualProtect((void*)addr, len, PAGE_EXECUTE_READWRITE, &oldProtect);
	memcpy((void*)addr, data, len);
	VirtualProtect((void*)addr, len, oldProtect, &oldProtect);
}

static bool IsCtrlHeld() {
	if (!g_DIHookCtrl) return false;
	return *((UInt8*)g_DIHookCtrl + 0xCF) != 0 || *((UInt8*)g_DIHookCtrl + 0x44F) != 0;  // left and right ctrl
}

static bool IsShiftHeld() {
	if (!g_DIHookCtrl) return false;
	return *((UInt8*)g_DIHookCtrl + 0x2A * 7 + 4) != 0 || *((UInt8*)g_DIHookCtrl + 0x36 * 7 + 4) != 0;
}

static void BuildFullSuggestion(const char* cmdName) {
	if (!cmdName || !*cmdName) {
		g_CmdSuggestion.clear();
		return;
	}

	g_CmdSuggestion = cmdName;

	const CommandInfo* cmd = GetCommandInfoByName(cmdName);
	if (cmd) {
		std::string params = FormatCommandParams(cmd);
		g_CmdSuggestion += params;
	}
}

static void UpdateCommandSuggestion(const char* rawInput) {
	if (!rawInput || !*rawInput) {
		g_CmdSuggestion.clear();
		g_CmdSuggestionInput.clear();
		return;
	}

	char clean[256];
	strncpy_s(clean, rawInput, _TRUNCATE);
	if (char* c = strchr(clean, '|')) memmove(c, c + 1, strlen(c));
	if (!*clean) {
		g_CmdSuggestion.clear();
		g_CmdSuggestionInput.clear();
		return;
	}

	if (strchr(clean, ' ')) {
		g_CmdSuggestion.clear();
		g_CmdSuggestionInput.clear();
		return;
	}

	if ((Autocomplete::g_LastType == CommandType::QuestVariable || Autocomplete::g_LastType == CommandType::Alias)
		&& !Autocomplete::g_LastInput.empty()) {
		g_CmdSuggestion.clear();
		g_CmdSuggestionInput.clear();
		return;
	}

	const char* cmdPart = clean;
	const char* dot = strchr(clean, '.');
	if (dot) cmdPart = dot + 1;

	if (g_CmdSuggestionInput == cmdPart) {
		if (const char* match = Autocomplete::Current())
			BuildFullSuggestion(match);
		return;
	}

	std::string suggestionCmd = g_CmdSuggestion;
	size_t spacePos = suggestionCmd.find(' ');
	if (spacePos != std::string::npos) suggestionCmd = suggestionCmd.substr(0, spacePos);

	if (_stricmp(suggestionCmd.c_str(), cmdPart) == 0) {
		g_CmdSuggestionInput = cmdPart;
		BuildFullSuggestion(suggestionCmd.c_str());
		return;
	}

	g_CmdSuggestionInput = cmdPart;
	CommandNames::Build();
	Autocomplete::FindCommands(cmdPart);
	if (const char* match = Autocomplete::Current())
		BuildFullSuggestion(match);
	else
		g_CmdSuggestion.clear();
}

static bool __fastcall HookHandler(ConsoleManager* mgr, void*, int key) {
	if (mgr->isConsoleOpen <= 0 || g_InHook)
		return ThisCall<bool>(g_OriginalHandler, mgr, key);

	g_InHook = true;

	if (HistorySearch::IsActive()) {
		if (key == VK_TAB || key == VK_RETURN || key == 0x8000000D) {
			if (const char* match = HistorySearch::Current()) {
				String* input = GetDebugInput();
				if (input) {
					input->Set(match);
					mgr->HandleInput(kSpclChar_RightArrow);
				}
			}
			HistorySearch::Reset();
			g_InHook = false;
			return true;
		} else if (IsCtrlHeld() && (key == 'c' || key == 'C')) {
			std::string orig = HistorySearch::sOriginal;
			HistorySearch::Reset();
			String* input = GetDebugInput();
			if (input) {
				char buf[256];
				strncpy_s(buf, orig.c_str(), _TRUNCATE);
				if (char* p = strchr(buf, '|')) memmove(p, p + 1, strlen(p));
				input->Set(buf);
			}
			ThisCall<void>(0x71D410, mgr);
			mgr->HandleInput(kSpclChar_RightArrow);
			g_InHook = false;
			return true;
		} else if (IsCtrlHeld() && (key == 'r' || key == 'R')) {
			HistorySearch::Next();
			if (const char* match = HistorySearch::Current()) {
				String* input = GetDebugInput();
				if (input) {
					input->Set(match);
					mgr->HandleInput(kSpclChar_RightArrow);
				}
			}
			g_InHook = false;
			return true;
		}
		HistorySearch::Reset();
	}

	if (IsCtrlHeld() && (key == 'r' || key == 'R')) {
		String* input = GetDebugInput();
		HistorySearch::Enter(input ? input->m_data : NULL);
		if (const char* match = HistorySearch::Current()) {
			if (input) {
				input->Set(match);
				mgr->HandleInput(kSpclChar_RightArrow);
			}
		}
		g_InHook = false;
		return true;
	}

	if (key == kSpclChar_Enter) {
		String* input = GetDebugInput();
		if (input && input->m_data) {
			char clean[256];
			strncpy_s(clean, input->m_data, _TRUNCATE);
			if (char* c = strchr(clean, '|')) memmove(c, c + 1, strlen(c));
			if (clean[0] == '!') {
				const char* expanded = Aliases::Lookup(clean + 1);
				if (expanded)
					input->Set(expanded);
			}
		}
		Autocomplete::g_LastInput.clear();
		Autocomplete::g_LastType = CommandType::None;
		g_InHook = false;
		return ThisCall<bool>(g_OriginalHandler, mgr, key);
	}

	if (key == VK_TAB) {
		String* input = GetDebugInput();
		if (input && input->m_data) {
			char clean[256];
			strncpy_s(clean, input->m_data, _TRUNCATE);
			if (char* c = strchr(clean, '|')) memmove(c, c + 1, strlen(c));

			auto cmd = ParseCommand(clean);
			if (cmd.type != CommandType::None) {
				std::string current = clean;
				bool shouldCycle = (current == Autocomplete::g_LastInput &&
				                    Autocomplete::Count() > 1);

				if (shouldCycle) {
					if (IsShiftHeld())
						Autocomplete::Prev();
					else
						Autocomplete::Next();
				} else {
					Autocomplete::g_LastType = cmd.type;

					const char* recType = CommandTypeToRecordType(cmd.type);
					if (recType) {
						Autocomplete::FindForms(recType, cmd.arg);
					} else if (cmd.type == CommandType::GameSetting) {
						GameSettings::Build();
						Autocomplete::FindSettings(cmd.arg);
					} else if (cmd.type == CommandType::ActorValue) {
						ActorValues::Build();
						Autocomplete::FindActorValues(cmd.arg);
					} else if (cmd.type == CommandType::QuestObjective) {
						if (cmd.arg2 != nullptr)
							Autocomplete::FindObjectives(cmd.arg, cmd.arg2);
						else
							Autocomplete::FindForms("QUST", cmd.arg);
					} else if (cmd.type == CommandType::QuestStage) {
						if (cmd.arg2 != nullptr)
							Autocomplete::FindStages(cmd.arg, cmd.arg2);
						else
							Autocomplete::FindForms("QUST", cmd.arg);
					} else if (cmd.type == CommandType::QuestVariable) {
						Autocomplete::FindQuestVariables(cmd.cmdName, cmd.arg);
					} else if (cmd.type == CommandType::Alias) {
						Autocomplete::FindAliases(cmd.arg);
					} else if (cmd.type == CommandType::FormType) {
						FormTypes::Build();
						Autocomplete::FindFormTypes(cmd.arg);
					} else if (cmd.type == CommandType::InventoryItem) {
						BaseForms::Build();
						Autocomplete::FindBaseForms(cmd.arg, BaseFormCategory::Inventory);
					} else if (cmd.type == CommandType::EquippableItem) {
						BaseForms::Build();
						Autocomplete::FindBaseForms(cmd.arg, BaseFormCategory::Equippable);
					} else if (cmd.type == CommandType::PlaceableForm) {
						BaseForms::Build();
						Autocomplete::FindBaseForms(cmd.arg, BaseFormCategory::Placeable);
					} else if (cmd.type == CommandType::CommandName) {
						CommandNames::Build();
						Autocomplete::FindCommands(cmd.arg);
						if (Autocomplete::Count() == 0 && cmd.cmdName && strchr(cmd.cmdName, '.')) {
							static char qBuf[128];
							strncpy_s(qBuf, cmd.cmdName, _TRUNCATE);
							if (char* d = strrchr(qBuf, '.')) *d = '\0';
							Autocomplete::FindQuestVariables(qBuf, cmd.arg);
							if (Autocomplete::Count() > 0) {
								cmd.type = CommandType::QuestVariable;
								cmd.cmdName = qBuf;
							}
						}
					}
				}

				if (const char* match = Autocomplete::Current()) {
					char newCmd[256];
					if (cmd.type == CommandType::Alias) {
						snprintf(newCmd, sizeof(newCmd), "!%s", match);
					} else if (cmd.type == CommandType::CommandName) {
						if (cmd.cmdName)
							snprintf(newCmd, sizeof(newCmd), "%s%s", cmd.cmdName, match);
						else
							snprintf(newCmd, sizeof(newCmd), "%s", match);
						g_CmdSuggestionInput = match;
						BuildFullSuggestion(match);
					} else if (cmd.type == CommandType::QuestVariable) {
						snprintf(newCmd, sizeof(newCmd), "%s.%s", cmd.cmdName, match);
					} else if ((cmd.type == CommandType::QuestObjective || cmd.type == CommandType::QuestStage) && cmd.arg2 != nullptr) {
						snprintf(newCmd, sizeof(newCmd), "%s %s %s", cmd.cmdName, cmd.arg, match);
					} else {
						snprintf(newCmd, sizeof(newCmd), "%s %s", cmd.cmdName, match);
					}

					Autocomplete::g_LastInput = newCmd;
					input->Set(newCmd);
					mgr->HandleInput(kSpclChar_RightArrow);
					g_InHook = false;
					return true;
				}
				g_InHook = false;
				return true;
			}
		}
		g_InHook = false;
		return true;
	}

	if (key != VK_TAB && key != kSpclChar_RightArrow && key != kSpclChar_LeftArrow) {
		Autocomplete::g_LastInput.clear();
		Autocomplete::g_LastType = CommandType::None;
	}

	g_InHook = false;
	return ThisCall<bool>(g_OriginalHandler, mgr, key);
}

struct NiColorAlpha { float r, g, b, a; };

static NiColorAlpha g_GrayColor = { 0.6f, 0.6f, 0.6f, 0.7f };
static const UInt32 kDebugTextPrint = 0xA0F8B0;

static const char* GetTypeHint(CommandType type) {
	switch (type) {
		case CommandType::Coc:                return "cells";
		case CommandType::Quest:              return "quests";
		case CommandType::QuestStage:         return "quest stages";
		case CommandType::QuestObjective:     return "quest objectives";
		case CommandType::Perk:               return "perks";
		case CommandType::Note:               return "notes";
		case CommandType::Faction:            return "factions";
		case CommandType::Sound:              return "sounds";
		case CommandType::ImageSpaceModifier: return "image space modifiers";
		case CommandType::Weather:            return "weathers";
		case CommandType::WorldSpace:         return "world spaces";
		case CommandType::Idle:               return "idle animations";
		case CommandType::Music:              return "music types";
		case CommandType::FormList:           return "form lists";
		case CommandType::Spell:              return "spells";
		case CommandType::FormType:           return "form types";
		case CommandType::InventoryItem:      return "items";
		case CommandType::EquippableItem:     return "equippables";
		case CommandType::PlaceableForm:      return "placeables";
		default:                              return nullptr;
	}
}

static void __fastcall HookPrint(DebugText* debugText, void*, char* str, float xPos, float yPos, int alignment, int a6, float duration, int fontNumber, NiColorAlpha* color) {
	ConsoleManager* mgr = ConsoleManager::GetSingleton();
	ThisCall<void>(g_OriginalPrint, debugText, str, xPos, yPos, alignment, a6, duration, fontNumber, color);

	if (!str || !*str) return;

	float lh = mgr ? (float)mgr->lineHeight : 16.0f;
	float suggestionY = yPos - lh;

	if (HistorySearch::IsActive()) {
		char prompt[256];
		HistorySearch::GetPrompt(prompt, sizeof(prompt));
		NiColorAlpha col = { 0.7f, 0.9f, 1.0f, 1.0f };
		ThisCall<void>(kDebugTextPrint, debugText, prompt, xPos, suggestionY, alignment, a6, duration, fontNumber, &col);
	} else {
		UpdateCommandSuggestion(str);

		char cleanInput[256];
		strncpy_s(cleanInput, str, _TRUNCATE);
		if (char* c = strchr(cleanInput, '|')) memmove(c, c + 1, strlen(c));

		if (cleanInput[0] == '!') {
			const char* aliasName = cleanInput + 1;
			const char* expanded = Aliases::Lookup(aliasName);
			if (expanded) {
				char hint[256];
				snprintf(hint, sizeof(hint), "-> %s", expanded);
				NiColorAlpha aliasCol = { 0.5f, 1.0f, 0.5f, 0.9f };
				char padded[256];
				snprintf(padded, sizeof(padded), "%-100s", hint);
				ThisCall<void>(kDebugTextPrint, debugText, padded, xPos, suggestionY, alignment, a6, duration, fontNumber, &aliasCol);
			} else if (*aliasName) {
				char padded[128];
				snprintf(padded, sizeof(padded), "%-100s", "Press TAB to cycle aliases");
				ThisCall<void>(kDebugTextPrint, debugText, padded, xPos, suggestionY, alignment, a6, duration, fontNumber, &g_GrayColor);
			} else {
				char padded[128];
				snprintf(padded, sizeof(padded), "%-100s", "Type alias name (TAB to cycle)");
				ThisCall<void>(kDebugTextPrint, debugText, padded, xPos, suggestionY, alignment, a6, duration, fontNumber, &g_GrayColor);
			}
		} else {

		auto renderPadded = [&](const char* text) {
			char padded[128];
			snprintf(padded, sizeof(padded), "%-100s", text);
			ThisCall<void>(kDebugTextPrint, debugText, padded, xPos, suggestionY, alignment, a6, duration, fontNumber, &g_GrayColor);
		};

		auto cmd = ParseCommand(cleanInput);

		if (cmd.type == CommandType::ActorValue) {
			bool showed = false;
			if (cmd.arg && *cmd.arg) {
				char avName[64];
				const char* end = cmd.arg;
				while (*end && !isspace((unsigned char)*end)) end++;
				size_t len = end - cmd.arg;
				if (len > 0 && len < sizeof(avName)) {
					memcpy(avName, cmd.arg, len);
					avName[len] = '\0';
					UInt32 avCode = GetActorValueCode(avName);
					if (avCode != 0xFFFFFFFF) {
						bool isPlayer = _strnicmp(cleanInput, "player.", 7) == 0;
						void* ref = isPlayer ? GetPlayerRef() : GetConsoleSelectedRef();
						if (ref) {
							float val = GetActorValueForRef(ref, avCode);
							char valBuf[128];
							snprintf(valBuf, sizeof(valBuf), "%s = %.2f", avName, val);
							renderPadded(valBuf);
							showed = true;
						}
					}
				}
			}
			if (!showed)
				renderPadded(g_CmdSuggestion.empty() ? "Press TAB to cycle actor values" : g_CmdSuggestion.c_str());
		} else if (cmd.type == CommandType::GameSetting) {
			bool showed = false;
			if (cmd.arg && *cmd.arg) {
				char nameBuf[128];
				const char* end = cmd.arg;
				while (*end && !isspace((unsigned char)*end)) end++;
				size_t len = end - cmd.arg;
				if (len > 0 && len < sizeof(nameBuf)) {
					memcpy(nameBuf, cmd.arg, len);
					nameBuf[len] = '\0';
					Setting* s = LookupGameSetting(nameBuf);
					if (s) {
						char valBuf[256];
						if (FormatSettingValue(s, valBuf, sizeof(valBuf))) {
							renderPadded(valBuf);
							showed = true;
						}
					}
				}
			}
			if (!showed)
				renderPadded(g_CmdSuggestion.empty() ? "Press TAB to cycle game settings" : g_CmdSuggestion.c_str());
		} else if (cmd.type != CommandType::None && cmd.type != CommandType::CommandName
		        && cmd.type != CommandType::QuestVariable && cmd.type != CommandType::Alias) {
			if (!g_CmdSuggestion.empty()) {
				renderPadded(g_CmdSuggestion.c_str());
			} else {
				const char* hint = GetTypeHint(cmd.type);
				if (hint) {
					char buf[64];
					if (!Cache::IsBuilt())
						snprintf(buf, sizeof(buf), "Press TAB to load %s...", hint);
					else
						snprintf(buf, sizeof(buf), "Press TAB to cycle %s", hint);
					renderPadded(buf);
				} else {
					renderPadded("");
				}
			}
		} else if (!g_CmdSuggestion.empty()) {
			renderPadded(g_CmdSuggestion.c_str());
		} else {
			renderPadded("");
		}

		}

		int matchCount = Autocomplete::Count();
		int matchIdx = Autocomplete::GetIndex();
		if (g_MatchListSize > 0 && matchCount > 1 && matchIdx >= 0) {
			int visible = (matchCount < g_MatchListSize) ? matchCount : g_MatchListSize;
			int half = visible / 2;
			int start = matchIdx - half;
			if (start < 0) start = 0;
			if (start + visible > matchCount) start = matchCount - visible;

			float listX = CdeclCall<float>(0x715D40) * 0.55f;
			NiColorAlpha selColor = { 1.0f, 1.0f, 1.0f, 1.0f };

			char countLine[32];
			snprintf(countLine, sizeof(countLine), "[%d/%d]", matchIdx + 1, matchCount);
			float baseY = suggestionY - lh;
			ThisCall<void>(kDebugTextPrint, debugText, countLine, listX, baseY, alignment, a6, duration, fontNumber, &g_GrayColor);

			for (int i = 0; i < visible; i++) {
				int idx = start + i;
				const char* m = Autocomplete::GetMatch(idx);
				if (!m) break;
				char line[128];
				snprintf(line, sizeof(line), "%s %s", (idx == matchIdx) ? ">" : " ", m);
				float lineY = baseY - lh * (i + 1);
				NiColorAlpha* c = (idx == matchIdx) ? &selColor : &g_GrayColor;
				ThisCall<void>(kDebugTextPrint, debugText, line, listX, lineY, alignment, a6, duration, fontNumber, c);
			}
		}
	}
}

void InitHooks() {
	UInt8* site = (UInt8*)0x70E09E;
	if (*site == 0xE8) {
		g_OriginalHandler = (UInt32)(site + 5 + *(SInt32*)(site + 1));
		WriteRelCall(0x70E09E, (UInt32)HookHandler);
	}

	site = (UInt8*)0x71CF8B;
	if (*site == 0xE8) {
		g_OriginalPrint = (UInt32)(site + 5 + *(SInt32*)(site + 1));
		WriteRelCall(0x71CF8B, (UInt32)HookPrint);
	}

	SafeWriteBuf(0x71D427, "\xA1\x00\x8D\x1D\x01\x8D\x50\x01\x8B\x4D\xE8\x90\x90\x90\x90", 15);
}
