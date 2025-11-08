#pragma once
#include "Patcher.h"
#include "MinHook.h"

class Hooks : public Patcher
{
public:
	Hooks();
	void SetHook(char* hookName, LPVOID pTarget, LPVOID pDetour, LPVOID* ppOriginal);

	static void cmd_register_commands_Hook();
	static void __fastcall slog_Hook(const char* s);

	// test
	static void __fastcall rtexture__init_handle_Hook(void* _this, void* fname);
	static void __fastcall textures__server__get_handle_vrezka_Hook();

#ifndef _WIN64
	static void __fastcall clevel_r_on_key_press_Hook2033(void* _this, void* _unused, int action, int key, int state); // 2033 orig
	static void __fastcall clevel_r_on_key_press_Hook(void* _this, void* _unused, int action, int key, int state, int resending); // LL orig

	static void vfs_ropen_Hook(/*const char* fn*/);
	static void __cdecl vfs_rbuffered_Hook(const char* fn, void* a1, void* method);
#else
	static void __fastcall clevel_r_on_key_press_Hook(void* _this, int action, int key, int state, int resending); // Redux or newer

	static void* __fastcall vfs_ropen_package_Hook(void* result, void* package, const char* fn, const int force_raw, unsigned int* uncompressed_size);
	static void* __fastcall vfs_ropen_package_HookExodus(void* result, void* package, const char* fn, const int force_raw, unsigned int* uncompressed_size, void* _unknown);
	static void* __fastcall vfs_ropen_package_HookExodusOld(void* result, const char* fn, void* _unknown1, void* _unknown2);
	static void __fastcall vfs_rbuffered_package_Hook(void* package, const char* fn, void* cb, const int force_raw);
	static void __fastcall vfs_rbuffered_package_HookExodus(const char* fn, void* cb);
	static bool __fastcall vfs_package_registry_level_downloaded_Hook(void* _this, const char* map_name);
#endif
};

