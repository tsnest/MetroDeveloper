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
#include "wpn_bobbing_la.h"
#include "BugTrap.h"

#define OTHER "other"

bool g_unlock_dev_console = false;
bool g_quicksave = false;
bool g_fly = false;
bool g_unlock_3rd_person_camera = false;
bool g_unlock_content_folder	= false;
bool g_cant_find_file_msg		= false;
bool g_restore_deleted_commands = false;

void* clevel_r_on_key_press_Orig = nullptr;
_cmd_register_commands cmd_register_commands_Orig = nullptr;
_slog slog_Orig				= nullptr;
_vfs_exists vfs_exists		= nullptr;
void* u_platform_initialize_end_Orig = nullptr;

static const char* cantFindFile = "!G Can't find file: %s";

#ifndef _WIN64
static bool isLL = false;
static bool isNavMapEnabled = false;
void* vfs_ropen_Orig = nullptr;
bool g_wpn_bobbing = false;

_vfs_rbuffered vfs_rbuffered_Orig = nullptr;
_vfs_registry vfs_registry = nullptr;
_vfs_package_registry_find vfs_package_registry_find = nullptr;
void** vfs = nullptr;

#else

void* vfs_ropen_package_Orig	 = nullptr;
void* vfs_rbuffered_package_Orig = nullptr;
_vfs_package_registry_level_downloaded vfs_package_registry_level_downloaded_Orig = nullptr;
_asset_manager_load_all_internal asset_manager_load_all_internal_Orig = nullptr;
_rblock_init rblock_init_Orig = nullptr;

extern "C" void walking_fix_attempt();
extern "C" void* load_navmap_Orig = nullptr;
#endif

Hooks::Hooks()
{
	g_unlock_dev_console	= Utils::GetBool(OTHER, "unlock_dev_console", false);
	g_quicksave				= Utils::GetBool(OTHER, "quicksave", false);
	g_fly					= Utils::GetBool(OTHER, "fly", false);
	g_unlock_3rd_person_camera	= Utils::GetBool(OTHER, "unlock_3rd_person_camera", false);
	g_unlock_content_folder		= Utils::GetBool(OTHER, "unlock_content_folder", false);
	g_cant_find_file_msg		= Utils::GetBool(OTHER, "cant_find_file_msg", false);
	g_restore_deleted_commands	= Utils::GetBool(OTHER, "restore_deleted_commands", false);

	bool minhook = (MH_Initialize() == MH_OK);
	if (minhook)
	{
#ifndef _WIN64
		isLL = Utils::isLL();

		///////////////////////////////////////////////////////////////
		// 51 ? 8B ? 8B 0D ? ? ? ? 85
		DWORD clevel_r_on_key_press_Address = FindPatternInEXE("\x51\x00\x8B\x00\x8B\x0D\x00\x00\x00\x00\x85", "x?x?xx????x");

		SetHook("clevel_r_on_key_press", (void*)clevel_r_on_key_press_Address, (isLL ? (void*) &clevel_r_on_key_press_HookLL : (void*) &clevel_r_on_key_press_Hook2033), 
			(void**) &clevel_r_on_key_press_Orig);

		///////////////////////////////////////////////////////////////

		g_wpn_bobbing = Utils::GetBool(BOBBING_SECT, "enabled", false);

		if (g_restore_deleted_commands || g_wpn_bobbing) {
			DWORD cmd_register_commands_Address = NULL;

			if (isLL) {
				if (!g_restore_deleted_commands) {
					goto tyda;
				}

				// B8 ? ? ? ? 53 BB - Last Light
				cmd_register_commands_Address = FindPatternInEXE("\xB8\x00\x00\x00\x00\x53\xBB", "x????xx");
			} else {
				// 55 8B EC 83 E4 F8 53 55 B8 ? ? ? ? 56 33 DB 84 05 ? ? ? ? 57 BD - 2033 ORIG
				cmd_register_commands_Address = FindPatternInEXE(
					"\x55\x8B\xEC\x83\xE4\xF8\x53\x55\xB8\x00\x00\x00\x00\x56\x33\xDB\x84\x05\x00\x00\x00\x00\x57\xBD",
					"xxxxxxxxx????xxxxx????xx");
			}

			SetHook("cmd_register_commands", (void*)cmd_register_commands_Address, (void*)&cmd_register_commands_Hook,
				(void**)&cmd_register_commands_Orig);
		}

	tyda:

		///////////////////////////////////////////////////////////////

		if (g_unlock_content_folder || g_cant_find_file_msg) {
			// 55 8B EC 83 E4 ? 83 EC ? 53 57 8D 44 24 - ORIG 2033 & LL
			DWORD vfs_ropen_Address = FindPatternInEXE("\x55\x8B\xEC\x83\xE4\x00\x83\xEC\x00\x53\x57\x8D\x44\x24", "xxxxx?xx?xxxxx");
			DWORD vfs_rbuffered_Address;

			if (!isLL) {
				// 55 8B EC 83 E4 ? 83 EC ? 53 56 57 8D 44 24 ? 50
				vfs_rbuffered_Address = FindPatternInEXE("\x55\x8B\xEC\x83\xE4\x00\x83\xEC\x00\x53\x56\x57\x8D\x44\x24\x00\x50", "xxxxx?xx?xxxxxx?x");

				// E8 ? ? ? ? 83 C4 28 0F B6 C0 - ORIG 2033
				DWORD call_vfs_exists = FindPatternInEXE("\xE8\x00\x00\x00\x00\x83\xC4\x28\x0F\xB6\xC0", "x????xxxxxx");
				vfs_exists = (_vfs_exists)Utils::GetAddrFromRelativeInstr(call_vfs_exists, 5, 1);
			} else {
				// 83 EC ? 53 55 56 57 8D 44 24 ? 50
				vfs_rbuffered_Address = FindPatternInEXE("\x83\xEC\x00\x53\x55\x56\x57\x8D\x44\x24\x00\x50", "xx?xxxxxxx?x");

				// A1 ? ? ? ? 85 C0 75 0A E8 ? ? ? ? A3 ? ? ? ? 83 78 08 00 74 1C 8B 54 24 04 8B 08 6A 00 - LL with patches
				vfs_exists = (_vfs_exists)FindPatternInEXE(
					"\xA1\x00\x00\x00\x00\x85\xC0\x75\x0A\xE8\x00\x00\x00\x00\xA3\x00\x00\x00\x00\x83\x78\x08\x00\x74\x1C\x8B\x54\x24\x04\x8B\x08\x6A\x00",
					"x????xxxxx????x????xxxxxxxxxxxxxx");

				if (vfs_exists == NULL) {
					// т.к. в версии без патчей функция vfs::exists заинлайнена везде, то мы сделаем эту функцию сами

					// E8 ? ? ? ? A3 ? ? ? ? 50
					DWORD call_vfs_registry = FindPatternInEXE("\xE8\x00\x00\x00\x00\xA3\x00\x00\x00\x00\x50", "x????x????x");
					vfs_registry = (_vfs_registry)Utils::GetAddrFromRelativeInstr(call_vfs_registry, 5, 1);

					// E8 ? ? ? ? 3B C7 74 0A
					DWORD call_vfs_package_registry_find = FindPatternInEXE("\xE8\x00\x00\x00\x00\x3B\xC7\x74\x0A", "x????xxxx");
					vfs_package_registry_find = (_vfs_package_registry_find)Utils::GetAddrFromRelativeInstr(call_vfs_package_registry_find, 5, 1);

					// A1 ? ? ? ? 83 C4 14 85 C0
					vfs = *(void***)(FindPatternInEXE("\xA1\x00\x00\x00\x00\x83\xC4\x14\x85\xC0", "x????xxxxx") + 1);

					vfs_exists = &vfs_exists_custom_ll;
				}
			}

			SetHook("vfs_ropen", (void*)vfs_ropen_Address, &vfs_ropen_HookASM, &vfs_ropen_Orig);
			SetHook("vfs_rbuffered", (void*)vfs_rbuffered_Address, &vfs_rbuffered_Hook, &vfs_rbuffered_Orig);
		}

		///////////////////////////////////////////////////////////////

		if (!isLL && Utils::GetBool(BOBBING_SECT, "enabled", false) &&
			(Utils::GetBool(BOBBING_SECT, "sprint", false) ||
				Utils::GetBool(BOBBING_SECT, "walk", false) ||
				Utils::GetBool(BOBBING_SECT, "aiming", false)))
		{
			// chud_item::update_hud_position + offset end of function
			// 74 29 8B 0B
			DWORD chud_item__update_hud_position_EndFuncAddress = FindPatternInEXE("\x74\x29\x8B\x0B", "xxxx") + 0x2E;
			SetHook("chud_item__update_hud_position", (void*)chud_item__update_hud_position_EndFuncAddress, &chud_item__update_hud_position_EndFuncHook, NULL);
		}

		///////////////////////////////////////////////////////////////

		isNavMapEnabled = (!isLL && strstr(GetCommandLine(), "-navmap"));

		if (Utils::GetBool("log", "enabled", false) || isNavMapEnabled) {
			DWORD slog_Address;
			if (!isLL) {
				// B8 ? ? ? ? E8 ? ? ? ? 53 33 DB - 2033
				slog_Address = FindPatternInEXE("\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x53\x33\xDB", "x????x????xxx");
			} else {
				// B8 ? ? ? ? E8 ? ? ? ? 53 33 DB 56 33 C0 - LL
				slog_Address = FindPatternInEXE("\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x53\x33\xDB\x56\x33\xC0", "x????x????xxxxxx");
			}

			SetHook("slog", (void*)slog_Address, &slog_Hook, &slog_Orig);
		}

		///////////////////////////////////////////////////////////////

		if (Utils::GetBool(OTHER, "bugtrap", false)) {
			// 55 8B EC 83 EC 24 53 56 57 68 ? ? ? ? FF 15 ? ? ? ? 50 FF 15
			DWORD u_platform_initialize_end_Address = FindPatternInEXE("\x55\x8B\xEC\x83\xEC\x24\x53\x56\x57\x68\x00\x00\x00\x00\xFF\x15\x00\x00\x00\x00\x50\xFF\x15",
				"xxxxxxxxxx????xx????xxx");

			SetHook("u_platform_initialize_end", (void*)u_platform_initialize_end_Address, &u_platform_initialize_end_Hook, &u_platform_initialize_end_Orig);
		}

		///////////////////////////////////////////////////////////////
#else
		///////////////////////////////////////////////////////////////
		DWORD64 clevel_r_on_key_press_Address = NULL;

		if (Utils::isRedux()) {
			// 40 53 55 56 57 48 83 EC ? 48 8B F1 - Redux STEAM
			clevel_r_on_key_press_Address = FindPatternInEXE("\x40\x53\x55\x56\x57\x48\x83\xEC\x00\x48\x8B\xF1", "xxxxxxxx?xxx");

			if (clevel_r_on_key_press_Address == NULL) {
				// 40 55 56 57 41 57 48 83 EC 58 - Redux EGS
				clevel_r_on_key_press_Address = FindPatternInEXE("\x40\x55\x56\x57\x41\x57\x48\x83\xEC\x58", "xxxxxxxxxx");
			}
		} else if (Utils::isArktika()) {
			// 48 89 5C 24 ? 55 57 41 54 41 55 41 57 48 83 EC 30 - Arktika
			clevel_r_on_key_press_Address = FindPatternInEXE("\x48\x89\x5C\x24\x00\x55\x57\x41\x54\x41\x55\x41\x57\x48\x83\xEC\x30", "xxxx?xxxxxxxxxxxx");
		} else if (Utils::isExodus()) {
			if (Utils::isExodusPatched) {
				// 44 89 4C 24 ? 44 89 44 24 ? 89 54 24 10 48 89 4C 24 ? 55 53 57 - Exodus NEW
				clevel_r_on_key_press_Address = FindPatternInEXE(
					"\x44\x89\x4C\x24\x00\x44\x89\x44\x24\x00\x89\x54\x24\x10\x48\x89\x4C\x24\x00\x55\x53\x57", "xxxx?xxxx?xxxxxxxx?xxx");
			} else {
				// 44 89 4c 24 ? 48 89 4c 24 ? 55 53 - Exodus OLD
				clevel_r_on_key_press_Address = FindPatternInEXE("\x44\x89\x4c\x24\x00\x48\x89\x4c\x24\x00\x55\x53", "xxxx?xxxx?xx");
			}
		}

		SetHook("clevel_r_on_key_press", (void*)clevel_r_on_key_press_Address, &clevel_r_on_key_press_Hook, &clevel_r_on_key_press_Orig);

		///////////////////////////////////////////////////////////////

		if (g_restore_deleted_commands) {
			DWORD64 cmd_register_commands_Address = NULL;

			if (Utils::isRedux()) {
				// 48 89 5C 24 ? 57 48 83 EC 20 8B 05 ? ? ? ? 48 8D 1D ? ? ? ? 48 8D 3D ? ? ? ? A8 01 75 60 - Redux STEAM
				cmd_register_commands_Address = FindPatternInEXE(
					"\x48\x89\x5C\x24\x00\x57\x48\x83\xEC\x20\x8B\x05\x00\x00\x00\x00\x48\x8D\x1D\x00\x00\x00\x00\x48\x8D\x3D\x00\x00\x00\x00\xA8\x01\x75\x60",
					"xxxx?xxxxxxx????xxx????xxx????xxxx");

				if (cmd_register_commands_Address == NULL) {
					// 40 53 48 83 EC 20 65 48 8B 04 25 ? ? ? ? 8B 0D ? ? ? ? BA ? ? ? ? 48 8B 1C C8 48 03 DA 8B 03 39 05 - Redux EGS
					cmd_register_commands_Address = FindPatternInEXE(
						"\x40\x53\x48\x83\xEC\x20\x65\x48\x8B\x04\x25\x00\x00\x00\x00\x8B\x0D\x00\x00\x00\x00\xBA\x00\x00\x00\x00\x48\x8B\x1C\xC8\x48\x03\xDA\x8B\x03\x39\x05",
						"xxxxxxxxxxx????xx????x????xxxxxxxxxxx");
				}
			} else if (Utils::isArktika() || Utils::isExodus()) {
				// 40 53 48 83 EC 20 65 48 8B 04 25 ? ? ? ? 8B 0D ? ? ? ? BA ? ? ? ? 48 8B 1C C8 48 03 DA 8B 03 39 05 - Arktika and Exodus
				cmd_register_commands_Address = FindPatternInEXE(
					"\x40\x53\x48\x83\xEC\x20\x65\x48\x8B\x04\x25\x00\x00\x00\x00\x8B\x0D\x00\x00\x00\x00\xBA\x00\x00\x00\x00\x48\x8B\x1C\xC8\x48\x03\xDA\x8B\x03\x39\x05",
					"xxxxxxxxxxx????xx????x????xxxxxxxxxxx");
			}

			SetHook("cmd_register_commands", (void*)cmd_register_commands_Address, &cmd_register_commands_Hook, &cmd_register_commands_Orig);
		}

		///////////////////////////////////////////////////////////////

		bool isReduxEGS = false;

		if (g_unlock_content_folder || g_cant_find_file_msg) {
			DWORD64 vfs_ropen_package_Address = NULL;
			DWORD64 vfs_rbuffered_package_Address = NULL;
			DWORD64 vfs_package_registry_level_downloaded_Address = NULL;
			DWORD64 asset_manager_load_all_internal_Address = NULL;
			DWORD64 mov_g_texture_manager = NULL;

			if (Utils::isRedux()) {
				// 48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 54 41 56 41 57 48 83 EC ? 45 33 E4 45 8B F9 - Redux STEAM
				vfs_ropen_package_Address = FindPatternInEXE(
					"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x56\x57\x41\x54\x41\x56\x41\x57\x48\x83\xEC\x00\x45\x33\xE4\x45\x8B\xF9",
					"xxxx?xxxx?xxxxxxxxxxx?xxxxxx");

				if (vfs_ropen_package_Address != NULL) {
					// 48 89 5C 24 ? 48 89 74 24 ? 41 56 48 83 EC ? 83 79 - Redux STEAM
					vfs_rbuffered_package_Address = FindPatternInEXE("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x41\x56\x48\x83\xEC\x00\x83\x79", "xxxx?xxxx?xxxxx?xx");

					// 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 54 41 56 41 57 48 83 EC 20 44 0F B7 B9 ? ? ? ? 33 DB 4C 8B F2 4C 8B E1 45 85 FF 74 6E - Redux ONLY ON STEAM
					vfs_package_registry_level_downloaded_Address = FindPatternInEXE(
						"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x41\x54\x41\x56\x41\x57\x48\x83\xEC\x20\x44\x0F\xB7\xB9\x00\x00\x00\x00\x33\xDB\x4C\x8B\xF2\x4C\x8B\xE1\x45\x85\xFF\x74\x6E",
						"xxxx?xxxx?xxxx?xxxx?xxxxxxxxxxxxxx????xxxxxxxxxxxxx");

					// 48 8b c4 48 89 58 ? 48 89 70 ? 48 89 78 ? 48 89 48 ? 55 48 8d a8 ? ? ? ? b8 - Redux STEAM
					asset_manager_load_all_internal_Address = FindPatternInEXE(
						"\x48\x8b\xc4\x48\x89\x58\x00\x48\x89\x70\x00\x48\x89\x78\x00\x48\x89\x48\x00\x55\x48\x8d\xa8\x00\x00\x00\x00\xb8",
						"xxxxxx?xxx?xxx?xxx?xxxx????x");

					mov_g_texture_manager = asset_manager_load_all_internal_Address + 0x28; // Redux STEAM
				} else {
					isReduxEGS = true;

					// 48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 54 41 56 41 57 48 83 EC 30 45 33 E4 41 8B E9 49 8B F0 48 8B DA 4C 8B F1 44 39 62 0C 0F 84 ? ? ? ? 4D 8B C8 4C 89 64 24 ? 4C 8B 02 41 BF ? ? ? ? 33 D2 - Redux EGS
					vfs_ropen_package_Address = FindPatternInEXE(
						"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x56\x57\x41\x54\x41\x56\x41\x57\x48\x83\xEC\x30\x45\x33\xE4\x41\x8B\xE9\x49\x8B\xF0\x48\x8B\xDA\x4C\x8B\xF1\x44\x39\x62\x0C\x0F\x84\x00\x00\x00\x00\x4D\x8B\xC8\x4C\x89\x64\x24\x00\x4C\x8B\x02\x41\xBF\x00\x00\x00\x00\x33\xD2",
						"xxxx?xxxx?xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx????xxxxxxx?xxxxx????xx");

					// 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 41 56 48 83 EC 40 48 8B 1D ? ? ? ? 48 8B F1 48 8B 6A 08 4C 8B 32 48 85 DB 75 0F E8 ? ? ? ? 48 8B D8 48 89 05 - Redux EGS
					vfs_rbuffered_package_Address = FindPatternInEXE(
						"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x41\x56\x48\x83\xEC\x40\x48\x8B\x1D\x00\x00\x00\x00\x48\x8B\xF1\x48\x8B\x6A\x08\x4C\x8B\x32\x48\x85\xDB\x75\x0F\xE8\x00\x00\x00\x00\x48\x8B\xD8\x48\x89\x05",
						"xxxx?xxxx?xxxx?xxxxxxxxx");

					// 48 8B C4 48 89 58 10 48 89 78 18 48 89 48 08 55 48 8D A8 ? ? ? ? B8 ? ? ? ? E8 - Redux EGS
					asset_manager_load_all_internal_Address = FindPatternInEXE(
						"\x48\x8B\xC4\x48\x89\x58\x10\x48\x89\x78\x18\x48\x89\x48\x08\x55\x48\x8D\xA8\x00\x00\x00\x00\xB8\x00\x00\x00\x00\xE8",
						"xxxxxxxxxxxxxxxxxxx????x????x");

					mov_g_texture_manager = asset_manager_load_all_internal_Address + 0x24; // Redux EGS
				}

				ContentUnlocker::g_texture_manager = (void**)Utils::GetAddrFromRelativeInstr(mov_g_texture_manager, 7, 3);
			} else if (Utils::isArktika()) {
				// 48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 54 41 56 41 57 48 83 EC 40 45 33 E4 45 8B F9 4D 8B F0 48 8B DA 48 8B F9 44 39 62 0C 0F 84 ? ? ? ? 4D 8B C8 4C 89 64 24 ? 4C 8B 02 BD ? ? ? ? 33 D2 89 6C 24 20 48 8B CB E8 ? ? ? ? 48 8B F0 - Arktika
				vfs_ropen_package_Address = FindPatternInEXE(
					"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x56\x57\x41\x54\x41\x56\x41\x57\x48\x83\xEC\x40\x45\x33\xE4\x45\x8B\xF9\x4D\x8B\xF0\x48\x8B\xDA\x48\x8B\xF9\x44\x39\x62\x0C\x0F\x84\x00\x00\x00\x00\x4D\x8B\xC8\x4C\x89\x64\x24\x00\x4C\x8B\x02\xBD\x00\x00\x00\x00\x33\xD2\x89\x6C\x24\x20\x48\x8B\xCB\xE8\x00\x00\x00\x00\x48\x8B\xF0",
					"xxxx?xxxx?xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx????xxxxxxx?xxxx????xxxxxxxxxx????xxx");

				// 48 89 5C 24 ? 48 89 74 24 ? 41 56 48 83 EC 40 83 79 0C 00 4D 8B F0 48 8B F2 48 8B D9 0F 84 ? ? ? ? 4C 8B 01 4C 8B CA 33 D2 48 C7 44 24 ? ? ? ? ? 48 89 7C 24 ? C7 44 24 ? ? ? ? ? E8 ? ? ? ? 48 8B F8 48 85 C0 74 7A 8B 48 18 8D 0C 49 - Arktika
				vfs_rbuffered_package_Address = FindPatternInEXE(
					"\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x41\x56\x48\x83\xEC\x40\x83\x79\x0C\x00\x4D\x8B\xF0\x48\x8B\xF2\x48\x8B\xD9\x0F\x84\x00\x00\x00\x00\x4C\x8B\x01\x4C\x8B\xCA\x33\xD2\x48\xC7\x44\x24\x00\x00\x00\x00\x00\x48\x89\x7C\x24\x00\xC7\x44\x24\x00\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8B\xF8\x48\x85\xC0\x74\x7A\x8B\x48\x18\x8D\x0C\x49",
					"xxxx?xxxx?xxxxxxxxxxxxxxxxxxxxx????xxxxxxxxxxxx?????xxxx?xxx?????x????xxxxxxxxxxxxxx");

				// 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 54 41 56 41 57 48 83 EC 20 44 0F B7 B9 ? ? ? ? 33 DB 4C 8B F2 4C 8B E1 45 85 FF 74 6D 33 FF 66 66 66 0F 1F 84 00 ? ? ? ? 46 8B 9C 27 - Arktika
				//vfs_package_registry_level_downloaded_Address = FindPatternInEXE(
				//	"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x41\x54\x41\x56\x41\x57\x48\x83\xEC\x20\x44\x0F\xB7\xB9\x00\x00\x00\x00\x33\xDB\x4C\x8B\xF2\x4C\x8B\xE1\x45\x85\xFF\x74\x6D\x33\xFF\x66\x66\x66\x0F\x1F\x84\x00\x00\x00\x00\x00\x46\x8B\x9C\x27",
				//	"xxxx?xxxx?xxxx?xxxx?xxxxxxxxxxxxxx????xxxxxxxxxxxxxxxxxxxxxx????xxxx");

				// 48 89 5C 24 ? 48 89 74 24 ? 55 57 41 56 48 8D AC 24 ? ? ? ? B8 ? ? ? ? E8 ? ? ? ? 48 2B E0 48 8B 81 ? ? ? ? 45 33 F6 - Arktika
				asset_manager_load_all_internal_Address = FindPatternInEXE(
					"\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x55\x57\x41\x56\x48\x8D\xAC\x24\x00\x00\x00\x00\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x2B\xE0\x48\x8B\x81\x00\x00\x00\x00\x45\x33\xF6",
					"xxxx?xxxx?xxxxxxxx????x????x????xxxxxx????xxx");

				// 48 8B 0D ? ? ? ? E8 ? ? ? ? 49 8B CD E8 ? ? ? ? 66 0F 6F 05 ? ? ? ? 48 8D 3D ? ? ? ? 48 8B 05 ? ? ? ? 4C 8D 4D F0 41 B8 ? ? ? ? 48 8D 55 98 48 8B CF F3 0F 7F 45 ? FF 90 ? ? ? ? 45 33 FF 48 8B 18 48 85 DB 74 09 48 8B 03 48 8B CB FF 50 08 - Arktika
				mov_g_texture_manager = FindPatternInEXE(
					"\x48\x8B\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x49\x8B\xCD\xE8\x00\x00\x00\x00\x66\x0F\x6F\x05\x00\x00\x00\x00\x48\x8D\x3D\x00\x00\x00\x00\x48\x8B\x05\x00\x00\x00\x00\x4C\x8D\x4D\xF0\x41\xB8\x00\x00\x00\x00\x48\x8D\x55\x98\x48\x8B\xCF\xF3\x0F\x7F\x45\x00\xFF\x90\x00\x00\x00\x00\x45\x33\xFF\x48\x8B\x18\x48\x85\xDB\x74\x09\x48\x8B\x03\x48\x8B\xCB\xFF\x50\x08",
					"xxx????x????xxxx????xxxx????xxx????xxx????xxxxxx????xxxxxxxxxxx?xx????xxxxxxxxxxxxxxxxxxxx");

				ContentUnlocker::g_texture_manager = (void**)Utils::GetAddrFromRelativeInstr(mov_g_texture_manager, 7, 3);
			} else if (Utils::isExodus()) {
				if (Utils::isExodusPatched) {
					// 48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 54 41 55 41 57 48 83 EC 40 45 33 ED 45 8B E1 49 8B E8 48 8B FA 48 8B F1 44 39 6A 0C 0F 84 ? ? ? ? 48 8B 84 24 ? ? ? ? 4D 8B C8 4C 8B 02 41 BF - Exodus
					vfs_ropen_package_Address = FindPatternInEXE(
						"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x56\x57\x41\x54\x41\x55\x41\x57\x48\x83\xEC\x40\x45\x33\xED\x45\x8B\xE1\x49\x8B\xE8\x48\x8B\xFA\x48\x8B\xF1\x44\x39\x6A\x0C\x0F\x84\x00\x00\x00\x00\x48\x8B\x84\x24\x00\x00\x00\x00\x4D\x8B\xC8\x4C\x8B\x02\x41\xBF",
						"xxxx?xxxx?xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx????xxxx????xxxxxxxx");

					SetHook("vfs_ropen_package", (void*)vfs_ropen_package_Address, &vfs_ropen_package_ExodusPatched_Hook, &vfs_ropen_package_Orig);
				} else {
					// 48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 54 41 55 41 56 48 83 EC 30 45 31 ED - Exodus OLD
					vfs_ropen_package_Address = FindPatternInEXE(
						"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x56\x57\x41\x54\x41\x55\x41\x56\x48\x83\xEC\x30\x45\x31\xED", "xxxx?xxxx?xxxxxxxxxxxxxxx");

					SetHook("vfs_ropen_package", (void*)vfs_ropen_package_Address, &vfs_ropen_package_Hook, &vfs_ropen_package_Orig);
				}

				// E8 ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 8B 7E 34 - Exodus ALL
				DWORD64 call_vfs_rbuffered_package = FindPatternInEXE("\xE8\x00\x00\x00\x00\x48\x8B\x05\x00\x00\x00\x00\x8B\x7E\x34", "x????xxx????xxx");
				vfs_rbuffered_package_Address = Utils::GetAddrFromRelativeInstr(call_vfs_rbuffered_package, 5, 1);

				// E8 ? ? ? ? 41 83 CF FF 85 C0 - Exodus ALL
				DWORD64 call_vfs_exists = FindPatternInEXE("\xE8\x00\x00\x00\x00\x41\x83\xCF\xFF\x85\xC0", "x????xxxxxx");
				vfs_exists = (_vfs_exists)Utils::GetAddrFromRelativeInstr(call_vfs_exists, 5, 1);
			}

			if (g_cant_find_file_msg && (Utils::isRedux() || Utils::isArktika())) {
				// 48 89 5C 24 ? 57 48 83 EC 30 48 8B F9 E8 ? ? ? ? 33 DB 39 58 0C 74 2C 4C 8B 00 - Redux STEAM
				vfs_exists = (_vfs_exists)FindPatternInEXE(
					"\x48\x89\x5C\x24\x00\x57\x48\x83\xEC\x30\x48\x8B\xF9\xE8\x00\x00\x00\x00\x33\xDB\x39\x58\x0C\x74\x2C\x4C\x8B\x00",
					"xxxx?xxxxxxxxx????xxxxxxxxxx");

				if (vfs_exists == NULL) {
					// 48 89 5C 24 ? 57 48 83 EC 30 48 8B 05 ? ? ? ? 48 8B F9 48 85 C0 75 0C E8 ? ? ? ? 48 89 05 ? ? ? ? 33 DB - Redux EGS
					vfs_exists = (_vfs_exists)FindPatternInEXE(
						"\x48\x89\x5C\x24\x00\x57\x48\x83\xEC\x30\x48\x8B\x05\x00\x00\x00\x00\x48\x8B\xF9\x48\x85\xC0\x75\x0C\xE8\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x33\xDB",
						"xxxx?xxxxxxxx????xxxxxxxxx????xxx????xx");
				}
			}

			if (!Utils::isExodus()) {
				SetHook("vfs_ropen_package", (void*)vfs_ropen_package_Address, &vfs_ropen_package_Hook, &vfs_ropen_package_Orig);
			}

			SetHook("vfs_rbuffered_package", (void*)vfs_rbuffered_package_Address, Utils::isExodus() ? (void*)&vfs_rbuffered_package_HookExodus : (void*)&vfs_rbuffered_package_Hook, &vfs_rbuffered_package_Orig);

			if (g_unlock_content_folder) {
				if (Utils::isRedux() && !isReduxEGS) {
					// в EGS версии эта функция заинлайнена
					SetHook("vfs_package_registry_level_downloaded", (void*)vfs_package_registry_level_downloaded_Address, &vfs_package_registry_level_downloaded_Hook,
						&vfs_package_registry_level_downloaded_Orig);
				}

				if (Utils::isRedux() || Utils::isArktika()) {
					SetHook("asset_manager_load_all_internal", (void*)asset_manager_load_all_internal_Address, &asset_manager_load_all_internal_Hook,
						&asset_manager_load_all_internal_Orig);
				}
			}
		}

		///////////////////////////////////////////////////////////////

		if (Utils::GetBool("log", "enabled", false)) {
			DWORD64 slog_Address;

			if (Utils::isRedux()) {
				// 40 53 B8 ? ? ? ? E8 ? ? ? ? 48 2B E0 33 C0 - Redux STEAM
				slog_Address = FindPatternInEXE("\x40\x53\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x2B\xE0\x33\xC0", "xxx????x????xxxxx");

				if (slog_Address == NULL) {
					// 48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 56 B8 ? ? ? ? E8 - Redux EGS
					slog_Address = FindPatternInEXE("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x56\x57\x41\x56\xB8\x00\x00\x00\x00\xE8", "xxxx?xxxx?xxxxx????x");
				}
			} else if (Utils::isArktika()) {
				// 48 89 5C 24 ? 57 B8 ? ? ? ? E8 ? ? ? ? 48 2B E0 48 8B D9 E8 ? ? ? ? 33 D2 BF ? ? ? ? 38 13 74 5C 33 C0 0F B6 0B 80 F9 0A 75 41 C6 44 04 ? ? 80 7C 24 ? ? 75 07 66 C7 44 24 - Arktika
				slog_Address = FindPatternInEXE(
					"\x48\x89\x5C\x24\x00\x57\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x2B\xE0\x48\x8B\xD9\xE8\x00\x00\x00\x00\x33\xD2\xBF\x00\x00\x00\x00\x38\x13\x74\x5C\x33\xC0\x0F\xB6\x0B\x80\xF9\x0A\x75\x41\xC6\x44\x04\x00\x00\x80\x7C\x24\x00\x00\x75\x07\x66\xC7\x44\x24",
					"xxxx?xx????x????xxxxxxx????xxx????xxxxxxxxxxxxxxxxx??xxx??xxxxxx");
			} else if (Utils::isExodus()) {
				if (Utils::isExodusPatched) {
					// 40 56 B8 ? ? ? ? E8 ? ? ? ? 48 2B E0 48 8B F1 - Exodus
					slog_Address = FindPatternInEXE("\x40\x56\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x2B\xE0\x48\x8B\xF1", "xxx????x????xxxxxx");
				} else {
					// 40 53 B8 ? ? ? ? E8 ? ? ? ? 48 29 C4 48 89 CB - Exodus OLD
					slog_Address = FindPatternInEXE("\x40\x53\xb8\x00\x00\x00\x00\xe8\x00\x00\x00\x00\x48\x29\xc4\x48\x89\xcb", "xxx????x????xxxxxx");
				}
			}

			SetHook("slog", (void*)slog_Address, &slog_Hook, &slog_Orig);
		}

		///////////////////////////////////////////////////////////////

		if (Utils::isArktika()) {
			if (Utils::GetBool("arktika1", "walking_hack", false)) {
				// 48 81 c4 ? ? ? ? 5e 5b 5d c3 cc cc cc cc cc cc cc cc cc cc cc cc cc cc cc cc cc 48 8b c4 44 89 48 - Arktika
				DWORD64 cplayer_process_state_end_Address = FindPatternInEXE(
					"\x48\x81\xc4\x00\x00\x00\x00\x5e\x5b\x5d\xc3\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc\xcc\x48\x8b\xc4\x44\x89\x48",
					"xxx????xxxxxxxxxxxxxxxxxxxxxxxxxxx");

				SetHook("walking_fix_attempt", (void*)cplayer_process_state_end_Address, &walking_fix_attempt, NULL);
			}
		}

		///////////////////////////////////////////////////////////////

		if ((g_fly || g_restore_deleted_commands) && Utils::isExodus() && Utils::isExodusPatched) {
			// Восстановление cflycam::r_on_key_press выпиленного в патчах исхода
			// C2 00 00 CC CC CC CC CC CC CC CC CC CC CC CC CC 48 89 5C 24 ? 57 48 83 EC 20 48 8B 41 10 - Exodus
			DWORD64 cflycam_r_on_key_press_Address = FindPatternInEXE(
				"\xC2\x00\x00\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48\x89\x5C\x24\x00\x57\x48\x83\xEC\x20\x48\x8B\x41\x10",
				"xxxxxxxxxxxxxxxxxxxx?xxxxxxxxx");

			SetHook("cflycam_r_on_key_press", (void*)cflycam_r_on_key_press_Address, &Fly::exodus_cflycam_r_on_key_press, NULL);
		}

		///////////////////////////////////////////////////////////////

		if (Utils::GetBool(OTHER, "bugtrap", false)) {
			DWORD64 u_platform_initialize_end_Address = NULL;

			if (Utils::isRedux() || Utils::isArktika()) {
				// 48 89 5C 24 ? 48 89 4C 24 ? 57 48 83 EC 30 0F 29 74 24 ? FF 15 - Redux STEAM and Arktika
				u_platform_initialize_end_Address = FindPatternInEXE("\x48\x89\x5C\x24\x00\x48\x89\x4C\x24\x00\x57\x48\x83\xEC\x30\x0F\x29\x74\x24\x00\xFF\x15",
					"xxxx?xxxx?xxxxxxxxx?xx");

				if (Utils::isRedux()) {
					if (isReduxEGS) {
						// 48 8B C4 48 89 58 20 48 89 48 08 57 48 83 EC 50 0F 29 70 E8 0F 29 78 D8 44 0F 29 40 ? FF 15 - Redux EGS
						u_platform_initialize_end_Address = FindPatternInEXE("\x48\x8B\xC4\x48\x89\x58\x20\x48\x89\x48\x08\x57\x48\x83\xEC\x50\x0F\x29\x70\xE8\x0F\x29\x78\xD8\x44\x0F\x29\x40\x00\xFF\x15",
							"xxxxxxxxxxxxxxxxxxxxxxxxxxxx?xx");
					}

					SetHook("u_platform_initialize_end", (void*)u_platform_initialize_end_Address, &u_platform_initialize_end_Hook, &u_platform_initialize_end_Orig);
				} else {
					SetHook("u_platform_initialize_end", (void*)u_platform_initialize_end_Address, &u_platform_initialize_end_HookArktika, &u_platform_initialize_end_Orig);
				}
			}
		}

		///////////////////////////////////////////////////////////////
#endif
	} else {
		MessageBox(NULL, "MinHook not initialized!", "MinHook", MB_OK | MB_ICONERROR);
	}
}

void Hooks::SetHook(char* hookName, void* pTarget, void* pDetour, void* ppOriginal)
{
	MH_STATUS status = MH_CreateHook(pTarget, pDetour, (void**) ppOriginal);

	if (status == MH_OK) {
		if (MH_EnableHook(pTarget) != MH_OK) {
			MessageBox(NULL, "MH_EnableHook() != MH_OK", hookName, MB_OK | MB_ICONERROR);
		}
	} else {
		printf("MH_STATUS = %i\n", status); // TEST
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

void __fastcall Hooks::clevel_r_on_key_press_HookLL(void* _this, void* _unused, int action, int key, int state, int resending) // LL orig
#else
void __fastcall Hooks::clevel_r_on_key_press_Hook(void* _this, int action, int key, int state, int resending) // Redux or new
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

	if (g_restore_deleted_commands) {
		RestoreCommands::cmd_register_commands();
	}

#ifndef _WIN64
	if (Utils::is2033()) {
		wpn_bobbing_la::cmd_register_commands();
	}
#endif
}

///////////////////////////////////////////////////////////////

void Hooks::vfs_ropen_cantfind(const char* fn)
{
	// файлы с расширением .model не выводим, т.к. движок их умеет выводить сам
	const char* ext = strrchr(fn, '.');
	bool b = true;

	if (ext != NULL) {
		b = !strstr(".model", ext);
	}

	if (b) {
		Utils::rlog(cantFindFile, fn);
	}
}

#ifdef _WIN64
void* __fastcall Hooks::vfs_ropen_package_ExodusPatched_Hook(void* result, void* package, const char* fn, const int force_raw, unsigned int* uncompressed_size, void* _unknown)
{
	void* ret = nullptr;

	if (g_unlock_content_folder) {
		ret = ContentUnlocker::vfs_ropen_package(result, fn);
	}

	if (ret == nullptr) {
		ret = ((_vfs_ropen_packageExodus)vfs_ropen_package_Orig)(result, package, fn, force_raw, uncompressed_size, _unknown);
	}

	if (g_cant_find_file_msg && *(void**)ret == nullptr) {
		vfs_ropen_cantfind(fn);
	}

	return ret;
}

void* __fastcall Hooks::vfs_ropen_package_Hook(void* result, void* package, const char* fn, const int force_raw, unsigned int* uncompressed_size)
{
	void* ret = nullptr;

	if (g_unlock_content_folder) {
		ret = ContentUnlocker::vfs_ropen_package(result, fn);
	}

	if (ret == nullptr) {
		ret = ((_vfs_ropen_package)vfs_ropen_package_Orig)(result, package, fn, force_raw, uncompressed_size);
	}

	if (g_cant_find_file_msg && *(void**)ret == nullptr) {
		vfs_ropen_cantfind(fn);
	}

	return ret;
}

void __fastcall Hooks::vfs_rbuffered_package_Hook(void* package, const char* fn, void* cb, const int force_raw)
{
	if (g_cant_find_file_msg && (!vfs_exists(fn) && !Utils::FileExists(fn))) {
		Utils::rlog(cantFindFile, fn);
	}

	ContentUnlocker::vfs_rbuffered_package(fn, (fastdelegate*)cb);
	((_vfs_rbuffered_package)vfs_rbuffered_package_Orig)(package, fn, cb, force_raw);
}

void __fastcall Hooks::vfs_rbuffered_package_HookExodus(const char* fn, void* cb)
{
	if (g_cant_find_file_msg && (!vfs_exists(fn) && !Utils::FileExists(fn))) {
		Utils::rlog(cantFindFile, fn);
	}

	ContentUnlocker::vfs_rbuffered_package(fn, (fastdelegate*)cb);
	((_vfs_rbuffered_packageExodus)vfs_rbuffered_package_Orig)(fn, cb);
}

bool __fastcall Hooks::vfs_package_registry_level_downloaded_Hook(void* _this, const char* map_name)
{
	return ContentUnlocker::vfs_package_registry_level_downloaded(vfs_package_registry_level_downloaded_Orig(_this, map_name), map_name);
}

void __fastcall Hooks::asset_manager_load_all_internal_Hook(void* _this, const int create_progress, DWORD64 _unused, int files)
{
	asset_manager_load_all_internal_Orig(_this, create_progress, _unused, files);

	// add handles fom content folder
	ContentUnlocker::textures_bin_process_dir(_this, "");
}

#else

// void* __usercall vfs::ropen@<eax>(void* result@<esi>, const char *fn)
__declspec(naked) void Hooks::vfs_ropen_HookASM()
{
	__asm {
		// === БЕРЁМ АРГУМЕНТЫ ===
		mov eax, [esp + 4]   // fn
		mov edx, esi         // result

		// === СОХРАНЯЕМ РЕГИСТРЫ ===
		pushad

		// передаём аргументы в C функцию
		push eax             // fn
		push edx             // result
		call Hooks::vfs_ropen_HookC
		add esp, 8

		// кладём return value в сохранённый eax
		mov[esp + 28], eax

		popad
		ret
	}
}

void* __cdecl Hooks::vfs_ropen_HookC(void* result, const char* fn)
{
	void* ret = nullptr;

	if (g_unlock_content_folder) {
		ret = ContentUnlocker::vfs_ropen(result, fn);
	}

	if (ret == nullptr) {
		// вызов оригинальной функции в 2033 и в LL __usercall
		__asm {
			mov esi, result
			push fn
			call vfs_ropen_Orig
			add esp, 4
			mov ret, eax
		}
	}

	if (g_cant_find_file_msg && *(void**)ret == nullptr) {
		vfs_ropen_cantfind(fn);
	}

	return ret;
}

void __cdecl Hooks::vfs_rbuffered_Hook(const char* fn, void* a1, void* method)
{
	if (g_cant_find_file_msg && (!vfs_exists(fn) && !Utils::FileExists(fn))) {
		Utils::rlog(cantFindFile, fn);
	}

	ContentUnlocker::vfs_rbuffered(fn, a1, (_method)method);
	vfs_rbuffered_Orig(fn, a1, method);
}

// т.к. в версии без патчей функция vfs::exists заинлайнена везде, то мы сделаем эту функцию сами
bool __cdecl Hooks::vfs_exists_custom_ll(const char* fn)
{
	if (vfs && vfs_registry && vfs_package_registry_find) {
		void* p = *vfs;
		if (!p) {
			p = vfs_registry();
			*vfs = p;
		}

		size_t _size = ((size_t*)p)[2];
		return _size && vfs_package_registry_find(p, 0, *(void**)p, fn, 0);
	} else {
		return true;
	}
}

#endif

///////////////////////////////////////////////////////////////

void* __cdecl Hooks::u_platform_initialize_end_Hook()
{
	void* ret = ((_u_platform_initialize_end)u_platform_initialize_end_Orig)();

	BugTrap::bugtrap_attach_process();

	return ret;
}

#ifdef _WIN64
void __fastcall Hooks::u_platform_initialize_end_HookArktika(void* _this)
{
	((_u_platform_initialize_end_Arktika)u_platform_initialize_end_Orig)(_this);

	BugTrap::bugtrap_attach_process();
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

#ifndef _WIN64
void __declspec(naked) Hooks::chud_item__update_hud_position_EndFuncHook(void)
{
	__asm
	{
		pusha;

		lea eax, [esi + 0D0h];
		push eax;
		call wpn_bobbing_la::do_bobbing;
		add esp, 4;

		popa;
		ret;
	}
}
#endif

///////////////////////////////////////////////////////////////
