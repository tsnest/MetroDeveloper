#pragma once
#include "Patcher.h"

#ifndef _WIN64
typedef void* (__cdecl* vfs_ropen_os_LL)(void* result, const char* fn);
typedef char(__cdecl* _method)(void* a1, void** buffer, size_t size);

#else

typedef void* (__fastcall* _vfs_ropen_os)(void* result, const char* fn);
typedef void* (__fastcall* _vfs_ropen_osExodus)(void* result, const char* fn, void* _unused, void* _unknown);

typedef void* (__fastcall* _asset_manager_load_redux)(void* _this, void* str_shared_name, DWORD64 _unknown);
typedef void* (__fastcall* _asset_manager_load_arktika)(void* _this, void* result, const void* str_shared_name, void* handles, void* reload, int set_write_time, int use_ext_path);

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
	static void** g_texture_manager;

	static void* vfs_ropen_package(void* result, const char* fn);
	static void vfs_rbuffered_package(const char* fn, fastdelegate* cb);
	static bool vfs_package_registry_level_downloaded(bool orig_ret, const char* map_name);
	static void textures_bin_process_dir(void* asset_manager, const char* dir);
#else
	static void* vfs_ropen(void* result, const char* fn);
	static void vfs_rbuffered(const char* fn, void* a1, _method method);
#endif
};

