#include "Utils.h"

_GetConsole Utils::GetConsole = nullptr;
GAME Utils::Game;
UINT* Utils::engine_time__global_ms;
DWORD64* Utils::g_level;
DWORD64* Utils::g_entities;

Utils::Utils()
{
#ifndef _WIN64
	if (GetModuleHandle("metro2033.exe") != NULL) {
		Game = GAME::ORIG2033;

		// 55 8B EC 83 E4 ? A1 ? ? ? ? 85 C0 56 57 75 ? E8 ? ? ? ? 85 C0 74 ? 8B F8 E8 ? ? ? ? EB ? 33 C0 8B F0 A3 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 5F - ORIG 2033
		Utils::GetConsole = (_GetConsole)FindPatternInEXE(
			(BYTE*)"\x55\x8B\xEC\x83\xE4\x00\xA1\x00\x00\x00\x00\x85\xC0\x56\x57\x75\x00\xE8\x00\x00\x00\x00\x85\xC0\x74\x00\x8B\xF8\xE8\x00\x00\x00\x00\xEB\x00\x33\xC0\x8B\xF0\xA3\x00\x00\x00\x00\xE8\x00\x00\x00\x00\xA1\x00\x00\x00\x00\x5F",
			"xxxxx?x????xxxxx?x????xxx?xxx????x?xxxxx????x????x????x");
	}
	else if (GetModuleHandle("MetroLL.exe") != NULL) {
		Game = GAME::LL;

		// 55 8B EC 83 E4 ? A1 ? ? ? ? 85 C0 75 ? E8 ? ? ? ? 8B C8 A3 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 8B E5 - Last Light
		Utils::GetConsole = (_GetConsole)FindPatternInEXE(
			(BYTE*)"\x55\x8B\xEC\x83\xE4\x00\xA1\x00\x00\x00\x00\x85\xC0\x75\x00\xE8\x00\x00\x00\x00\x8B\xC8\xA3\x00\x00\x00\x00\xE8\x00\x00\x00\x00\xA1\x00\x00\x00\x00\x8B\xE5",
			"xxxxx?x????xxx?x????xxx????x????x????xx");
	}

#else

	if (GetModuleHandle("metro.exe") != NULL) {
		Game = GAME::REDUX;
	}
	else if (GetModuleHandle("arktika1.exe") != NULL) {
		Game = GAME::ARKTIKA;
	}
	else if (GetModuleHandle("MetroExodus.exe") != NULL) {
		Game = GAME::EXODUS;

		// читаем адрес инструкции mov eax, [engine.time._global_ms]
		// 8B 05 ? ? ? ? 48 05 ? ? ? ? 48 3B C1 48 0F 47 C1 89 87 ? ? ? ? 48 85 D2 74 17 F0 0F C1 72 ? 83 FE 01 75 0D 48 8D 8C 24 ? ? ? ? E8 ? ? ? ? 48 8B C7 48 81 C4 - Exodus
		DWORD64 mov = FindPatternInEXE(
			(BYTE*)"\x8B\x05\x00\x00\x00\x00\x48\x05\x00\x00\x00\x00\x48\x3B\xC1\x48\x0F\x47\xC1\x89\x87\x00\x00\x00\x00\x48\x85\xD2\x74\x17\xF0\x0F\xC1\x72\x00\x83\xFE\x01\x75\x0D\x48\x8D\x8C\x24\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8B\xC7\x48\x81\xC4",
			"xx????xx????xxxxxxxxx????xxxxxxxxx?xxxxxxxxx????x????xxxxxx");

		// вычисляем адрес и получаем engine.time._global_ms
		engine_time__global_ms = (UINT*)(mov + 5 + *(DWORD*)(mov + 2));

		///////////////////////////////////////////////////////////////

		// читаем адрес инструкции mov rcx, [g_level]
		// 48 8B 0D ? ? ? ? 48 8B 41 28 48 85 C0 74 20 48 8D 90 ? ? ? ? 48 8B 02 48 85 C0 75 14 48 8B CA E8 ? ? ? ? 48 8B 0D ? ? ? ? EB 03 48 8B C5 48 89 44 24 ? 48 85 C0 74 0B F0 FF 40 08 48 8B 0D
		mov = FindPatternInEXE(
			(BYTE*)"\x48\x8B\x0D\x00\x00\x00\x00\x48\x8B\x41\x28\x48\x85\xC0\x74\x20\x48\x8D\x90\x00\x00\x00\x00\x48\x8B\x02\x48\x85\xC0\x75\x14\x48\x8B\xCA\xE8\x00\x00\x00\x00\x48\x8B\x0D\x00\x00\x00\x00\xEB\x03\x48\x8B\xC5\x48\x89\x44\x24\x00\x48\x85\xC0\x74\x0B\xF0\xFF\x40\x08\x48\x8B\x0D",
			"xxx????xxxxxxxxxxxx????xxxxxxxxxxxx????xxx????xxxxxxxxx?xxxxxxxxxxxx");

		// вычисляем адрес и получаем g_level
		g_level = (DWORD64*)(mov + 7 + *(DWORD*)(mov + 3));

		// читаем адрес инструкции mov rbx, [g_entities]
		// 48 8B 1D ? ? ? ? 48 8B F2 48 C7 C7 ? ? ? ? 33 C0 F0 0F B1 BB ? ? ? ? 74 5D 8B 83 ? ? ? ? 85 C0 74 47 8B 0D ? ? ? ? 03 0D ? ? ? ? 74 0E 48 8D 0D ? ? ? ? E8 ? ? ? ? EB DA
		mov = FindPatternInEXE(
			(BYTE*)"\x48\x8B\x1D\x00\x00\x00\x00\x48\x8B\xF2\x48\xC7\xC7\x00\x00\x00\x00\x33\xC0\xF0\x0F\xB1\xBB\x00\x00\x00\x00\x74\x5D\x8B\x83\x00\x00\x00\x00\x85\xC0\x74\x47\x8B\x0D\x00\x00\x00\x00\x03\x0D\x00\x00\x00\x00\x74\x0E\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\xEB\xDA",
			"xxx????xxxxxx????xxxxxx????xxxx????xxxxxx????xx????xxxxx????x????xx");

		// вычисляем адрес и получаем g_entities
		g_entities = (DWORD64*)(mov + 7 + *(DWORD*)(mov + 3));
	}

	// 48 83 ec ? 48 8b 05 ? ? ? ? 48 85 c0 75 ? e8 ? ? ? ? 48 8b 05 - Redux STEAM
	Utils::GetConsole = (_GetConsole)FindPatternInEXE(
		(BYTE*)"\x48\x83\xec\x00\x48\x8b\x05\x00\x00\x00\x00\x48\x85\xc0\x75\x00\xe8\x00\x00\x00\x00\x48\x8b\x05",
		"xxx?xxx????xxxx?x????xxx");

	if (Utils::GetConsole == NULL) {
		// 48 83 EC 38 48 8B 05 ? ? ? ? 48 85 C0 0F 85 ? ? ? ? 48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 - Redux EGS
		Utils::GetConsole = (_GetConsole)FindPatternInEXE(
			(BYTE*)"\x48\x83\xEC\x38\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x0F\x85\x00\x00\x00\x00\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24",
			"xxxxxxx????xxxxx????xxxx?xxxx?xxxx");
	}
#endif
}

GAME Utils::GetGame()
{
	return Game;
}

#ifdef _WIN64
// Only EXODUS
UINT Utils::GetTimeGlobalMS()
{
	if (Game == GAME::EXODUS) {
		return *engine_time__global_ms;
	} else {
		return 0;
	}
}

DWORD64 Utils::GetGLevel()
{
	if (Game == GAME::EXODUS) {
		return *g_level;
	}
	else {
		return 0;
	}
}

DWORD64 Utils::GetGEntities()
{
	if (Game == GAME::EXODUS) {
		return *g_entities;
	}
	else {
		return 0;
	}
}
#endif

#ifndef _WIN64
bool Utils::isLL()
{
	return Game == GAME::LL;
}
#endif

void Utils::GetString(const char* section_name, const char* str_name, const char* default_str, char* result, DWORD size)
{
	GetPrivateProfileString(section_name, str_name, default_str, result, size, ".\\MetroDeveloper.ini");
}

bool Utils::GetBool(const char* section_name, const char* bool_name, bool default_bool)
{
	string256 str;
	GetString(section_name, bool_name, (default_bool ? "true" : "false"), str, sizeof(str));
	return (strcmp(str, "true") == 0) || (strcmp(str, "yes") == 0) || (strcmp(str, "on") == 0) || (strcmp(str, "1") == 0);
}

bool Utils::FileExists(const char* fn)
{
	DWORD attrs = GetFileAttributes(fn);
	return (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}
