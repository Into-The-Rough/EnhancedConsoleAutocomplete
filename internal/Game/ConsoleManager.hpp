#pragma once
#include "Types.hpp"

struct TextNode {
	TextNode* next;
	TextNode* prev;
	String text;
};

struct TextList {
	TextNode* first;
	TextNode* last;
	UInt32 count;
};

struct ConsoleManager {
	void* scriptContext;
	TextList printedLines;
	TextList inputHistory;
	UInt32 historyIndex;
	UInt32 unk020;
	UInt32 printedCount;
	UInt32 unk028;
	UInt32 lineHeight;
	int textXPos;
	int textYPos;
	UInt8 isConsoleOpen;

	bool HandleInput(int key) { return ThisCall<bool>(0x71B210, this, key); }
	static ConsoleManager* GetSingleton() { return *(ConsoleManager**)0x11D8CE8; }
};

struct DebugLine {
	float fOffsetX;       //0x00
	float fOffsetY;       //0x04
	UInt32 uiAlignment;   //0x08
	void* spNode;         //0x0C
	String strText;       //0x10
	float fLifetime;      //0x18
	UInt8 color[0x10];    //0x1C - NiColorAlpha
};
static_assert(sizeof(DebugLine) == 0x2C);

static constexpr UInt32 kDebugTextLineCount = 200; //DebugText::kLines[200], see DebugText::Idle

struct DebugText {
	void* vtbl;
	DebugLine kLines[kDebugTextLineCount];

	static DebugText* GetSingleton() { return CdeclCall<DebugText*>(0xA0D9E0, true); }
};

//console input is whichever kLines entry has the highest fOffsetY
inline String* GetDebugInput() {
	DebugText* dt = DebugText::GetSingleton();
	if (!dt) return nullptr;
	DebugLine* result = &dt->kLines[0];
	for (UInt32 i = 1; i < kDebugTextLineCount; i++) {
		if (!dt->kLines[i].strText.m_data) break;
		if (dt->kLines[i].fOffsetY > result->fOffsetY)
			result = &dt->kLines[i];
	}
	return &result->strText;
}
