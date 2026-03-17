#include "HistorySearch.hpp"
#include "Utils.hpp"
#include "Game/ConsoleManager.hpp"
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include <cstdio>

namespace HistorySearch {

static bool bActive = false;
static std::string sQuery;
static std::vector<std::string> matches;
static int idx = -1;
std::string sOriginal;

bool IsActive() { return bActive; }

void Reset() {
	bActive = false;
	sQuery.clear();
	matches.clear();
	idx = -1;
	sOriginal.clear();
}

static bool ContainsCI(const char* str, const std::string& substr) {
	if (substr.empty()) return true;
	std::string hay = str;
	std::transform(hay.begin(), hay.end(), hay.begin(), ::tolower);
	return hay.find(substr) != std::string::npos;
}

static void FindMatches() {
	matches.clear();
	idx = -1;
	ConsoleManager* mgr = ConsoleManager::GetSingleton();
	if (!mgr) return;

	for (TextNode* node = mgr->inputHistory.first; node; node = node->next) {
		if (!node->text.m_data || !*node->text.m_data) continue;

		char buf[256];
		strncpy_s(buf, node->text.m_data, _TRUNCATE);
		StripCursor(buf);

		if (!sQuery.empty() && !ContainsCI(buf, sQuery)) continue;

		bool dupe = false;
		for (auto& m : matches) {
			if (_stricmp(m.c_str(), buf) == 0) { dupe = true; break; }
		}
		if (!dupe) matches.push_back(buf);
	}
	if (!matches.empty()) idx = 0;
}

void Enter(const char* input) {
	bActive = true;
	sOriginal = input ? input : "";
	char buf[256];
	strncpy_s(buf, sOriginal.c_str(), _TRUNCATE);
	StripCursor(buf);
	sQuery = buf;
	std::transform(sQuery.begin(), sQuery.end(), sQuery.begin(), ::tolower);
	FindMatches();
}

void Next() {
	if (!matches.empty()) idx = (idx + 1) % matches.size();
}

void Prev() {
	if (!matches.empty()) idx = (idx - 1 + (int)matches.size()) % (int)matches.size();
}

const char* Current() {
	return (idx >= 0 && idx < (int)matches.size()) ? matches[idx].c_str() : NULL;
}

int Count() { return (int)matches.size(); }

void GetPrompt(char* out, size_t sz) {
	if (matches.empty())
		snprintf(out, sz, "(search)'%s': no matches", sQuery.c_str());
	else
		snprintf(out, sz, "(search %d/%d)'%s':", idx + 1, (int)matches.size(), sQuery.c_str());
}

}
