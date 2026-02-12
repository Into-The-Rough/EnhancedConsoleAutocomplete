#include <windows.h>
#include "internal/Game/Types.hpp"
#include "internal/Game/NVSEInterfaces.hpp"
#include "internal/Hooks.hpp"
#include "internal/Autocomplete.hpp"

#define PLUGIN_NAME "Enhanced Console Autocomplete"
#define PLUGIN_VERSION 1

static constexpr UInt32 kGameVersion = 0x040020D0;

int g_MatchListSize = 5;

static void LoadINI() {
	char iniPath[MAX_PATH];
	GetModuleFileNameA(nullptr, iniPath, MAX_PATH);
	if (char* p = strrchr(iniPath, '\\')) *p = '\0';
	strcat_s(iniPath, "\\Data\\config\\EnhancedConsoleAutocomplete.ini");
	g_MatchListSize = GetPrivateProfileIntA("General", "iMatchListSize", 5, iniPath);
	if (g_MatchListSize < 0) g_MatchListSize = 0;
	if (g_MatchListSize > 20) g_MatchListSize = 20;
	Aliases::Load(iniPath);
}

static UInt32 g_PluginHandle = 0xFFFFFFFF;
static NVSEMessagingInterface* g_Messaging = nullptr;

static void MessageHandler(NVSEMessagingInterface::Message* msg) {
	if (msg->type == NVSEMessagingInterface::kMessage_PostPostLoad)
		InitHooks();
}

extern "C" __declspec(dllexport) bool NVSEPlugin_Query(const NVSEInterface* nvse, PluginInfo* info) {
	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = PLUGIN_NAME;
	info->version = PLUGIN_VERSION;
	return !nvse->isEditor && nvse->runtimeVersion == kGameVersion;
}

extern "C" __declspec(dllexport) bool NVSEPlugin_Load(const NVSEInterface* nvse) {
	LoadINI();
	g_PluginHandle = nvse->GetPluginHandle();

	if (auto* data = (NVSEDataInterface*)nvse->QueryInterface(kInterface_Data))
		SetDIHookCtrl(data->GetSingleton(NVSEDataInterface::kNVSEData_DIHookControl));

	SetCommandTable((NVSECommandTableInterface*)nvse->QueryInterface(kInterface_CommandTable));

	g_Messaging = (NVSEMessagingInterface*)nvse->QueryInterface(kInterface_Messaging);
	if (g_Messaging && g_Messaging->RegisterListener)
		g_Messaging->RegisterListener(g_PluginHandle, "NVSE", MessageHandler);
	else
		InitHooks();

	return true;
}

BOOL WINAPI DllMain(HMODULE, DWORD, LPVOID) { return TRUE; }
