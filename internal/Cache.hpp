#pragma once
#include <vector>
#include "Game/Types.hpp"

struct CachedForm {
	NiFixedString editorID;
	char type[4];
};

namespace Cache {
	void Build();
	bool IsBuilt();
	const std::vector<CachedForm>& GetAll();
}
