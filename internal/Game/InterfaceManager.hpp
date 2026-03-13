#pragma once
#include "Types.hpp"
#include "Forms.hpp"

struct InterfaceManager {
	UInt8 pad000[0x38];
	float cursorX;
	float pad03C;
	float cursorY;
	UInt8 pad044[0xAC];
	TESForm* debugSelection;

	static InterfaceManager* GetSingleton() { return *(InterfaceManager**)0x11D8A80; }
};
