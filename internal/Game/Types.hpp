#pragma once
#include <windows.h>
#include <utility>

typedef unsigned char UInt8;
typedef unsigned short UInt16;
typedef unsigned int UInt32;
typedef unsigned long long UInt64;
typedef signed int SInt32;

template <typename T_Ret = void, typename ...Args>
__forceinline T_Ret ThisCall(UInt32 _addr, void* _this, Args ...args) {
	return ((T_Ret(__thiscall*)(void*, Args...))_addr)(_this, std::forward<Args>(args)...);
}

template <typename T_Ret = void, typename ...Args>
__forceinline T_Ret CdeclCall(UInt32 _addr, Args ...args) {
	return ((T_Ret(__cdecl*)(Args...))_addr)(std::forward<Args>(args)...);
}

#define GameHeapAlloc(size) ThisCall<void*>(0xAA3E40, (void*)0x11F6238, size)
#define GameHeapFree(ptr) ThisCall<void>(0xAA4060, (void*)0x11F6238, ptr)

struct String {
	char* m_data;
	UInt16 m_dataLen;
	UInt16 m_bufLen;

	void Set(const char* src) {
		if (!src || !*src) {
			if (m_data) {
				GameHeapFree(m_data);
				m_data = nullptr;
			}
			m_dataLen = m_bufLen = 0;
			return;
		}
		UInt16 len = (UInt16)strlen(src);
		if (m_bufLen < len) {
			m_bufLen = len;
			if (m_data) GameHeapFree(m_data);
			m_data = (char*)GameHeapAlloc(len + 1);
		}
		m_dataLen = len;
		memcpy(m_data, src, len + 1);
	}
};

template<typename TKey, typename TData>
struct NiTMapBase {
	void* vtbl;
	struct Entry {
		Entry* next;
		TKey key;
		TData data;
	};
	UInt32 numBuckets;
	Entry** buckets;
	UInt32 numItems;
};

template<typename T>
struct ListNode {
	T* data;
	ListNode* next;
};
