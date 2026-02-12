#pragma once
#include "Types.hpp"

class NiFixedString {
public:
	char* m_kHandle;

	NiFixedString() : m_kHandle(nullptr) {}

	NiFixedString(const char* str) {
		m_kHandle = str ? CdeclCall<char*>(0xA5B690, str) : nullptr;
	}

	NiFixedString(const NiFixedString& other) {
		if (other.m_kHandle)
			InterlockedIncrement((long*)GetRealBuffer(other.m_kHandle));
		m_kHandle = other.m_kHandle;
	}

	~NiFixedString() {
		if (m_kHandle)
			InterlockedDecrement((long*)GetRealBuffer(m_kHandle));
	}

	NiFixedString& operator=(const NiFixedString& other) {
		if (m_kHandle != other.m_kHandle) {
			if (other.m_kHandle)
				InterlockedIncrement((long*)GetRealBuffer(other.m_kHandle));
			if (m_kHandle)
				InterlockedDecrement((long*)GetRealBuffer(m_kHandle));
			m_kHandle = other.m_kHandle;
		}
		return *this;
	}

	const char* c_str() const { return m_kHandle; }

private:
	static char* GetRealBuffer(char* handle) { return handle - 2 * sizeof(size_t); }
};
