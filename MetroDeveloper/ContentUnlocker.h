#pragma once
#include "Patcher.h"

#ifndef _WIN64
typedef char(__cdecl* _method)(void* a1, void** buffer, size_t size);
#else
struct fastdelegate
{
	void* object;
	bool (*method)(void* object, LPVOID& buffer, size_t size);
};
#endif

class ContentUnlocker : public Patcher
{
public:
	ContentUnlocker();

#ifdef _WIN64
	static void* __fastcall vfs_ropen_package(void* result, void* package, const char* fn, const int force_raw, unsigned int* uncompressed_size);
	static void __fastcall vfs_rbuffered_package(const char* fn, fastdelegate* cb);
	static bool __fastcall vfs_package_registry_level_downloaded(bool orig_ret, const char* map_name);
#else
	static void __cdecl vfs_rbuffered(const char* fn, void* a1, _method method);
#endif
};

