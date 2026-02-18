#include "HistorySearch.hpp"
#include "Game/ConsoleManager.hpp"
#include <vector>
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

static const char* _stristr(const char* str, const char* substr) {
	if (!*substr) return str;
	for (; *str; str++) {
		const char* s = str;
		const char* sub = substr;
		while (*s && *sub && (tolower(*s) == tolower(*sub))) { s++; sub++; }
		if (!*sub) return str;
	}
	return NULL;
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
		if (char* cursor = strchr(buf, '|')) memmove(cursor, cursor + 1, strlen(cursor));

		if (!sQuery.empty() && !_stristr(buf, sQuery.c_str())) continue;

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
	if (char* p = strchr(buf, '|')) memmove(p, p + 1, strlen(p));
	sQuery = buf;
	FindMatches();
}

void Next() {
	if (!matches.empty()) idx = (idx + 1) % matches.size();
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
