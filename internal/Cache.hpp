#pragma once
#include <vector>
#include <string>

struct CachedForm {
	std::string editorID;
	char type[4];
};

namespace Cache {
	void Build();
	bool IsBuilt();
	const std::vector<CachedForm>& GetAll();
}
