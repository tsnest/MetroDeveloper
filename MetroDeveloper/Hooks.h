#pragma once
#include "Patcher.h"
#include "MinHook\MinHook.h"

typedef void(__stdcall* _cmd_register_commands)();
typedef void(__thiscall* _slog)(const char* s);
typedef void* (__cdecl* _u_platform_initialize_end)();

#ifdef _WIN64
#include "MinHook\buffer.h"

typedef void* (__fastcall* _vfs_ropen_package)(void* result, void* package, const char* fn, const int force_raw, unsigned int* uncompressed_size);
typedef void* (__fastcall* _vfs_ropen_packageExodus)(void* result, void* package, const char* fn, const int force_raw, unsigned int* uncompressed_size, void* _unknown);
typedef void (__fastcall* _vfs_rbuffered_package)(void* package, const char* fn, void* cb, const int force_raw);
typedef void (__fastcall* _vfs_rbuffered_packageExodus)(const char* fn, void* cb);
typedef bool (__fastcall* _vfs_package_registry_level_downloaded)(void* _this, const char* map_name);
typedef void (__fastcall* _asset_manager_load_all_internal)(void* _this, const int create_progress, DWORD64 _unused, int files);
typedef void* (__fastcall* _vfs_exists)(const char* fn);
typedef void* (__fastcall* _rblock_init)(const char* fn, unsigned int* f_offset, unsigned int* f_size, int not_packaged);
typedef void (__fastcall* _u_platform_initialize_end_Arktika)(void* _this);

#else

typedef void* (__cdecl* _vfs_registry)();
typedef void* (__thiscall* _vfs_package_registry_find)(void* _this, int layer, void* current, const char* path, int nearest /*, void* _layer*/); // последний аргумент появился в патчах LL
typedef bool (__cdecl* _vfs_exists)(const char* fn);
typedef void (__cdecl* _vfs_rbuffered)(const char* fn, void* a1, void* method);
#endif

class Hooks : public Patcher
{
public:
	Hooks();
	static void SetHook(char* hookName, void* pTarget, void* pDetour, void* ppOriginal);

	static void cmd_register_commands_Hook();
	static void __fastcall slog_Hook(const char* s);
	static void vfs_ropen_cantfind(const char* fn);
	static void* __cdecl u_platform_initialize_end_Hook();

#ifndef _WIN64
	static void __fastcall clevel_r_on_key_press_Hook2033(void* _this, void* _unused, int action, int key, int state); // 2033 orig
	static void __fastcall clevel_r_on_key_press_HookLL(void* _this, void* _unused, int action, int key, int state, int resending); // LL orig

	static void vfs_ropen_HookASM();
	static void* __cdecl vfs_ropen_HookC(void* result, const char* fn);
	static void __cdecl vfs_rbuffered_Hook(const char* fn, void* a1, void* method);
	static void chud_item__update_hud_position_EndFuncHook(void);
	static bool __cdecl vfs_exists_custom_ll(const char* fn);

#else

	static void __fastcall clevel_r_on_key_press_Hook(void* _this, int action, int key, int state, int resending); // Redux or newer

	static void* __fastcall vfs_ropen_package_Hook(void* result, void* package, const char* fn, const int force_raw, unsigned int* uncompressed_size);
	static void* __fastcall vfs_ropen_package_ExodusPatched_Hook(void* result, void* package, const char* fn, const int force_raw, unsigned int* uncompressed_size, void* _unknown);
	static void __fastcall vfs_rbuffered_package_Hook(void* package, const char* fn, void* cb, const int force_raw);
	static void __fastcall vfs_rbuffered_package_HookExodus(const char* fn, void* cb);
	static bool __fastcall vfs_package_registry_level_downloaded_Hook(void* _this, const char* map_name);
	static void __fastcall asset_manager_load_all_internal_Hook(void* _this, const int create_progress, DWORD64 _unused, int files);
	static void __fastcall u_platform_initialize_end_HookArktika(void* _this);
#endif
};

