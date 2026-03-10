#pragma once
#include <cstring>

inline void StripCursor(char* s) {
	if (char* c = strchr(s, '|'))
		memmove(c, c + 1, strlen(c));
}
