#include "Hooks.h"
#include "Utils.h"
#include "ConsoleUnlocker.h"
#include "QuickSave.h"
#include "Unlock3rdPerson.h"
#include "Fly.h"
#include "RestoreCommands.h"
#include "ContentUnlocker.h"
#include "LogFile.h"
#include "NavMapGen.h"

bool g_unlock_dev_console = false;
bool g_quicksave = false;
bool g_fly = false;
bool g_unlock_3rd_person_camera = false;
bool g_unlock_content_folder = false;

void* clevel_r_on_key_press_Orig = nullptr;

typedef void(__stdcall* _cmd_register_commands)();
_cmd_register_commands cmd_register_commands_Orig = nullptr;

typedef void(__thiscall* _slog)(const char* s);
_slog slog_Orig = nullptr;
void* slog_Address = nullptr;

#ifndef _WIN64
static bool isLL = false;
static bool isNavMapEnabled = false;
void* vfs_ropen_Orig = nullptr;
void* vfs_ropen_os = nullptr;
typedef void(__cdecl* _vfs_rbuffered)(const char* fn, void* a1, void* method);
_vfs_rbuffered vfs_rbuffered_Orig = nullptr;
#else
typedef void* (__fastcall* _vfs_ropen_package)(void* result, void* package, const char* fn, const int force_raw, unsigned int* uncompressed_size);
typedef void* (__fastcall* _vfs_ropen_packageExodus)(void* result, void* package, const char* fn, const int force_raw, unsigned int* uncompressed_size, void* _unknown);
typedef void(__fastcall* _vfs_rbuffered_package)(void* package, const char* fn, void* cb, const int force_raw);
typedef void(__fastcall* _vfs_rbuffered_packageExodus)(const char* fn, void* cb);
typedef void* (__fastcall* _rblock_init)(const char* fn, unsigned int* f_offset, unsigned int* f_size, unsigned int not_packaged);
typedef bool(__fastcall* _vfs_package_registry_level_downloaded)(void* _this, const char* map_name);

void* vfs_ropen_package_Orig = nullptr;
void* vfs_rbuffered_package_Orig = nullptr;
_rblock_init rblock_init_Orig = nullptr;
_vfs_package_registry_level_downloaded vfs_package_registry_level_downloaded_Orig = nullptr;

extern "C" void walking_fix_attempt();
void* cplayer_process_state_end_Orig = nullptr;
#endif

Hooks::Hooks()
{
	g_unlock_dev_console = Utils::GetBool("other", "unlock_dev_console", false);
	g_quicksave = Utils::GetBool("other", "quicksave", false);
	g_fly = Utils::GetBool("other", "fly", false);
	g_unlock_3rd_person_camera = Utils::GetBool("other", "unlock_3rd_person_camera", false);
	g_unlock_content_folder = Utils::GetBool("other", "unlock_content_folder", false);

	bool minhook = (MH_Initialize() == MH_OK);
	if (minhook)
	{
#ifndef _WIN64
		isLL = Utils::isLL();

		///////////////////////////////////////////////////////////////
		// 51 ? 8B ? 8B 0D ? ? ? ? 85
		LPVOID clevel_r_on_key_press_Address = (LPVOID)FindPatternInEXE(
			(BYTE*)"\x51\x00\x8B\x00\x8B\x0D\x00\x00\x00\x00\x85",
			"x?x?xx????x");

		SetHook("clevel_r_on_key_press", clevel_r_on_key_press_Address, (isLL ? (LPVOID)&clevel_r_on_key_press_Hook : (LPVOID)&clevel_r_on_key_press_Hook2033), 
			reinterpret_cast<LPVOID*>(&clevel_r_on_key_press_Orig));

		///////////////////////////////////////////////////////////////

		if (isLL && Utils::GetBool("other", "restore_deleted_commands", false)) {
			// B8 ? ? ? ? 53 BB
			LPVOID cmd_register_commands_Address = (LPVOID)FindPatternInEXE(
				(BYTE*)"\xB8\x00\x00\x00\x00\x53\xBB",
				"x????xx");

			SetHook("cmd_register_commands", cmd_register_commands_Address, (LPVOID)&cmd_register_commands_Hook,
				reinterpret_cast<LPVOID*>(&cmd_register_commands_Orig));
		}

		///////////////////////////////////////////////////////////////

		if (g_unlock_content_folder) {
			// 55 8B EC 83 E4 ? 83 EC ? 53 57 8D 44 24
			LPVOID vfs_ropen_Address = (LPVOID)FindPatternInEXE(
				(BYTE*)"\x55\x8B\xEC\x83\xE4\x00\x83\xEC\x00\x53\x57\x8D\x44\x24",
				"xxxxx?xx?xxxxx");

			LPVOID vfs_rbuffered_Address;

			if (!isLL) {
				// 55 8B EC 83 E4 ? 81 EC ? ? ? ? 53 8B 1D ? ? ? ? 56 8D 44 24
				vfs_ropen_os = (LPVOID)FindPatternInEXE(
					(BYTE*)"\x55\x8B\xEC\x83\xE4\x00\x81\xEC\x00\x00\x00\x00\x53\x8B\x1D\x00\x00\x00\x00\x56\x8D\x44\x24",
					"xxxxx?xx????xxx????xxxx");

				// 55 8B EC 83 E4 ? 83 EC ? 53 56 57 8D 44 24 ? 50
				vfs_rbuffered_Address = (LPVOID)FindPatternInEXE(
					(BYTE*)"\x55\x8B\xEC\x83\xE4\x00\x83\xEC\x00\x53\x56\x57\x8D\x44\x24\x00\x50",
					"xxxxx?xx?xxxxxx?x");
			} else {
				// 55 8B EC 83 E4 ? 81 EC ? ? ? ? 56 57 8B 3D ? ? ? ? 8D 44 24
				vfs_ropen_os = (LPVOID)FindPatternInEXE(
					(BYTE*)"\x55\x8B\xEC\x83\xE4\x00\x81\xEC\x00\x00\x00\x00\x56\x57\x8B\x3D\x00\x00\x00\x00\x8D\x44\x24",
					"xxxxx?xx????xxxx????xxx");

				// 83 EC ? 53 55 56 57 8D 44 24 ? 50
				vfs_rbuffered_Address = (LPVOID)FindPatternInEXE(
					(BYTE*)"\x83\xEC\x00\x53\x55\x56\x57\x8D\x44\x24\x00\x50",
					"xx?xxxxxxx?x");
			}

			SetHook("vfs_ropen", vfs_ropen_Address, (LPVOID)&vfs_ropen_Hook, reinterpret_cast<LPVOID*>(&vfs_ropen_Orig));
			SetHook("vfs_rbuffered", vfs_rbuffered_Address, (LPVOID)&vfs_rbuffered_Hook, reinterpret_cast<LPVOID*>(&vfs_rbuffered_Orig));
		}

		///////////////////////////////////////////////////////////////

		isNavMapEnabled = (!isLL && strstr(GetCommandLine(), "-navmap"));

		if (Utils::GetBool("log", "enabled", false) || isNavMapEnabled) {
			if (!isLL) {
				// B8 ? ? ? ? E8 ? ? ? ? 53 33 DB - 2033
				slog_Address = (LPVOID)FindPatternInEXE(
					(BYTE*)"\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x53\x33\xDB",
					"x????x????xxx");
			} else {
				// B8 ? ? ? ? E8 ? ? ? ? 53 33 DB 56 33 C0 - LL
				slog_Address = (LPVOID)FindPatternInEXE(
					(BYTE*)"\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x53\x33\xDB\x56\x33\xC0",
					"x????x????xxxxxx");
			}

			SetHook("slog", slog_Address, (LPVOID)&slog_Hook, reinterpret_cast<LPVOID*>(&slog_Orig));
		}

		///////////////////////////////////////////////////////////////
#else
		///////////////////////////////////////////////////////////////
		LPVOID clevel_r_on_key_press_Address = NULL;

		if (Utils::GetGame() == GAME::REDUX) {
			// 40 53 55 56 57 48 83 EC ? 48 8B F1 - Redux STEAM
			clevel_r_on_key_press_Address = (LPVOID)FindPatternInEXE(
				(BYTE*)"\x40\x53\x55\x56\x57\x48\x83\xEC\x00\x48\x8B\xF1",
				"xxxxxxxx?xxx");

			if (clevel_r_on_key_press_Address == NULL) {
				// 40 55 56 57 41 57 48 83 EC 58 - Redux EGS
				clevel_r_on_key_press_Address = (LPVOID)FindPatternInEXE(
					(BYTE*)"\x40\x55\x56\x57\x41\x57\x48\x83\xEC\x58",
					"xxxxxxxxxx");
			}
		} else if (Utils::GetGame() == GAME::ARKTIKA) {
			// 48 89 5C 24 ? 55 57 41 54 41 55 41 57 48 83 EC 30 - Arktika
			clevel_r_on_key_press_Address = (LPVOID)FindPatternInEXE(
				(BYTE*)"\x48\x89\x5C\x24\x00\x55\x57\x41\x54\x41\x55\x41\x57\x48\x83\xEC\x30",
				"xxxx?xxxxxxxxxxxx");
		} else if (Utils::GetGame() == GAME::EXODUS) {
			// 44 89 4C 24 ? 44 89 44 24 ? 89 54 24 10 48 89 4C 24 ? 55 53 57 - Exodus
			clevel_r_on_key_press_Address = (LPVOID)FindPatternInEXE(
				(BYTE*)"\x44\x89\x4C\x24\x00\x44\x89\x44\x24\x00\x89\x54\x24\x10\x48\x89\x4C\x24\x00\x55\x53\x57",
				"xxxx?xxxx?xxxxxxxx?xxx");
		}

		SetHook("clevel_r_on_key_press", clevel_r_on_key_press_Address, (LPVOID)&clevel_r_on_key_press_Hook, reinterpret_cast<LPVOID*>(&clevel_r_on_key_press_Orig));

		///////////////////////////////////////////////////////////////

		if (Utils::GetBool("other", "restore_deleted_commands", false)) {
			LPVOID cmd_register_commands_Address = NULL;

			if (Utils::GetGame() == GAME::REDUX) {
				// 48 89 5C 24 ? 57 48 83 EC 20 8B 05 ? ? ? ? 48 8D 1D ? ? ? ? 48 8D 3D ? ? ? ? A8 01 75 60 - Redux STEAM
				cmd_register_commands_Address = (LPVOID)FindPatternInEXE(
					(BYTE*)"\x48\x89\x5C\x24\x00\x57\x48\x83\xEC\x20\x8B\x05\x00\x00\x00\x00\x48\x8D\x1D\x00\x00\x00\x00\x48\x8D\x3D\x00\x00\x00\x00\xA8\x01\x75\x60",
					"xxxx?xxxxxxx????xxx????xxx????xxxx");

				if (cmd_register_commands_Address == NULL) {
					// 40 53 48 83 EC 20 65 48 8B 04 25 ? ? ? ? 8B 0D ? ? ? ? BA ? ? ? ? 48 8B 1C C8 48 03 DA 8B 03 39 05 - Redux EGS
					cmd_register_commands_Address = (LPVOID)FindPatternInEXE(
						(BYTE*)"\x40\x53\x48\x83\xEC\x20\x65\x48\x8B\x04\x25\x00\x00\x00\x00\x8B\x0D\x00\x00\x00\x00\xBA\x00\x00\x00\x00\x48\x8B\x1C\xC8\x48\x03\xDA\x8B\x03\x39\x05",
						"xxxxxxxxxxx????xx????x????xxxxxxxxxxx");
				}
			} else if (Utils::GetGame() == GAME::ARKTIKA || Utils::GetGame() == GAME::EXODUS) {
				// 40 53 48 83 EC 20 65 48 8B 04 25 ? ? ? ? 8B 0D ? ? ? ? BA ? ? ? ? 48 8B 1C C8 48 03 DA 8B 03 39 05 - Arktika and Exodus
				cmd_register_commands_Address = (LPVOID)FindPatternInEXE(
					(BYTE*)"\x40\x53\x48\x83\xEC\x20\x65\x48\x8B\x04\x25\x00\x00\x00\x00\x8B\x0D\x00\x00\x00\x00\xBA\x00\x00\x00\x00\x48\x8B\x1C\xC8\x48\x03\xDA\x8B\x03\x39\x05",
					"xxxxxxxxxxx????xx????x????xxxxxxxxxxx");
			}

			SetHook("cmd_register_commands", cmd_register_commands_Address, (LPVOID)&cmd_register_commands_Hook, reinterpret_cast<LPVOID*>(&cmd_register_commands_Orig));
		}

		///////////////////////////////////////////////////////////////

		if (g_unlock_content_folder) {
			LPVOID vfs_ropen_package_Address = NULL;
			LPVOID vfs_rbuffered_package_Address = NULL;
			LPVOID vfs_package_registry_level_downloaded_Address = NULL;

			bool onReduxEGS = false;

			if (Utils::GetGame() == GAME::REDUX) {
				// 48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 54 41 56 41 57 48 83 EC ? 45 33 E4 45 8B F9 - Redux STEAM
				vfs_ropen_package_Address = (LPVOID)FindPatternInEXE(
					(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x56\x57\x41\x54\x41\x56\x41\x57\x48\x83\xEC\x00\x45\x33\xE4\x45\x8B\xF9",
					"xxxx?xxxx?xxxxxxxxxxx?xxxxxx");

				if (vfs_ropen_package_Address == NULL) {
					// 48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 54 41 56 41 57 48 83 EC 30 45 33 E4 41 8B E9 49 8B F0 48 8B DA 4C 8B F1 44 39 62 0C 0F 84 ? ? ? ? 4D 8B C8 4C 89 64 24 ? 4C 8B 02 41 BF ? ? ? ? 33 D2 - Redux EGS
					vfs_ropen_package_Address = (LPVOID)FindPatternInEXE(
						(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x56\x57\x41\x54\x41\x56\x41\x57\x48\x83\xEC\x30\x45\x33\xE4\x41\x8B\xE9\x49\x8B\xF0\x48\x8B\xDA\x4C\x8B\xF1\x44\x39\x62\x0C\x0F\x84\x00\x00\x00\x00\x4D\x8B\xC8\x4C\x89\x64\x24\x00\x4C\x8B\x02\x41\xBF\x00\x00\x00\x00\x33\xD2",
						"xxxx?xxxx?xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx????xxxxxxx?xxxxx????xx");
				}

				// 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 41 56 48 83 EC 40 48 8B 1D ? ? ? ? 48 8B F1 48 8B 6A 08 4C 8B 32 48 85 DB 75 0F E8 ? ? ? ? 48 8B D8 48 89 05 - Redux EGS
				vfs_rbuffered_package_Address = (LPVOID)FindPatternInEXE(
					(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x41\x56\x48\x83\xEC\x40\x48\x8B\x1D\x00\x00\x00\x00\x48\x8B\xF1\x48\x8B\x6A\x08\x4C\x8B\x32\x48\x85\xDB\x75\x0F\xE8\x00\x00\x00\x00\x48\x8B\xD8\x48\x89\x05",
					"xxxx?xxxx?xxxx?xxxxxxxxx");

				if (vfs_rbuffered_package_Address == NULL) {
					// 48 89 5C 24 ? 48 89 74 24 ? 41 56 48 83 EC ? 83 79 - Redux STEAM
					vfs_rbuffered_package_Address = (LPVOID)FindPattern(
						(DWORD64)mi.lpBaseOfDll,
						mi.SizeOfImage,
						(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x41\x56\x48\x83\xEC\x00\x83\x79",
						"xxxx?xxxx?xxxxx?xx");

					onReduxEGS = false;
				} else {
					onReduxEGS = true;
				}

				// 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 54 41 56 41 57 48 83 EC 20 44 0F B7 B9 ? ? ? ? 33 DB 4C 8B F2 4C 8B E1 45 85 FF 74 6E - Redux ONLY ON STEAM
				if (!onReduxEGS) {
					vfs_package_registry_level_downloaded_Address = (LPVOID)FindPatternInEXE(
						(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x41\x54\x41\x56\x41\x57\x48\x83\xEC\x20\x44\x0F\xB7\xB9\x00\x00\x00\x00\x33\xDB\x4C\x8B\xF2\x4C\x8B\xE1\x45\x85\xFF\x74\x6E",
						"xxxx?xxxx?xxxx?xxxx?xxxxxxxxxxxxxx????xxxxxxxxxxxxx");
				}
			} else if (Utils::GetGame() == GAME::ARKTIKA) {
				// 48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 54 41 56 41 57 48 83 EC 40 45 33 E4 45 8B F9 4D 8B F0 48 8B DA 48 8B F9 44 39 62 0C 0F 84 ? ? ? ? 4D 8B C8 4C 89 64 24 ? 4C 8B 02 BD ? ? ? ? 33 D2 89 6C 24 20 48 8B CB E8 ? ? ? ? 48 8B F0 - Arktika
				vfs_ropen_package_Address = (LPVOID)FindPatternInEXE(
					(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x56\x57\x41\x54\x41\x56\x41\x57\x48\x83\xEC\x40\x45\x33\xE4\x45\x8B\xF9\x4D\x8B\xF0\x48\x8B\xDA\x48\x8B\xF9\x44\x39\x62\x0C\x0F\x84\x00\x00\x00\x00\x4D\x8B\xC8\x4C\x89\x64\x24\x00\x4C\x8B\x02\xBD\x00\x00\x00\x00\x33\xD2\x89\x6C\x24\x20\x48\x8B\xCB\xE8\x00\x00\x00\x00\x48\x8B\xF0",
					"xxxx?xxxx?xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx????xxxxxxx?xxxx????xxxxxxxxxx????xxx");

				// 48 89 5C 24 ? 48 89 74 24 ? 41 56 48 83 EC 40 83 79 0C 00 4D 8B F0 48 8B F2 48 8B D9 0F 84 ? ? ? ? 4C 8B 01 4C 8B CA 33 D2 48 C7 44 24 ? ? ? ? ? 48 89 7C 24 ? C7 44 24 ? ? ? ? ? E8 ? ? ? ? 48 8B F8 48 85 C0 74 7A 8B 48 18 8D 0C 49 - Arktika
				vfs_rbuffered_package_Address = (LPVOID)FindPatternInEXE(
					(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x41\x56\x48\x83\xEC\x40\x83\x79\x0C\x00\x4D\x8B\xF0\x48\x8B\xF2\x48\x8B\xD9\x0F\x84\x00\x00\x00\x00\x4C\x8B\x01\x4C\x8B\xCA\x33\xD2\x48\xC7\x44\x24\x00\x00\x00\x00\x00\x48\x89\x7C\x24\x00\xC7\x44\x24\x00\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8B\xF8\x48\x85\xC0\x74\x7A\x8B\x48\x18\x8D\x0C\x49",
					"xxxx?xxxx?xxxxxxxxxxxxxxxxxxxxx????xxxxxxxxxxxx?????xxxx?xxx?????x????xxxxxxxxxxxxxx");

				// 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 54 41 56 41 57 48 83 EC 20 44 0F B7 B9 ? ? ? ? 33 DB 4C 8B F2 4C 8B E1 45 85 FF 74 6D 33 FF 66 66 66 0F 1F 84 00 ? ? ? ? 46 8B 9C 27 - Arktika
				//vfs_package_registry_level_downloaded_Address = (LPVOID)FindPatternInEXE(
				//	(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x41\x54\x41\x56\x41\x57\x48\x83\xEC\x20\x44\x0F\xB7\xB9\x00\x00\x00\x00\x33\xDB\x4C\x8B\xF2\x4C\x8B\xE1\x45\x85\xFF\x74\x6D\x33\xFF\x66\x66\x66\x0F\x1F\x84\x00\x00\x00\x00\x00\x46\x8B\x9C\x27",
				//	"xxxx?xxxx?xxxx?xxxx?xxxxxxxxxxxxxx????xxxxxxxxxxxxxxxxxxxxxx????xxxx");
			} else if (Utils::GetGame() == GAME::EXODUS) {
				// 48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 54 41 55 41 57 48 83 EC 40 45 33 ED 45 8B E1 49 8B E8 48 8B FA 48 8B F1 44 39 6A 0C 0F 84 ? ? ? ? 48 8B 84 24 ? ? ? ? 4D 8B C8 4C 8B 02 41 BF - Exodus
				vfs_ropen_package_Address = (LPVOID)FindPatternInEXE(
					(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x56\x57\x41\x54\x41\x55\x41\x57\x48\x83\xEC\x40\x45\x33\xED\x45\x8B\xE1\x49\x8B\xE8\x48\x8B\xFA\x48\x8B\xF1\x44\x39\x6A\x0C\x0F\x84\x00\x00\x00\x00\x48\x8B\x84\x24\x00\x00\x00\x00\x4D\x8B\xC8\x4C\x8B\x02\x41\xBF",
					"xxxx?xxxx?xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx????xxxx????xxxxxxxx");

				// 40 56 48 83 EC 30 48 8B 42 08 - Exodus
				vfs_rbuffered_package_Address = (LPVOID)FindPatternInEXE(
					(BYTE*)"\x40\x56\x48\x83\xEC\x30\x48\x8B\x42\x08",
					"xxxxxxxxxx");
			}

			SetHook("vfs_ropen_package", vfs_ropen_package_Address, Utils::GetGame() == GAME::EXODUS ? (LPVOID)&vfs_ropen_package_HookExodus : (LPVOID)&vfs_ropen_package_Hook, reinterpret_cast<LPVOID*>(&vfs_ropen_package_Orig));
			SetHook("vfs_rbuffered_package", vfs_rbuffered_package_Address, Utils::GetGame() == GAME::EXODUS ? (LPVOID)&vfs_rbuffered_package_HookExodus : (LPVOID)&vfs_rbuffered_package_Hook, reinterpret_cast<LPVOID*>(&vfs_rbuffered_package_Orig));
			if (Utils::GetGame() == GAME::REDUX && !onReduxEGS) {
				SetHook("vfs_package_registry_level_downloaded", vfs_package_registry_level_downloaded_Address, (LPVOID)&vfs_package_registry_level_downloaded_Hook,
					reinterpret_cast<LPVOID*>(&vfs_package_registry_level_downloaded_Orig));
			}
		}

		///////////////////////////////////////////////////////////////

		if (Utils::GetBool("log", "enabled", false)) {
			if (Utils::GetGame() == GAME::REDUX) {
				// 40 53 B8 ? ? ? ? E8 ? ? ? ? 48 2B E0 33 C0 - Redux STEAM
				slog_Address = (LPVOID)FindPatternInEXE(
					(BYTE*)"\x40\x53\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x2B\xE0\x33\xC0",
					"xxx????x????xxxxx");

				if (slog_Address == NULL) {
					// 48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 56 B8 ? ? ? ? E8 - Redux EGS
					slog_Address = (LPVOID)FindPatternInEXE(
						(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x56\x57\x41\x56\xB8\x00\x00\x00\x00\xE8",
						"xxxx?xxxx?xxxxx????x");
				}
			} else if (Utils::GetGame() == GAME::ARKTIKA) {
				// 48 89 5C 24 ? 57 B8 ? ? ? ? E8 ? ? ? ? 48 2B E0 48 8B D9 E8 ? ? ? ? 33 D2 BF ? ? ? ? 38 13 74 5C 33 C0 0F B6 0B 80 F9 0A 75 41 C6 44 04 ? ? 80 7C 24 ? ? 75 07 66 C7 44 24 - Arktika
				slog_Address = (LPVOID)FindPatternInEXE(
					(BYTE*)"\x48\x89\x5C\x24\x00\x57\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x2B\xE0\x48\x8B\xD9\xE8\x00\x00\x00\x00\x33\xD2\xBF\x00\x00\x00\x00\x38\x13\x74\x5C\x33\xC0\x0F\xB6\x0B\x80\xF9\x0A\x75\x41\xC6\x44\x04\x00\x00\x80\x7C\x24\x00\x00\x75\x07\x66\xC7\x44\x24",
					"xxxx?xx????x????xxxxxxx????xxx????xxxxxxxxxxxxxxxxx??xxx??xxxxxx");
			} else if (Utils::GetGame() == GAME::EXODUS) {
				// 40 56 B8 ? ? ? ? E8 ? ? ? ? 48 2B E0 48 8B F1 E8 ? ? ? ? 45 33 C0 44 38 06 0F 84 ? ? ? ? 48 89 9C 24 ? ? ? ? 33 DB 33 C0 48 89 BC 24 ? ? ? ? 0F B6 14 33 80 FA 0D 75 36 80 7C 33 ? ? 75 2F C6 44 04 ? ? 80 7C 24 ? ? 75 07 66 C7 44 24 - Exodus
				slog_Address = (LPVOID)FindPatternInEXE(
					(BYTE*)"\x40\x56\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x2B\xE0\x48\x8B\xF1\xE8\x00\x00\x00\x00\x45\x33\xC0\x44\x38\x06\x0F\x84\x00\x00\x00\x00\x48\x89\x9C\x24\x00\x00\x00\x00\x33\xDB\x33\xC0\x48\x89\xBC\x24\x00\x00\x00\x00\x0F\xB6\x14\x33\x80\xFA\x0D\x75\x36\x80\x7C\x33\x00\x00\x75\x2F\xC6\x44\x04\x00\x00\x80\x7C\x24\x00\x00\x75\x07\x66\xC7\x44\x24",
					"xxx????x????xxxxxxx????xxxxxxxx????xxxx????xxxxxxxx????xxxxxxxxxxxx??xxxxx??xxx??xxxxxx");
			}

			SetHook("slog", slog_Address, (LPVOID)&slog_Hook, reinterpret_cast<LPVOID*>(&slog_Orig));
		}

		///////////////////////////////////////////////////////////////

		if (Utils::GetGame() == GAME::ARKTIKA) {
			if (Utils::GetBool("arktika1", "walking_hack", false)) {
				// 48 81 c4 ? ? ? ? 5e 5b 5d c3 cc cc cc cc cc cc cc cc cc cc cc cc cc cc cc cc cc 48 8b c4 44 89 48 - Arktika
				LPVOID cplayer_process_state_end_Address = (LPVOID)FindPatternInEXE(
					(BYTE*)"\x48\x81\xc4\x00\x00\x00\x00\x5e\x5b\x5d\xc3\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc\x48\x8b\xc4\x44\x89\x48",
					"xxx????xxxxxxxxxxxxxxxxxxxxxxxxxxx");

				SetHook("walking_fix_attempt", cplayer_process_state_end_Address, (LPVOID)&walking_fix_attempt, reinterpret_cast<LPVOID*>(&cplayer_process_state_end_Orig));
			}
		}

		///////////////////////////////////////////////////////////////
#endif
	} else {
		MessageBox(NULL, "MinHook not initialized!", "MinHook", MB_OK | MB_ICONERROR);
	}
}

void Hooks::SetHook(char* hookName, LPVOID pTarget, LPVOID pDetour, LPVOID *ppOriginal)
{
	MH_STATUS status = MH_CreateHook(pTarget, pDetour, ppOriginal);

	if (status == MH_OK) {
		if (MH_EnableHook(pTarget) != MH_OK) {
			MessageBox(NULL, "MH_EnableHook() != MH_OK", hookName, MB_OK | MB_ICONERROR);
		}
	} else {
		MessageBox(NULL, "MH_CreateHook() != MH_OK", hookName, MB_OK | MB_ICONERROR);
	}
}

///////////////////////////////////////////////////////////////

#ifndef _WIN64
void __fastcall Hooks::clevel_r_on_key_press_Hook2033(void* _this, void* _unused, int action, int key, int state) // 2033 orig
{
	//printf("action = %d, key = %d, state = %d\n", action, key, state);

	if (g_unlock_dev_console) {
		ConsoleUnlocker::clevel_r_on_key_press(action, key, state, -1);
	}

	if (g_quicksave) {
		QuickSave::clevel_r_on_key_press(action, key, state, -1);
	}

	if (g_fly) {
		Fly::clevel_r_on_key_press(action, key, state, -1);
	}

	if (g_unlock_3rd_person_camera) {
		Unlock3rdPerson::clevel_r_on_key_press(_this, action, key, state, -1);
	}

	typedef void(__thiscall* _clevel_r_on_key_press_2033)(void* _this, int action, int key, int state);
	((_clevel_r_on_key_press_2033)clevel_r_on_key_press_Orig)(_this, action, key, state);
}

void __fastcall Hooks::clevel_r_on_key_press_Hook(void* _this, void* _unused, int action, int key, int state, int resending) // LL orig
#else
void __fastcall Hooks::clevel_r_on_key_press_Hook(void* _this, int action, int key, int state, int resending) // Redux or newer
#endif
{
	//printf("action = %d, key = %d, state = %d, resending = %d\n", action, key, state, resending);

	if (g_unlock_dev_console) {
		ConsoleUnlocker::clevel_r_on_key_press(action, key, state, resending);
	}

	if (g_quicksave) {
		QuickSave::clevel_r_on_key_press(action, key, state, resending);
	}

	if (g_fly) {
		Fly::clevel_r_on_key_press(action, key, state, resending);
	}

	if (g_unlock_3rd_person_camera) {
		Unlock3rdPerson::clevel_r_on_key_press(_this, action, key, state, resending);
	}

	typedef void(__thiscall* _clevel_r_on_key_press)(void* _this, int action, int key, int state, int resending);
	((_clevel_r_on_key_press)clevel_r_on_key_press_Orig)(_this, action, key, state, resending);
}

///////////////////////////////////////////////////////////////

void Hooks::cmd_register_commands_Hook()
{
	cmd_register_commands_Orig();

	RestoreCommands::cmd_register_commands();
}

///////////////////////////////////////////////////////////////

#ifdef _WIN64
void* __fastcall Hooks::vfs_ropen_package_HookExodus(void* result, void* package, const char* fn, const int force_raw, unsigned int* uncompressed_size, void* _unknown)
{
	void* ret = ContentUnlocker::vfs_ropen_package(result, package, fn, force_raw, uncompressed_size);
	if (ret == nullptr) {
		ret = ((_vfs_ropen_packageExodus)vfs_ropen_package_Orig)(result, package, fn, force_raw, uncompressed_size, _unknown);
	}

	return ret;
}

void* __fastcall Hooks::vfs_ropen_package_Hook(void* result, void* package, const char* fn, const int force_raw, unsigned int* uncompressed_size)
{
	void* ret = ContentUnlocker::vfs_ropen_package(result, package, fn, force_raw, uncompressed_size);
	if (ret == nullptr) {
		ret = ((_vfs_ropen_package)vfs_ropen_package_Orig)(result, package, fn, force_raw, uncompressed_size);
	}

	return ret;
}

void __fastcall Hooks::vfs_rbuffered_package_Hook(void* package, const char* fn, void* cb, const int force_raw)
{
	ContentUnlocker::vfs_rbuffered_package(fn, (fastdelegate*)cb);
	((_vfs_rbuffered_package)vfs_rbuffered_package_Orig)(package, fn, cb, force_raw);
}

void __fastcall Hooks::vfs_rbuffered_package_HookExodus(const char* fn, void* cb)
{
	ContentUnlocker::vfs_rbuffered_package(fn, (fastdelegate*)cb);
	((_vfs_rbuffered_packageExodus)vfs_rbuffered_package_Orig)(fn, cb);
}

bool __fastcall Hooks::vfs_package_registry_level_downloaded_Hook(void* _this, const char* map_name)
{
	return ContentUnlocker::vfs_package_registry_level_downloaded(vfs_package_registry_level_downloaded_Orig(_this, map_name), map_name);
}

#else

//char format[] = "%s\n";
__declspec(naked) void Hooks::vfs_ropen_Hook(/*const char* fn*/)
{
	__asm {
		/*
		mov eax, [esp + 4]
		push eax
		mov eax, offset format
		push eax
		call printf
		add esp, 8
		*/

		mov eax, [esp + 4]
		push eax
		call GetFileAttributesA
		cmp eax, 0xFFFFFFFF
		je to_orig_code
	}

	if (!isLL) {
		__asm {
			push edi
			push esi
			mov edi, esi
			mov eax, [esp + 0x0C]
			push eax
			call vfs_ropen_os
			add esp, 4
			pop esi
			pop edi
			ret
		}
	} else {
		__asm {
			mov eax, [esp + 4]
			push eax
			push esi
			call vfs_ropen_os
			add esp, 8
			ret
		}
	}

	__asm {
	to_orig_code:
		jmp vfs_ropen_Orig
	}
}

void __cdecl Hooks::vfs_rbuffered_Hook(const char* fn, void* a1, void* method)
{
	ContentUnlocker::vfs_rbuffered(fn, a1, (_method)method);
	vfs_rbuffered_Orig(fn, a1, method);
}
#endif

///////////////////////////////////////////////////////////////

void __fastcall Hooks::slog_Hook(const char* s)
{
#ifndef _WIN64
	if (isNavMapEnabled) {
		NavMapGen::slog(s);
	}
#endif

	LogFile::slog(s);
	slog_Orig(s);
}

///////////////////////////////////////////////////////////////
