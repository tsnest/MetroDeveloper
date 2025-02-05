#include "Utils.h"

_GetConsole Utils::GetConsole = nullptr;
GAME Utils::Game;

Utils::Utils()
{
#ifndef _WIN64
	if (GetModuleHandle("metro2033.exe") != NULL) {
		Game = GAME::ORIG2033;
	}
	else if (GetModuleHandle("MetroLL.exe") != NULL) {
		Game = GAME::LL;
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
	}
#endif

#ifndef _WIN64
	if (Game == GAME::ORIG2033)
	{
		// 55 8B EC 83 E4 ? A1 ? ? ? ? 85 C0 56 57 75 ? E8 ? ? ? ? 85 C0 74 ? 8B F8 E8 ? ? ? ? EB ? 33 C0 8B F0 A3 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 5F - ORIG 2033
		Utils::GetConsole = (_GetConsole)FindPatternInEXE(
			(BYTE*)"\x55\x8B\xEC\x83\xE4\x00\xA1\x00\x00\x00\x00\x85\xC0\x56\x57\x75\x00\xE8\x00\x00\x00\x00\x85\xC0\x74\x00\x8B\xF8\xE8\x00\x00\x00\x00\xEB\x00\x33\xC0\x8B\xF0\xA3\x00\x00\x00\x00\xE8\x00\x00\x00\x00\xA1\x00\x00\x00\x00\x5F",
			"xxxxx?x????xxxxx?x????xxx?xxx????x?xxxxx????x????x????x");
	}
	else
	{
		// 55 8B EC 83 E4 ? A1 ? ? ? ? 85 C0 75 ? E8 ? ? ? ? 8B C8 A3 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 8B E5 - Last Light
		Utils::GetConsole = (_GetConsole)FindPatternInEXE(
			(BYTE*)"\x55\x8B\xEC\x83\xE4\x00\xA1\x00\x00\x00\x00\x85\xC0\x75\x00\xE8\x00\x00\x00\x00\x8B\xC8\xA3\x00\x00\x00\x00\xE8\x00\x00\x00\x00\xA1\x00\x00\x00\x00\x8B\xE5",
			"xxxxx?x????xxx?x????xxx????x????x????xx");
	}
#else
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
