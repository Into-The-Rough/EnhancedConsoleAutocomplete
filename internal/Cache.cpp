#include "Cache.hpp"
#include "Game/Types.hpp"
#include <windows.h>

namespace Cache {

#pragma pack(push, 1)
struct RecordHeader { char type[4]; UInt32 dataSize, flags, formID, timestamp; UInt16 vci, fv; };
struct SubrecordHeader { char type[4]; UInt16 dataSize; };
struct GroupHeader { char type[4]; UInt32 groupSize, label, groupType, timestamp, unknown; };
#pragma pack(pop)

static std::vector<CachedForm> g_Forms;
static bool g_Built = false;

static void ParsePlugin(const char* path) {
	HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE) return;

	DWORD fileSize = GetFileSize(hFile, nullptr);
	if (fileSize == INVALID_FILE_SIZE || fileSize < sizeof(RecordHeader)) {
		CloseHandle(hFile);
		return;
	}

	HANDLE hMap = CreateFileMappingA(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
	if (!hMap) { CloseHandle(hFile); return; }

	const UInt8* data = (const UInt8*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
	if (!data) { CloseHandle(hMap); CloseHandle(hFile); return; }

	size_t pos = 0;
	if (pos + sizeof(RecordHeader) <= fileSize) {
		auto* tes4 = (const RecordHeader*)(data + pos);
		if (memcmp(tes4->type, "TES4", 4) == 0)
			pos += sizeof(RecordHeader) + tes4->dataSize;
	}

	while (pos + 4 <= fileSize) {
		if (memcmp(data + pos, "GRUP", 4) == 0) {
			if (pos + sizeof(GroupHeader) > fileSize) break;
			pos += sizeof(GroupHeader);
			continue;
		}

		if (pos + sizeof(RecordHeader) > fileSize) break;
		auto* rec = (const RecordHeader*)(data + pos);
		size_t dataStart = pos + sizeof(RecordHeader);
		size_t dataEnd = dataStart + rec->dataSize;

		if (rec->dataSize > fileSize - pos - sizeof(RecordHeader)) break;

		//extract EDID from uncompressed records
		if (!(rec->flags & 0x00040000) && dataEnd <= fileSize) {
			size_t subPos = dataStart;
			while (subPos + sizeof(SubrecordHeader) < dataEnd) {
				auto* sub = (const SubrecordHeader*)(data + subPos);
				size_t subDataStart = subPos + sizeof(SubrecordHeader);
				if (subDataStart + sub->dataSize > dataEnd) break;

				if (memcmp(sub->type, "EDID", 4) == 0 && sub->dataSize > 0) {
					std::string edid((const char*)(data + subDataStart), sub->dataSize - 1);
					if (!edid.empty()) {
						CachedForm cf;
						cf.editorID = std::move(edid);
						memcpy(cf.type, rec->type, 4);
						g_Forms.push_back(std::move(cf));
					}
					break;
				}
				subPos = subDataStart + sub->dataSize;
			}
		}

		pos += sizeof(RecordHeader) + rec->dataSize;
	}

	UnmapViewOfFile(data);
	CloseHandle(hMap);
	CloseHandle(hFile);
}

void Build() {
	if (g_Built) return;

	char dataPath[MAX_PATH];
	GetModuleFileNameA(nullptr, dataPath, MAX_PATH);
	if (char* p = strrchr(dataPath, '\\')) *p = '\0';
	strcat_s(dataPath, "\\Data\\");

	WIN32_FIND_DATAA fd;
	char searchPath[MAX_PATH];

	for (const char* ext : { "*.esm", "*.esp" }) {
		sprintf_s(searchPath, "%s%s", dataPath, ext);
		HANDLE hFind = FindFirstFileA(searchPath, &fd);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				char fullPath[MAX_PATH];
				sprintf_s(fullPath, "%s%s", dataPath, fd.cFileName);
				ParsePlugin(fullPath);
			} while (FindNextFileA(hFind, &fd));
			FindClose(hFind);
		}
	}

	g_Built = true;
}

bool IsBuilt() { return g_Built; }

const std::vector<CachedForm>& GetAll() { return g_Forms; }

}
