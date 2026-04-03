#pragma once
#include "Patcher.h"
#include <math.h>
#include <stdio.h>
#include <stdint.h>

#define LODWORD(f) (*((UINT*)&(f)))

typedef void** (__stdcall* _GetConsole)();
typedef void (*_rlog)(const char* format, ...); // In Metro 2033, it returns an int value
typedef char string256[256];

#ifdef _WIN64

typedef void* (__fastcall* _str_container_do_dock)(void* _this, const char* value, unsigned int s_len, unsigned __int16 s_type);

#else

typedef void* (__stdcall* _str_container_do_dock_2033)(const char* value);
typedef void* (__thiscall* _str_container_do_dock_LL)(void* _this, const char* value, __int16 s_type);

#endif

enum GAME
{
#ifndef _WIN64
	ORIG2033,
	LL,
#else
	REDUX,
	ARKTIKA,
	EXODUS
#endif
};

class Utils : public Patcher
{
public:
	Utils();

	static GAME Game;
	static GAME GetGame();

	static _rlog rlog;
	static _GetConsole GetConsole;
	static void** g_string_container;

#ifdef _WIN64
	static bool isRedux();
	static bool isArktika();
	static bool isExodus();

	static bool isReduxEGS;
	static bool isExodusPatched;

	static DWORD64 GetAddrFromRelativeInstr(DWORD64 instr_addr, int instr_len, int rel_offset);

	static DWORD64* g_level;
	static DWORD64* g_game;
	static DWORD64* g_entities;
	static UINT* engine_time__global_ms;
	static float* slowmo_scale_debug;
	static _str_container_do_dock str_container_do_dock;

	static UINT GetTimeGlobalMS();
	static DWORD64 GetGLevel();
	static DWORD64 GetGGame();
	static DWORD64 GetGEntities();
	static void slowmo_debug_increase();
	static void slowmo_debug_decrease();
	static void slowmo_debug(float f);

#else

	static bool is2033();
	static bool isLL();

	static bool is2033Patched;

	static DWORD GetAddrFromRelativeInstr(DWORD instr_addr, int instr_len, int rel_offset);

	static DWORD* g_level;
	static DWORD* g_game;
	static float* delta_f;
	static void* str_container_do_dock;

	static DWORD GetGLevel();
	static DWORD GetGGame();
	static float GetDeltaF();
#endif

	static void* str_shared(const char* str);

	static void GetString(const char* section_name, const char* str_name, const char* default_str, char* result, DWORD size);
	static bool GetBool(const char* section_name, const char* bool_name, bool default_bool);
	static float GetFloat(const char* section_name, const char* param_name, float param_default);
	static bool FileExists(const char* fn);
};

