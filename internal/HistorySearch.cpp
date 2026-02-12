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

static char g_TempStrings[1000][256];
static int g_TempStringCount = 0;

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

static bool CollectHistoryStrings(ConsoleManager* mgr) {
	g_TempStringCount = 0;
	__try {
		int count = 0;
		for (TextNode* node = mgr->inputHistory.first; node && count < 1000; node = node->next, count++) {
			if ((UInt32)node < 0x10000) break;
			if (!node->text.m_data) continue;
			if ((UInt32)node->text.m_data < 0x10000) continue;
			if (node->text.m_dataLen == 0 || node->text.m_dataLen > 4096) continue;
			if (!*node->text.m_data) continue;
			unsigned char firstChar = (unsigned char)node->text.m_data[0];
			if (firstChar < 0x20 && firstChar != '\t') continue;

			if (g_TempStringCount < 1000) {
				strncpy_s(g_TempStrings[g_TempStringCount], sizeof(g_TempStrings[0]),
				          node->text.m_data, _TRUNCATE);
				char* cursor = strchr(g_TempStrings[g_TempStringCount], '|');
				if (cursor) memmove(cursor, cursor + 1, strlen(cursor));  // strip cursor char
				g_TempStringCount++;
			}
		}
		return true;
	} __except(EXCEPTION_EXECUTE_HANDLER) {
		return false;
	}
}

static void FindMatches() {
	matches.clear();
	idx = -1;
	ConsoleManager* mgr = ConsoleManager::GetSingleton();
	if (!mgr) return;

	if (!CollectHistoryStrings(mgr)) return;

	for (int i = 0; i < g_TempStringCount; i++) {
		const char* str = g_TempStrings[i];
		if (!sQuery.empty() && !_stristr(str, sQuery.c_str())) continue;

		bool dupe = false;
		for (auto& m : matches) {
			if (_stricmp(m.c_str(), str) == 0) { dupe = true; break; }
		}
		if (!dupe) matches.push_back(str);
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
