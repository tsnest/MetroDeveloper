#pragma once
#include "Patcher.h"
#include "uconsole.h"
#include <math.h>

#define LODWORD(f) (*((UINT*)&(f)))

typedef void** (__stdcall* _GetConsole)();
typedef char string256[256];

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
	static UINT* engine_time__global_ms;
	static DWORD64* g_level;
	static DWORD64* g_entities;
	static float* slowmo_scale_debug;

	static GAME GetGame();
#ifdef _WIN64
	static UINT GetTimeGlobalMS();
	static DWORD64 GetGLevel();
	static DWORD64 GetGEntities();
	static void slowmo_debug_increase();
	static void slowmo_debug_decrease();
	static void slowmo_debug(float f);
#endif

#ifndef _WIN64
	static bool isLL();
#endif
	static _GetConsole GetConsole;
	static void GetString(const char* section_name, const char* str_name, const char* default_str, char* result, DWORD size);
	static bool GetBool(const char* section_name, const char* bool_name, bool default_bool);
	static bool FileExists(const char* fn);
};

