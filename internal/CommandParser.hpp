#pragma once
#include "Autocomplete.hpp"

struct CommandMatch {
	CommandType type;
	const char* arg;
	const char* arg2;
	const char* cmdName;
};

CommandMatch ParseCommand(const char* s);
CommandType InferTypeFromCommand(const char* cmdName, int paramIndex = 0);
CommandType InferTypeForSecondParam(const char* cmdName);
