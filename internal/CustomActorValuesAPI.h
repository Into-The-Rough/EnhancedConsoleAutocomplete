#pragma once

#include <cstdint>

class Actor;

enum : std::uint32_t
{
	kCustomActorValuesInterfaceVersion1 = 1,
	kCustomActorValuesInterfaceVersion2 = 2,
};

struct CustomActorValuesInterfaceV1
{
	std::uint32_t interfaceVersion;

	std::uint32_t (__cdecl* ResolveByName)(const char* name);
	const char* (__cdecl* GetName)(std::uint32_t avCode);
	const char* (__cdecl* GetDisplayName)(std::uint32_t avCode);
	float (__cdecl* GetDefaultValue)(std::uint32_t avCode);
	bool (__cdecl* IsCustom)(std::uint32_t avCode);
	float (__cdecl* GetValue)(Actor* actor, std::uint32_t avCode);
	bool (__cdecl* SetValue)(Actor* actor, std::uint32_t avCode, float value);
	bool (__cdecl* ModValue)(Actor* actor, std::uint32_t avCode, float delta);
};

struct CustomActorValuesInterfaceV2
{
	std::uint32_t interfaceVersion;

	std::uint32_t (__cdecl* ResolveByName)(const char* name);
	const char* (__cdecl* GetName)(std::uint32_t avCode);
	const char* (__cdecl* GetDisplayName)(std::uint32_t avCode);
	float (__cdecl* GetDefaultValue)(std::uint32_t avCode);
	bool (__cdecl* IsCustom)(std::uint32_t avCode);
	float (__cdecl* GetValue)(Actor* actor, std::uint32_t avCode);
	bool (__cdecl* SetValue)(Actor* actor, std::uint32_t avCode, float value);
	bool (__cdecl* ModValue)(Actor* actor, std::uint32_t avCode, float delta);
	std::uint32_t (__cdecl* GetCustomCount)();
	std::uint32_t (__cdecl* GetCustomCodeByIndex)(std::uint32_t index);
};

extern "C"
{
	typedef const CustomActorValuesInterfaceV1* (__cdecl* CustomActorValues_GetInterfaceFn)(std::uint32_t version);
}
