#include "Utils.h"

bool Utils::isInited = false;
_rlog Utils::rlog = nullptr;
_GetConsole Utils::GetConsole = nullptr;
void** Utils::g_string_container = nullptr;

GAME Utils::Game;

#ifdef _WIN64

bool Utils::isReduxEGS = false;
bool Utils::isExodusPatched = false;

DWORD64* Utils::g_level;
DWORD64* Utils::g_game;
DWORD64* Utils::g_entities;
UINT* Utils::engine_time__global_ms;
float* Utils::slowmo_scale_debug;
_str_container_do_dock Utils::str_container_do_dock = nullptr;
DWORD64 Utils::igame_level_signal = NULL;
_payload_exodus Utils::payload_exodus = nullptr;

#else

bool Utils::is2033Patched = false;

DWORD* Utils::g_level;
DWORD* Utils::g_game;
float* Utils::delta_f;
void* Utils::str_container_do_dock = nullptr;
DWORD Utils::igame_level_signal = nullptr;

#endif

Utils::Utils()
{
#ifndef _WIN64
	if (GetModuleHandle("metro2033.exe") != NULL) {
		Game = GAME::ORIG2033;

		// 81 EC ? ? ? ? 8B 8C 24 ? ? ? ? 53 56 8D 84 24 ? ? ? ? 50 51 8D 54 24 40 68 ? ? ? ? 52 FF 15 ? ? ? ? 33 DB 83 C4 10 3B C3 88 9C 24 ? ? ? ? 7D 7C 8B 8C 24 - ORIG 2033
		Utils::rlog = (_rlog)FindPatternInEXE(
			"\x81\xEC\x00\x00\x00\x00\x8B\x8C\x24\x00\x00\x00\x00\x53\x56\x8D\x84\x24\x00\x00\x00\x00\x50\x51\x8D\x54\x24\x40\x68\x00\x00\x00\x00\x52\xFF\x15\x00\x00\x00\x00\x33\xDB\x83\xC4\x10\x3B\xC3\x88\x9C\x24\x00\x00\x00\x00\x7D\x7C\x8B\x8C\x24",
			"xx????xxx????xxxxx????xxxxxxx????xxx????xxxxxxxxxx????xxxxx");

		if (Utils::rlog != NULL) {
			isInited = true;

			// 55 8B EC 83 E4 ? A1 ? ? ? ? 85 C0 56 57 75 ? E8 ? ? ? ? 85 C0 74 ? 8B F8 E8 ? ? ? ? EB ? 33 C0 8B F0 A3 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 5F - ORIG 2033
			Utils::GetConsole = (_GetConsole)FindPatternInEXE(
				"\x55\x8B\xEC\x83\xE4\x00\xA1\x00\x00\x00\x00\x85\xC0\x56\x57\x75\x00\xE8\x00\x00\x00\x00\x85\xC0\x74\x00\x8B\xF8\xE8\x00\x00\x00\x00\xEB\x00\x33\xC0\x8B\xF0\xA3\x00\x00\x00\x00\xE8\x00\x00\x00\x00\xA1\x00\x00\x00\x00\x5F",
				"xxxxx?x????xxxxx?x????xxx?xxx????x?xxxxx????x????x????x");

			// A1 ? ? ? ? 85 C0 0F 84 ? ? ? ? 8B 48 14 85 C9 0F 84 - ORIG 2033
			DWORD mov_g_level = FindPatternInEXE("\xA1\x00\x00\x00\x00\x85\xC0\x0F\x84\x00\x00\x00\x00\x8B\x48\x14\x85\xC9\x0F\x84", "x????xxxx????xxxxxxx");
			// вычисняем адрес и получаем g_level
			g_level = *(DWORD**)(mov_g_level + 1);

			// D9 05 ? ? ? ? 8B 11 8B 42 28 - ORIG 2033
			DWORD fld_delta_f = FindPatternInEXE("\xD9\x05\x00\x00\x00\x00\x8B\x11\x8B\x42\x28", "xx????xxxxx");
			delta_f = *(float**)(fld_delta_f + 2);

			// 83 EC 14 53 55 56 8B 35 84 0D A1 00 57 8B 7C 24 28 85 FF - ORIG 2033
			Utils::str_container_do_dock = (void*)FindPatternInEXE(
				"\x83\xEC\x14\x53\x55\x56\x8B\x35\x84\x0D\xA1\x00\x57\x8B\x7C\x24\x28\x85\xFF",
				"xx?xxxxx????xxxx?xx"
			);
		}
	}
	else if (GetModuleHandle("MetroLL.exe") != NULL) {
		Game = GAME::LL;

		// 55 8B EC 83 E4 F8 81 EC ? ? ? ? 8B 4D 08 56 57 8D 45 0C 50 51 8D 54 24 38 68 ? ? ? ? 52 FF 15 - Last Light
		Utils::rlog = (_rlog)FindPatternInEXE(
			"\x55\x8B\xEC\x83\xE4\xF8\x81\xEC\x00\x00\x00\x00\x8B\x4D\x08\x56\x57\x8D\x45\x0C\x50\x51\x8D\x54\x24\x38\x68\x00\x00\x00\x00\x52\xFF\x15",
			"xxxxxxxx????xxxxxxxxxxxxxxx????xxx");

		if (Utils::rlog != NULL) {
			isInited = true;

			// 55 8B EC 83 E4 ? A1 ? ? ? ? 85 C0 75 ? E8 ? ? ? ? 8B C8 A3 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 8B E5 - Last Light
			Utils::GetConsole = (_GetConsole)FindPatternInEXE(
				"\x55\x8B\xEC\x83\xE4\x00\xA1\x00\x00\x00\x00\x85\xC0\x75\x00\xE8\x00\x00\x00\x00\x8B\xC8\xA3\x00\x00\x00\x00\xE8\x00\x00\x00\x00\xA1\x00\x00\x00\x00\x8B\xE5",
				"xxxxx?x????xxx?x????xxx????x????x????xx");

			// читаем адрес инструкции mov eax, g_game
			// A1 ? ? ? ? 0F 57 C0 56 - Last Light
			DWORD mov_g_game = FindPatternInEXE(
				"\xA1\x00\x00\x00\x00\x0F\x57\xC0\x56",
				"x????xxxx");

			// вычисняем адрес и получаем g_game
			g_game = *(DWORD**)(mov_g_game + 1);

			// 83 EC 14 53 55 56 8B F1 8B 4C 24 24 57 85 C9 75 0C 33 C0 5F 5E 5D 5B 83 C4 14 C2 08 00 - Last Light
			Utils::str_container_do_dock = (void*)FindPatternInEXE(
				"\x83\xEC\x14\x53\x55\x56\x8B\xF1\x8B\x4C\x24\x24\x57\x85\xC9\x75\x0C\x33\xC0\x5F\x5E\x5D\x5B\x83\xC4\x14\xC2\x08\x00",
				"xxxxxxxxxxxxxxxxxxxxxxxxxxxxx");

			// 8B 0D ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 8B 0E 85 C9 74 10 83 C1 04 83 CA FF F0 0F C1 11 C7 06 ? ? ? ? 8B 4C 24 08 - Last Light
			DWORD mov_g_string_container = FindPatternInEXE(
				"\x8B\x0D\x00\x00\x00\x00\x68\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x8B\x0E\x85\xC9\x74\x10\x83\xC1\x04\x83\xCA\xFF\xF0\x0F\xC1\x11\xC7\x06\x00\x00\x00\x00\x8B\x4C\x24\x08",
				"xx????x????x????xxxxxxxxxxxxxxxxxx????xxxx");
			g_string_container = *(void***)(mov_g_string_container + 2);
		}
	}

#else

	DWORD64 mov_g_game = NULL;
	DWORD64 mov_g_level = NULL;

	if (GetModuleHandle("metro.exe") != NULL) {
		Game = GAME::REDUX;

		// 48 8B C4 48 89 48 08 48 89 50 10 4C 89 40 18 4C 89 48 20 48 81 EC ? ? ? ? 4C 8B C1 4C 8D 48 10 48 8D 4C 24 ? BA ? ? ? ? FF 15 ? ? ? ? 33 C9 C6 84 24 ? ? ? ? ? 48 89 4C 24 ? 85 C0 0F 89 ? ? ? ? 48 8B 94 24 ? ? ? ? 48 89 4C 24 - Redux
		Utils::rlog = (_rlog)FindPatternInEXE(
			"\x48\x8B\xC4\x48\x89\x48\x08\x48\x89\x50\x10\x4C\x89\x40\x18\x4C\x89\x48\x20\x48\x81\xEC\x00\x00\x00\x00\x4C\x8B\xC1\x4C\x8D\x48\x10\x48\x8D\x4C\x24\x00\xBA\x00\x00\x00\x00\xFF\x15\x00\x00\x00\x00\x33\xC9\xC6\x84\x24\x00\x00\x00\x00\x00\x48\x89\x4C\x24\x00\x85\xC0\x0F\x89\x00\x00\x00\x00\x48\x8B\x94\x24\x00\x00\x00\x00\x48\x89\x4C\x24",
			"xxxxxxxxxxxxxxxxxxxxxx????xxxxxxxxxxx?x????xx????xxxxx?????xxxx?xxxx????xxxx????xxxx");

		if (Utils::rlog != NULL) {
			isInited = true;

			// 48 8B 05 ? ? ? ? 4C 8D 85 ? ? ? ? 48 8D 95 ? ? ? ? 48 8B 58 10 48 8D 4C 24 ? E8 ? ? ? ? 48 8B CB 48 89 7C 24 ? 0F 57 DB 0F 57 D2 48 8B D0 E8 ? ? ? ? 48 8D 8D ? ? ? ? E9 - Redux STEAM
			mov_g_game = FindPatternInEXE(
				"\x48\x8B\x05\x00\x00\x00\x00\x4C\x8D\x85\x00\x00\x00\x00\x48\x8D\x95\x00\x00\x00\x00\x48\x8B\x58\x10\x48\x8D\x4C\x24\x00\xE8\x00\x00\x00\x00\x48\x8B\xCB\x48\x89\x7C\x24\x00\x0F\x57\xDB\x0F\x57\xD2\x48\x8B\xD0\xE8\x00\x00\x00\x00\x48\x8D\x8D\x00\x00\x00\x00\xE9",
				"xxx????xxx????xxx????xxxxxxxx?x????xxxxxxx?xxxxxxxxxx????xxx????x");

			DWORD64 mov_g_string_container = NULL;

			if (mov_g_game != NULL) {
				// 48 8B 0D ? ? ? ? 48 8D 15 ? ? ? ? 88 81 ? ? ? ? 48 85 D2 74 16 48 8B 0D ? ? ? ? 45 33 C9 45 8D 41 0E E8 ? ? ? ? 48 8B F8 48 8D 55 0F - Redux STEAM
				mov_g_level = FindPatternInEXE(
					"\x48\x8B\x0D\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00\x88\x81\x00\x00\x00\x00\x48\x85\xD2\x74\x16\x48\x8B\x0D\x00\x00\x00\x00\x45\x33\xC9\x45\x8D\x41\x0E\xE8\x00\x00\x00\x00\x48\x8B\xF8\x48\x8D\x55\x0F",
					"xxx????xxx????xx????xxxxxxxx????xxxxxxxx????xxxxxxx");

				// 48 8B 0D ? ? ? ? 48 8D 54 24 ? 45 33 C9 44 8B C0 E8 ? ? ? ? 48 8B 4F 08 48 85 C9 74 04 F0 FF 49 08 - Redux STEAM
				mov_g_string_container = FindPatternInEXE(
					"\x48\x8B\x0D\x00\x00\x00\x00\x48\x8D\x54\x24\x00\x45\x33\xC9\x44\x8B\xC0\xE8\x00\x00\x00\x00\x48\x8B\x4F\x08\x48\x85\xC9\x74\x04\xF0\xFF\x49\x08",
					"xxx????xxxx?xxxxxxx????xxxxxxxxxxxxx");

				// 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC 30 48 8B 2D ? ? ? ? 45 8B F1 4D 8B F8 48 8B FA FF 15 - Redux STEAM
				igame_level_signal = FindPatternInEXE(
					"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x41\x56\x41\x57\x48\x83\xEC\x30\x48\x8B\x2D\x00\x00\x00\x00\x45\x8B\xF1\x4D\x8B\xF8\x48\x8B\xFA\xFF\x15",
					"xxxx?xxxx?xxxx?xxxxxxxxxxxx????xxxxxxxxxxx");
			} else {
				isReduxEGS = true;

				// 48 8B 05 ? ? ? ? 4C 8D 85 ? ? ? ? 48 8D 95 ? ? ? ? 48 8D 4C 24 ? 48 8B 58 10 E8 ? ? ? ? 48 8B D0 48 89 7C 24 ? 0F 57 DB 0F 57 D2 48 8B CB E8 ? ? ? ? B8 ? ? ? ? E9 ? ? ? ? 48 8B 05 ? ? ? ? 48 85 C0 0F 84 ? ? ? ? 48 8B 88 - Redux EGS
				mov_g_game = FindPatternInEXE(
					"\x48\x8B\x05\x00\x00\x00\x00\x4C\x8D\x85\x00\x00\x00\x00\x48\x8D\x95\x00\x00\x00\x00\x48\x8D\x4C\x24\x00\x48\x8B\x58\x10\xE8\x00\x00\x00\x00\x48\x8B\xD0\x48\x89\x7C\x24\x00\x0F\x57\xDB\x0F\x57\xD2\x48\x8B\xCB\xE8\x00\x00\x00\x00\xB8\x00\x00\x00\x00\xE9\x00\x00\x00\x00\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x0F\x84\x00\x00\x00\x00\x48\x8B\x88",
					"xxx????xxx????xxx????xxxx?xxxxx????xxxxxxx?xxxxxxxxxx????x????x????xxx????xxxxx????xxx");

				// 48 8B 0D ? ? ? ? 44 38 7C 24 ? 75 0E 48 8B 41 50 F0 FF 40 08 48 8B 41 50 EB 1E 48 8D 44 24 ? 48 FF C6 - Redux EGS
				mov_g_string_container = FindPatternInEXE(
					"\x48\x8B\x0D\x00\x00\x00\x00\x44\x38\x7C\x24\x00\x75\x0E\x48\x8B\x41\x50\xF0\xFF\x40\x08\x48\x8B\x41\x50\xEB\x1E\x48\x8D\x44\x24\x00\x48\xFF\xC6",
					"xxx????xxxx?xxxxxxxxxxxxxxxxxxxx?xxx");

				// 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 56 48 83 EC 30 48 8B 3D ? ? ? ? 41 8B E9 4D 8B F0 48 8B F2 FF 15 - Redux EGS
				igame_level_signal = FindPatternInEXE(
					"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x41\x56\x48\x83\xEC\x30\x48\x8B\x3D\x00\x00\x00\x00\x41\x8B\xE9\x4D\x8B\xF0\x48\x8B\xF2\xFF\x15",
					"xxxx?xxxx?xxxx?xxxx?xxxxxxxxx????xxxxxxxxxxx");
			}

			// 48 89 5c 24 ? 48 89 6c 24 ? 48 89 74 24 ? 57 41 54 41 55 41 56 41 57 48 83 ec ? 48 8b 41 ? 45 0f b7 e1 - Redux STEAM & EGS
			Utils::str_container_do_dock = (_str_container_do_dock)FindPatternInEXE(
				"\x48\x89\x5c\x24\x00\x48\x89\x6c\x24\x00\x48\x89\x74\x24\x00\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x83\xec\x00\x48\x8b\x41\x00\x45\x0f\xb7\xe1",
				"xxxx?xxxx?xxxx?xxxxxxxxxxxx?xxx?xxxx");

			g_string_container = (void**)GetAddrFromRelativeInstr(mov_g_string_container, 7, 3);
		}
	}
	else if (GetModuleHandle("arktika1.exe") != NULL) {
		Game = GAME::ARKTIKA;

		// 48 8B C4 48 89 48 08 48 89 50 10 4C 89 40 18 4C 89 48 20 53 57 48 81 EC ? ? ? ? 48 8B D9 48 8D 78 10 E8 ? ? ? ? 48 89 7C 24 ? 48 8D 54 24 ? 33 FF 4C 8B CB 41 B8 ? ? ? ? 48 89 7C 24 - Arktika
		Utils::rlog = (_rlog)FindPatternInEXE(
			"\x48\x8B\xC4\x48\x89\x48\x08\x48\x89\x50\x10\x4C\x89\x40\x18\x4C\x89\x48\x20\x53\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xD9\x48\x8D\x78\x10\xE8\x00\x00\x00\x00\x48\x89\x7C\x24\x00\x48\x8D\x54\x24\x00\x33\xFF\x4C\x8B\xCB\x41\xB8\x00\x00\x00\x00\x48\x89\x7C\x24",
			"xxxxxxxxxxxxxxxxxxxxxxxx????xxxxxxxx????xxxx?xxxx?xxxxxxx????xxxx");

		if (Utils::rlog != NULL) {
			isInited = true;

			// 48 8B 05 ? ? ? ? 4C 8D 87 ? ? ? ? 48 8D 97 ? ? ? ? 48 8D 4C 24 ? 48 8B 58 10 E8 ? ? ? ? 48 8B D0 48 89 74 24 ? 0F 57 DB 0F 57 D2 48 8B CB E8 ? ? ? ? 48 8D 8F ? ? ? ? EB 07 48 8D 8F ? ? ? ? E8 - Arktika
			mov_g_game = FindPatternInEXE(
				"\x48\x8B\x05\x00\x00\x00\x00\x4C\x8D\x87\x00\x00\x00\x00\x48\x8D\x97\x00\x00\x00\x00\x48\x8D\x4C\x24\x00\x48\x8B\x58\x10\xE8\x00\x00\x00\x00\x48\x8B\xD0\x48\x89\x74\x24\x00\x0F\x57\xDB\x0F\x57\xD2\x48\x8B\xCB\xE8\x00\x00\x00\x00\x48\x8D\x8F\x00\x00\x00\x00\xEB\x07\x48\x8D\x8F\x00\x00\x00\x00\xE8",
				"xxx????xxx????xxx????xxxx?xxxxx????xxxxxxx?xxxxxxxxxx????xxx????xxxxx????x");

			// 48 89 5C 24 ? 48 89 4C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 30 83 3D ? ? ? ? ? 45 0F B7 E9 - Arktika
			Utils::str_container_do_dock = (_str_container_do_dock)FindPatternInEXE(
				"\x48\x89\x5C\x24\x00\x48\x89\x4C\x24\x00\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x83\xEC\x30\x83\x3D\x00\x00\x00\x00\x00\x45\x0F\xB7\xE9",
				"xxxx?xxxx?xxxxxxxxxxxxxxxxx?????xxxx");

			// 48 8B 0D ? ? ? ? 40 38 6C 24 ? 75 14 48 8B 81 ? ? ? ? F0 FF 40 08 48 8B 81 ? ? ? ? EB 23 48 8D 54 24 ? 48 83 C8 FF 90 - Arktika
			DWORD64 mov_g_string_container = FindPatternInEXE(
				"\x48\x8B\x0D\x00\x00\x00\x00\x40\x38\x6C\x24\x00\x75\x14\x48\x8B\x81\x00\x00\x00\x00\xF0\xFF\x40\x08\x48\x8B\x81\x00\x00\x00\x00\xEB\x23\x48\x8D\x54\x24\x00\x48\x83\xC8\xFF\x90",
				"xxx????xxxx?xxxxx????xxxxxxx????xxxxxx?xxxxx");

			g_string_container = (void**)GetAddrFromRelativeInstr(mov_g_string_container, 7, 3);

			// 48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 56 48 83 EC 30 C7 44 24 ? ? ? ? ? 41 8B F1 4C 8B 35 ? ? ? ? 49 8B E8 48 8B FA FF 15 - Arktika
			igame_level_signal = FindPatternInEXE(
				"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x56\x57\x41\x56\x48\x83\xEC\x30\xC7\x44\x24\x00\x00\x00\x00\x00\x41\x8B\xF1\x4C\x8B\x35\x00\x00\x00\x00\x49\x8B\xE8\x48\x8B\xFA\xFF\x15",
				"xxxx?xxxx?xxxxxxxxxxx?????xxxxxx????xxxxxxxx");
		}
	}
	else if (GetModuleHandle("MetroExodus.exe") != NULL) {
		Game = GAME::EXODUS;

		DWORD64 call_igame_level_signal;
		DWORD64 call_payload_exodus;

		// 48 8B C4 48 89 48 08 48 89 50 10 4C 89 40 18 4C 89 48 20 53 57 48 81 EC ? ? ? ? 48 8B D9 C6 44 24 ? ? 48 8D 78 10 E8 ? ? ? ? 48 89 7C 24 ? 48 8D 54 24 - Exodus NEW
		Utils::rlog = (_rlog)FindPatternInEXE(
			"\x48\x8B\xC4\x48\x89\x48\x08\x48\x89\x50\x10\x4C\x89\x40\x18\x4C\x89\x48\x20\x53\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xD9\xC6\x44\x24\x00\x00\x48\x8D\x78\x10\xE8\x00\x00\x00\x00\x48\x89\x7C\x24\x00\x48\x8D\x54\x24",
			"xxxxxxxxxxxxxxxxxxxxxxxx????xxxxxx??xxxxx????xxxx?xxxx");

		if (Utils::rlog == NULL) {
			// 48 89 E0 48 89 48 08 48 89 50 10 4C 89 40 18 4C 89 48 20 53 57 48 81 EC ? ? ? ? 48 89 CB C6 44 24 ? ? 48 8D 78 10 E8 - Exodus OLD
			Utils::rlog = (_rlog)FindPatternInEXE(
				"\x48\x89\xE0\x48\x89\x48\x08\x48\x89\x50\x10\x4C\x89\x40\x18\x4C\x89\x48\x20\x53\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x89\xCB\xC6\x44\x24\x00\x00\x48\x8D\x78\x10\xE8",
				"xxxxxxxxxxxxxxxxxxxxxxxx????xxxxxx??xxxxx");
		} else {
			isExodusPatched = true;
		}

		if (Utils::rlog != NULL) {
			isInited = true;

			// 48 89 5C 24 ? 66 44 89 4C 24 ? 48 89 4C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 30 83 3D - Exodus ALL
			Utils::str_container_do_dock = (_str_container_do_dock)FindPatternInEXE(
				"\x48\x89\x5C\x24\x00\x66\x44\x89\x4C\x24\x00\x48\x89\x4C\x24\x00\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x83\xEC\x30\x83\x3D",
				"xxxx?xxxxx?xxxx?xxxxxxxxxxxxxxxxx");

			// 48 8B 05 ? ? ? ? 4C 8D 87 ? ? ? ? 48 8D 97 ? ? ? ? 48 8D 4C 24 ? 48 8B 58 10 E8 ? ? ? ? 48 8B D0 48 89 74 24 ? 0F 57 DB 0F 57 D2 48 8B CB E8 ? ? ? ? B8 ? ? ? ? EB 05 - Exodus ALL
			mov_g_game = FindPatternInEXE(
				"\x48\x8B\x05\x00\x00\x00\x00\x4C\x8D\x87\x00\x00\x00\x00\x48\x8D\x97\x00\x00\x00\x00\x48\x8D\x4C\x24\x00\x48\x8B\x58\x10\xE8\x00\x00\x00\x00\x48\x8B\xD0\x48\x89\x74\x24\x00\x0F\x57\xDB\x0F\x57\xD2\x48\x8B\xCB\xE8\x00\x00\x00\x00\xB8\x00\x00\x00\x00\xEB\x05",
				"xxx????xxx????xxx????xxxx?xxxxx????xxxxxxx?xxxxxxxxxx????x????xx");

			DWORD64 mov_g_string_container;

			if (isExodusPatched) {
				// 48 8B 0D ? ? ? ? 48 8D 15 ? ? ? ? 45 33 C9 45 8D 41 05 E8 ? ? ? ? 44 8B C0 4C 8D 0D ? ? ? ? 85 C0 74 19 - Exodus NEW
				mov_g_string_container = FindPatternInEXE(
					"\x48\x8B\x0D\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00\x45\x33\xC9\x45\x8D\x41\x05\xE8\x00\x00\x00\x00\x44\x8B\xC0\x4C\x8D\x0D\x00\x00\x00\x00\x85\xC0\x74\x19",
					"xxx????xxx????xxxxxxxx????xxxxxx????xxxx");

				// читаем адрес инструкции mov eax, [engine.time._global_ms]
				// 8B 05 ? ? ? ? 48 05 ? ? ? ? 48 3B C1 48 0F 47 C1 89 87 ? ? ? ? 48 85 D2 74 17 F0 0F C1 72 ? 83 FE 01 75 0D 48 8D 8C 24 ? ? ? ? E8 ? ? ? ? 48 8B C7 48 81 C4 - Exodus NEW
				DWORD64 mov = FindPatternInEXE(
					"\x8B\x05\x00\x00\x00\x00\x48\x05\x00\x00\x00\x00\x48\x3B\xC1\x48\x0F\x47\xC1\x89\x87\x00\x00\x00\x00\x48\x85\xD2\x74\x17\xF0\x0F\xC1\x72\x00\x83\xFE\x01\x75\x0D\x48\x8D\x8C\x24\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8B\xC7\x48\x81\xC4",
					"xx????xx????xxxxxxxxx????xxxxxxxxx?xxxxxxxxx????x????xxxxxx");

				// вычисляем адрес и получаем engine.time._global_ms
				engine_time__global_ms = (UINT*)GetAddrFromRelativeInstr(mov, 5, 2);

				///////////////////////////////////////////////////////////////

				// читаем адрес инструкции mov rcx, [g_level]
				// 48 8B 0D ? ? ? ? 48 8B 41 28 48 85 C0 74 20 48 8D 90 ? ? ? ? 48 8B 02 48 85 C0 75 14 48 8B CA E8 ? ? ? ? 48 8B 0D ? ? ? ? EB 03 48 8B C5 48 89 44 24 ? 48 85 C0 74 0B F0 FF 40 08 48 8B 0D - Exodus NEW
				mov_g_level = FindPatternInEXE(
					"\x48\x8B\x0D\x00\x00\x00\x00\x48\x8B\x41\x28\x48\x85\xC0\x74\x20\x48\x8D\x90\x00\x00\x00\x00\x48\x8B\x02\x48\x85\xC0\x75\x14\x48\x8B\xCA\xE8\x00\x00\x00\x00\x48\x8B\x0D\x00\x00\x00\x00\xEB\x03\x48\x8B\xC5\x48\x89\x44\x24\x00\x48\x85\xC0\x74\x0B\xF0\xFF\x40\x08\x48\x8B\x0D",
					"xxx????xxxxxxxxxxxx????xxxxxxxxxxxx????xxx????xxxxxxxxx?xxxxxxxxxxxx");

				// читаем адрес инструкции mov rbx, [g_entities]
				// 48 8B 1D ? ? ? ? 48 8B F2 48 C7 C7 ? ? ? ? 33 C0 F0 0F B1 BB ? ? ? ? 74 5D 8B 83 ? ? ? ? 85 C0 74 47 8B 0D ? ? ? ? 03 0D ? ? ? ? 74 0E 48 8D 0D ? ? ? ? E8 ? ? ? ? EB DA - Exodus NEW
				mov = FindPatternInEXE(
					"\x48\x8B\x1D\x00\x00\x00\x00\x48\x8B\xF2\x48\xC7\xC7\x00\x00\x00\x00\x33\xC0\xF0\x0F\xB1\xBB\x00\x00\x00\x00\x74\x5D\x8B\x83\x00\x00\x00\x00\x85\xC0\x74\x47\x8B\x0D\x00\x00\x00\x00\x03\x0D\x00\x00\x00\x00\x74\x0E\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\xEB\xDA",
					"xxx????xxxxxx????xxxxxx????xxxx????xxxxxx????xx????xxxxx????x????xx");

				// вычисляем адрес и получаем g_entities
				g_entities = (DWORD64*)GetAddrFromRelativeInstr(mov, 7, 3);

				// читаем адрес инструкции mulss xmm2, cs:slowmo_scale_debug
				// F3 0F 59 15 ? ? ? ? F3 0F 59 15 ? ? ? ? F3 0F 59 D0 F3 0F 58 15 ? ? ? ? 0F 28 F2 - Exodus NEW
				DWORD64 mulss = FindPatternInEXE(
					"\xF3\x0F\x59\x15\x00\x00\x00\x00\xF3\x0F\x59\x15\x00\x00\x00\x00\xF3\x0F\x59\xD0\xF3\x0F\x58\x15\x00\x00\x00\x00\x0F\x28\xF2",
					"xxxx????xxxx????xxxxxxxx????xxx");

				// вычисляем адрес и получаем slowmo_scale_debug
				slowmo_scale_debug = (float*)GetAddrFromRelativeInstr(mulss, 8, 4);

				// E8 ? ? ? ? 40 88 35 - Exodus NEW
				call_igame_level_signal = FindPatternInEXE("\xE8\x00\x00\x00\x00\x40\x88\x35", "x????xxx");

				// E8 ? ? ? ? 4C 8B 38 - Exodus NEW
				call_payload_exodus = FindPatternInEXE("\xE8\x00\x00\x00\x00\x4C\x8B\x38", "x????xxx");
			} else {
				// 48 8B 0D ? ? ? ? 45 33 C9 89 7C 24 30 - Exodus OLD
				mov_g_string_container = FindPatternInEXE(
					"\x48\x8B\x0D\x00\x00\x00\x00\x45\x33\xC9\x89\x7C\x24\x30",
					"xxx????xxxxxxx");

				// E8 ? ? ? ? E9 ? ? ? ? 48 8D 51 14 - Exodus OLD
				call_igame_level_signal = FindPatternInEXE("\xE8\x00\x00\x00\x00\xE9\x00\x00\x00\x00\x48\x8D\x51\x14", "x????x????xxxx");

				// E8 ? ? ? ? 4C 89 65 58 - Exodus OLD
				call_payload_exodus = FindPatternInEXE("\xE8\x00\x00\x00\x00\x4C\x89\x65\x58", "x????xxxx");
			}

			g_string_container = (void**)GetAddrFromRelativeInstr(mov_g_string_container, 7, 3);
			igame_level_signal = Utils::GetAddrFromRelativeInstr(call_igame_level_signal, 5, 1);
			payload_exodus = (_payload_exodus)Utils::GetAddrFromRelativeInstr(call_payload_exodus, 5, 1);
		}
	}

	if (isInited) {
		if ((isRedux() && isReduxEGS) || isArktika()) {
			// 48 8B 0D ? ? ? ? 48 8D 15 ? ? ? ? 45 33 C9 88 81 - Redux EGS & Arktika
			mov_g_level = FindPatternInEXE(
				"\x48\x8B\x0D\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00\x45\x33\xC9\x88\x81",
				"xxx????xxx????xxxxx");
		}

		if (isExodus() && !isExodusPatched) {
			// 48 83 EC ? 48 8B 05 ? ? ? ? 48 85 C0 0F 85 ? ? ? ? 48 89 5C 24 ? 48 89 - Exodus Old
			Utils::GetConsole = (_GetConsole)FindPatternInEXE(
				"\x48\x83\xEC\x00\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x0F\x85\x00\x00\x00\x00\x48\x89\x5C\x24\x00\x48\x89",
				"xxx?xxx????xxxxx????xxxx?xx");
		} else {
			// 48 83 ec ? 48 8b 05 ? ? ? ? 48 85 c0 75 ? e8 ? ? ? ? 48 8b 05 - Redux STEAM and Arktika1 and Exodus NEW
			Utils::GetConsole = (_GetConsole)FindPatternInEXE(
				"\x48\x83\xec\x00\x48\x8b\x05\x00\x00\x00\x00\x48\x85\xc0\x75\x00\xe8\x00\x00\x00\x00\x48\x8b\x05",
				"xxx?xxx????xxxx?x????xxx");

			if (Utils::GetConsole == NULL) {
				// 48 83 EC 38 48 8B 05 ? ? ? ? 48 85 C0 0F 85 ? ? ? ? 48 89 5C 24 ? 48 89 - Redux EGS
				Utils::GetConsole = (_GetConsole)FindPatternInEXE(
					"\x48\x83\xEC\x38\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x0F\x85\x00\x00\x00\x00\x48\x89\x5C\x24\x00\x48\x89",
					"xxxxxxx????xxxxx????xxxx?xx");
			}
		}

		// вычисляем адрес и получаем g_game
		g_game = (DWORD64*)GetAddrFromRelativeInstr(mov_g_game, 7, 3);

		if (mov_g_level != NULL) {
			// вычисляем адрес и получаем g_level
			g_level = (DWORD64*)GetAddrFromRelativeInstr(mov_g_level, 7, 3);
		}
	}
#endif
}

GAME Utils::GetGame()
{
	return Game;
}

#ifdef _WIN64
bool Utils::isRedux()
{
	return Game == GAME::REDUX;
}

bool Utils::isArktika()
{
	return Game == GAME::ARKTIKA;
}

bool Utils::isExodus()
{
	return Game == GAME::EXODUS;
}

DWORD64 Utils::GetAddrFromRelativeInstr(DWORD64 instr_addr, int instr_len, int rel_offset)
{
	return (instr_addr + instr_len + *(int32_t*)(instr_addr + rel_offset));
}

// Only EXODUS
UINT Utils::GetTimeGlobalMS()
{
	if (isExodus()) {
		return *engine_time__global_ms;
	} else {
		return NULL;
	}
}

DWORD64 Utils::GetGLevel()
{
	return *g_level;
}

DWORD64 Utils::GetGGame()
{
	return *g_game;
}

DWORD64 Utils::GetGEntities()
{
	if (isExodus()) {
		return *g_entities;
	} else {
		return NULL;
	}
}

void Utils::slowmo_debug_increase()
{
	if (*slowmo_scale_debug > 0.0099999998) {
		if (*slowmo_scale_debug > 0.029999999) {
			if (*slowmo_scale_debug > 0.059999999) {
				if (*slowmo_scale_debug > 0.12)
					*slowmo_scale_debug = fmaxf(fminf(*slowmo_scale_debug + 0.25, 10.0), 0.25);
				else
					*slowmo_scale_debug = 0.25;
			} else {
				*slowmo_scale_debug = 0.12;
			}
		} else {
			*slowmo_scale_debug = 0.059999999;
		}
	} else {
		*slowmo_scale_debug = 0.029999999;
	}
}

void Utils::slowmo_debug_decrease()
{
	if (*slowmo_scale_debug <= 0.5) {
		if (*slowmo_scale_debug <= 0.25) {
			if (*slowmo_scale_debug <= 0.12) {
				if (*slowmo_scale_debug <= 0.059999999) {
					if (*slowmo_scale_debug <= 0.029999999)
						*slowmo_scale_debug = 0.0099999998;
					else
						*slowmo_scale_debug = 0.029999999;
				} else {
					*slowmo_scale_debug = 0.059999999;
				}
			} else {
				*slowmo_scale_debug = 0.12;
			}
		} else {
			*slowmo_scale_debug = 0.25;
		}
	} else {
		*slowmo_scale_debug = fmaxf(fminf(*slowmo_scale_debug - 0.25, 1.0), 0.0049999999);
	}
}

void Utils::slowmo_debug(float f)
{
	*slowmo_scale_debug = f;
}

#else

bool Utils::is2033()
{
	return Game == GAME::ORIG2033;
}

bool Utils::isLL()
{
	return Game == GAME::LL;
}

DWORD Utils::GetAddrFromRelativeInstr(DWORD instr_addr, int instr_len, int rel_offset)
{
	return (instr_addr + instr_len + *(int32_t*)(instr_addr + rel_offset));
}

DWORD Utils::GetGLevel()
{
	if (is2033()) {
		return *g_level;
	} else {
		return NULL;
	}
}

DWORD Utils::GetGGame()
{
	if (isLL()) {
		return *g_game;
	} else {
		return NULL;
	}
}

float Utils::GetDeltaF()
{
	if (is2033()) {
		return *delta_f;
	} else {
		return 0;
	}
}

#endif

void* Utils::str_shared(const char* str)
{
#ifdef _WIN64
	return str_container_do_dock(*g_string_container, str, strlen(str), 0);
#else
	if (is2033()) {
		return ((_str_container_do_dock_2033)str_container_do_dock)(str);
	} else {
		return ((_str_container_do_dock_LL)str_container_do_dock)(*g_string_container, str, 0);
	}
#endif
}

void Utils::signal(const char* str)
{
	void* s = Utils::str_shared(str);

#ifdef _WIN64
	if (Utils::isRedux()) {
		((_igame_level_signal)igame_level_signal)(NULL, &s, NULL, 0);
	}
	else if (Utils::isArktika()) {
		((_igame_level_signal_a1)igame_level_signal)(NULL, &s, NULL, 0);
	}
	else {
		((_igame_level_signal_ex)igame_level_signal)(NULL, &s, NULL, 0, payload_exodus());
	}

#else

	if (Utils::is2033()) {
		((_igame_level_signal_2033)igame_level_signal)(&s, 0);
	} else {
		((_igame_level_signal_LL)igame_level_signal)(&s, NULL, 0);
	}
#endif
}

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

float Utils::GetFloat(const char* section_name, const char* param_name, float param_default)
{
	string256 str;
	float param;

	GetString(section_name, param_name, "", str, sizeof(str));
	if (!str[0] || sscanf(str, "%f", &param) != 1)
		return param_default;

	return param;
}

bool Utils::FileExists(const char* fn)
{
	DWORD attrs = GetFileAttributes(fn);
	return (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}
