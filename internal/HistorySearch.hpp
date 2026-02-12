#pragma once
#include <string>

namespace HistorySearch {
	extern std::string sOriginal;

	bool IsActive();
	void Reset();
	void Enter(const char* input);
	void Next();
	const char* Current();
	int Count();
	void GetPrompt(char* out, size_t sz);
}
