#include "Fly.h"
#include "Utils.h"

typedef void(__fastcall*** _track)(void*);
#ifdef _WIN64
typedef void (__fastcall* _force_transform)(void* _this, float* m, int on_load);
typedef void* (__fastcall* _entities_core__ser_lock)(void* _this, const char* dbg_info);
_entities_core__ser_lock entities_core__ser_lock = nullptr;

typedef _track(__fastcall* _cflycam_cflycam)(void* _this, const char* name, float slide, float scale);
typedef LPCRITICAL_SECTION(__fastcall* _memory)();
typedef void* (__fastcall* _tlsf_memalign)(DWORD64 tlsf, DWORD align, DWORD size);
typedef void* (__fastcall* _camera_manager_play_track_redux)(DWORD64 _this, void* t, float accrue, float start_pos, void* unused3, void* e, void* unused4, void* unused5, void* owner);
typedef void* (__fastcall* _camera_manager_play_track_arktika_and_exodus)(DWORD64 _this, void* t, float accrue, float start_pos, void* owner);
_tlsf_memalign tlsf_memalign = nullptr;
DWORD64 g_game = NULL;
#else
typedef _track(__thiscall* _cflycam_cflycam)(void* _this, const char* name);
typedef LPCRITICAL_SECTION(__cdecl* _memory)();
typedef void* (__thiscall* _camera_manager_play_track)(DWORD _this, void* t, double hz, void* owner);
void* tlsf_memalign = nullptr;
DWORD g_game = NULL;
static bool isLL = true;
#endif
_cflycam_cflycam cflycam_cflycam = nullptr;
_memory memory = nullptr;
void* camera_manager_play_track = nullptr;

Fly::Fly()
{
	if (Utils::GetBool("other", "fly", false))
	{
#ifdef _WIN64
		if (Utils::GetGame() == GAME::REDUX) {
			// 48 8B C4 48 89 58 10 48 89 68 18 56 57 41 54 41 56 41 57 48 81 EC ? ? ? ? 0F 29 70 C8 0F 29 78 B8 44 0F 29 40 - Redux STEAM
			cflycam_cflycam = (_cflycam_cflycam)FindPatternInEXE(
				(BYTE*)"\x48\x8B\xC4\x48\x89\x58\x10\x48\x89\x68\x18\x56\x57\x41\x54\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x0F\x29\x70\xC8\x0F\x29\x78\xB8\x44\x0F\x29\x40",
				"xxxxxxxxxxxxxxxxxxxxxx????xxxxxxxxxxxx");

			if (cflycam_cflycam == NULL) {
				// 48 8B C4 48 89 58 10 48 89 68 18 48 89 70 20 57 41 56 41 57 48 81 EC ? ? ? ? 0F 29 70 D8 48 8D B1 - Redux EGS
				cflycam_cflycam = (_cflycam_cflycam)FindPatternInEXE(
					(BYTE*)"\x48\x8B\xC4\x48\x89\x58\x10\x48\x89\x68\x18\x48\x89\x70\x20\x57\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x0F\x29\x70\xD8\x48\x8D\xB1",
					"xxxxxxxxxxxxxxxxxxxxxxx????xxxxxxx");
			}

			// 48 83 EC 28 48 8B 05 ? ? ? ? 48 85 C0 75 7F 8B 05 ? ? ? ? 48 89 5C 24 ? A8 01 75 1A 83 C8 01 89 05 ? ? ? ? E8 ? ? ? ? 48 8D 0D ? ? ? ? E8 - Redux STEAM
			memory = (_memory)FindPatternInEXE(
				(BYTE*)"\x48\x83\xEC\x28\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x75\x7F\x8B\x05\x00\x00\x00\x00\x48\x89\x5C\x24\x00\xA8\x01\x75\x1A\x83\xC8\x01\x89\x05\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\xE8",
				"xxxxxxx????xxxxxxx????xxxx?xxxxxxxxx????x????xxx????x");

			if (memory == NULL) {
				// 48 83 EC 28 48 8B 05 ? ? ? ? 48 85 C0 0F 85 ? ? ? ? 65 48 8B 04 25 ? ? ? ? 48 89 5C 24 ? 48 89 6C 24 ? 48 8D 2D ? ? ? ? 48 89 74 24 ? 33 F6 48 8B 08 BA ? ? ? ? 48 89 7C 24 ? 8B 04 0A 39 05 ? ? ? ? 0F 8F - Redux EGS
				memory = (_memory)FindPatternInEXE(
					(BYTE*)"\x48\x83\xEC\x28\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x0F\x85\x00\x00\x00\x00\x65\x48\x8B\x04\x25\x00\x00\x00\x00\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x8D\x2D\x00\x00\x00\x00\x48\x89\x74\x24\x00\x33\xF6\x48\x8B\x08\xBA\x00\x00\x00\x00\x48\x89\x7C\x24\x00\x8B\x04\x0A\x39\x05\x00\x00\x00\x00\x0F\x8F",
					"xxxxxxx????xxxxx????xxxxx????xxxx?xxxx?xxx????xxxx?xxxxxx????xxxx?xxxxx????xx");
			}

			// 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 20 49 8D 40 FF 41 B9 ? ? ? ? 33 F6 48 8B FA 48 8B E9 49 3B C1 77 14 - Redux STEAM
			tlsf_memalign = (_tlsf_memalign)FindPatternInEXE(
				(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\x49\x8D\x40\xFF\x41\xB9\x00\x00\x00\x00\x33\xF6\x48\x8B\xFA\x48\x8B\xE9\x49\x3B\xC1\x77\x14",
				"xxxx?xxxx?xxxx?xxxxxxxxxxx????xxxxxxxxxxxxx");

			if (tlsf_memalign == NULL) {
				// 48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8B DA 49 8D 40 FF 33 D2 41 B9 ? ? ? ? 48 8B F1 8B FA 49 3B C1 77 14 BF ? ? ? ? 49 8D 40 07 48 83 E0 F8 48 3B C7 48 0F 47 F8 - Redux EGS
				tlsf_memalign = (_tlsf_memalign)FindPatternInEXE(
					(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\x48\x8B\xDA\x49\x8D\x40\xFF\x33\xD2\x41\xB9\x00\x00\x00\x00\x48\x8B\xF1\x8B\xFA\x49\x3B\xC1\x77\x14\xBF\x00\x00\x00\x00\x49\x8D\x40\x07\x48\x83\xE0\xF8\x48\x3B\xC7\x48\x0F\x47\xF8",
					"xxxx?xxxx?xxxxxxxxxxxxxxxx????xxxxxxxxxxx????xxxxxxxxxxxxxxx");
			}

			// 48 89 5C 24 ? 48 89 54 24 ? 57 48 83 EC 60 48 8B 02 48 8B DA 48 8B 94 24 ? ? ? ? 0F 29 74 24 ? 0F 29 7C 24 ? 0F 28 F3 48 8B F9 48 8B CB 0F 28 FA FF 90 - Redux STEAM
			camera_manager_play_track = (void*)FindPatternInEXE(
				(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x54\x24\x00\x57\x48\x83\xEC\x60\x48\x8B\x02\x48\x8B\xDA\x48\x8B\x94\x24\x00\x00\x00\x00\x0F\x29\x74\x24\x00\x0F\x29\x7C\x24\x00\x0F\x28\xF3\x48\x8B\xF9\x48\x8B\xCB\x0F\x28\xFA\xFF\x90",
				"xxxx?xxxx?xxxxxxxxxxxxxxx????xxxx?xxxx?xxxxxxxxxxxxxx");

			if (camera_manager_play_track == NULL) {
				// 48 89 5C 24 ? 48 89 74 24 ? 48 89 54 24 ? 57 48 83 EC 50 48 8B 02 48 8B DA 48 8B 94 24 ? ? ? ? 48 8B F9 0F 29 74 24 ? 48 8B CB 0F 29 7C 24 ? 0F 28 F2 0F 28 FB FF 90 - Redux EGS
				camera_manager_play_track = (void*)FindPatternInEXE(
					(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x54\x24\x00\x57\x48\x83\xEC\x50\x48\x8B\x02\x48\x8B\xDA\x48\x8B\x94\x24\x00\x00\x00\x00\x48\x8B\xF9\x0F\x29\x74\x24\x00\x48\x8B\xCB\x0F\x29\x7C\x24\x00\x0F\x28\xF2\x0F\x28\xFB\xFF\x90",
					"xxxx?xxxx?xxxx?xxxxxxxxxxxxxxx????xxxxxxx?xxxxxxx?xxxxxxxx");
			}
		} else if (Utils::GetGame() == GAME::ARKTIKA) {
			// 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? F3 0F 10 1D - Arktika
			cflycam_cflycam = (_cflycam_cflycam)FindPatternInEXE(
				(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00\xF3\x0F\x10\x1D",
				"xxxx?xxxx?xxxx?xxxxxxxxxxxx????xxxx");

			// 48 83 EC 28 48 8B 05 ? ? ? ? 48 85 C0 75 52 48 89 5C 24 ? E8 ? ? ? ? E8 ? ? ? ? 33 DB 48 85 C0 74 0A 48 8B C8 E8 ? ? ? ? EB 03 48 8B C3 48 89 05 - Arktika
			memory = (_memory)FindPatternInEXE(
				(BYTE*)"\x48\x83\xEC\x28\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x75\x52\x48\x89\x5C\x24\x00\xE8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x33\xDB\x48\x85\xC0\x74\x0A\x48\x8B\xC8\xE8\x00\x00\x00\x00\xEB\x03\x48\x8B\xC3\x48\x89\x05",
				"xxxxxxx????xxxxxxxxx?x");

			// 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 20 33 F6 49 8D 40 FF 41 B9 ? ? ? ? 48 8B FA 48 8B E9 49 3B C1 77 14 BE ? ? ? ? 49 8D 40 07 48 83 E0 F8 48 3B C6 48 0F 47 F0 48 8D 0C 16 33 D2 48 8D 41 1F 49 3B C1 - Arktika
			tlsf_memalign = (_tlsf_memalign)FindPatternInEXE(
				(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\x33\xF6\x49\x8D\x40\xFF\x41\xB9\x00\x00\x00\x00\x48\x8B\xFA\x48\x8B\xE9\x49\x3B\xC1\x77\x14\xBE\x00\x00\x00\x00\x49\x8D\x40\x07\x48\x83\xE0\xF8\x48\x3B\xC6\x48\x0F\x47\xF0\x48\x8D\x0C\x16\x33\xD2\x48\x8D\x41\x1F\x49\x3B\xC1",
				"xxxx?xxxx?xxxx?xxxxxxxxxxxxx????xxxxxxxxxxxx????xxxxxxxxxxxxxxxxxxxxxxxxxxxx");

			// 48 89 5C 24 ? 48 89 54 24 ? 57 48 83 EC 50 48 8B 02 48 8B DA 48 8B 94 24 ? ? ? ? 48 8B F9 0F 29 74 24 ? 48 8B CB 0F 29 7C 24 ? 0F 28 F3 0F 28 FA FF 90 ? ? ? ? 48 8B 03 0F 28 D6 0F 28 CF 48 8B CB FF 90 ? ? ? ? 48 8D 05 - Arktika
			camera_manager_play_track = (void*)FindPatternInEXE(
				(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x54\x24\x00\x57\x48\x83\xEC\x50\x48\x8B\x02\x48\x8B\xDA\x48\x8B\x94\x24\x00\x00\x00\x00\x48\x8B\xF9\x0F\x29\x74\x24\x00\x48\x8B\xCB\x0F\x29\x7C\x24\x00\x0F\x28\xF3\x0F\x28\xFA\xFF\x90\x00\x00\x00\x00\x48\x8B\x03\x0F\x28\xD6\x0F\x28\xCF\x48\x8B\xCB\xFF\x90\x00\x00\x00\x00\x48\x8D\x05",
				"xxxx?xxxx?xxxxxxxxxxxxxxx????xxxxxxx?xxxxxxx?xxxxxxxx????xxxxxxxxxxxxxx????xxx");
		} else if (Utils::GetGame() == GAME::EXODUS) {
			// 40 53 55 56 57 41 54 41 56 41 57 48 81 EC ? ? ? ? F3 0F 10 1D ? ? ? ? 48 8D 05 ? ? ? ? F3 0F 10 15 ? ? ? ? 45 - Exodus
			cflycam_cflycam = (_cflycam_cflycam)FindPatternInEXE(
				(BYTE*)"\x40\x53\x55\x56\x57\x41\x54\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00\xF3\x0F\x10\x1D\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\xF3\x0F\x10\x15\x00\x00\x00\x00\x45",
				"xxxxxxxxxxxxxx????xxxx????xxx????xxxx????x");

			// 48 83 EC 28 48 8B 05 ? ? ? ? 48 85 C0 0F 85 ? ? ? ? 65 48 8B 04 25 ? ? ? ? 48 89 5C 24 ? 48 8D 1D ? ? ? ? 48 89 6C 24 ? 48 89 74 24 ? 33 F6 48 8B 08 BA - Exodus
			memory = (_memory)FindPatternInEXE(
				(BYTE*)"\x48\x83\xEC\x28\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x0F\x85\x00\x00\x00\x00\x65\x48\x8B\x04\x25\x00\x00\x00\x00\x48\x89\x5C\x24\x00\x48\x8D\x1D\x00\x00\x00\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x33\xF6\x48\x8B\x08\xBA",
				"xxxxxxx????xxxxx????xxxxx????xxxx?xxx????xxxx?xxxx?xxxxxx");

			// 48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8B DA 49 8D 40 FF 33 D2 41 B9 ? ? ? ? 48 8B F1 8B FA 49 3B C1 77 14 BF - Exodus
			tlsf_memalign = (_tlsf_memalign)FindPatternInEXE(
				(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\x48\x8B\xDA\x49\x8D\x40\xFF\x33\xD2\x41\xB9\x00\x00\x00\x00\x48\x8B\xF1\x8B\xFA\x49\x3B\xC1\x77\x14\xBF",
				"xxxx?xxxx?xxxxxxxxxxxxxxxx????xxxxxxxxxxx");

			// 48 89 6C 24 ? 48 89 54 24 ? 57 48 83 EC 50 48 8B 02 48 8B F9 48 89 5C 24 ? 48 8B DA 48 8B 94 24 ? ? ? ? 48 8B CB 0F 29 74 24 ? 0F 28 F2 0F 29 7C 24 - Exodus
			camera_manager_play_track = (void*)FindPatternInEXE(
				(BYTE*)"\x48\x89\x6C\x24\x00\x48\x89\x54\x24\x00\x57\x48\x83\xEC\x50\x48\x8B\x02\x48\x8B\xF9\x48\x89\x5C\x24\x00\x48\x8B\xDA\x48\x8B\x94\x24\x00\x00\x00\x00\x48\x8B\xCB\x0F\x29\x74\x24\x00\x0F\x28\xF2\x0F\x29\x7C\x24",
				"xxxx?xxxx?xxxxxxxxxxxxxxx?xxxxxxx????xxxxxxx?xxxxxxx");

			// 48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 56 48 83 EC 20 48 8B 1D ? ? ? ? 48 8B F2 48 C7 C7 ? ? ? ? 33 C0 F0 0F B1 BB ? ? ? ? 74 5D 8B 83 ? ? ? ? 85 C0 74 47 8B 0D ? ? ? ? 03 0D ? ? ? ? 74 0E 48 8D 0D ? ? ? ? E8 ? ? ? ? EB DA
			entities_core__ser_lock = (_entities_core__ser_lock)FindPatternInEXE(
				(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x41\x56\x48\x83\xEC\x20\x48\x8B\x1D\x00\x00\x00\x00\x48\x8B\xF2\x48\xC7\xC7\x00\x00\x00\x00\x33\xC0\xF0\x0F\xB1\xBB\x00\x00\x00\x00\x74\x5D\x8B\x83\x00\x00\x00\x00\x85\xC0\x74\x47\x8B\x0D\x00\x00\x00\x00\x03\x0D\x00\x00\x00\x00\x74\x0E\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\xEB\xDA",
				"xxxx?xxxx?xxxx?xxxxxxxxx????xxxxxx????xxxxxx????xxxx????xxxxxx????xx????xxxxx????x????xx");
		}

#else
		isLL = Utils::isLL();

		if (isLL)
		{
			// 55 8B EC 83 E4 F0 81 EC ? ? ? ? 53 56 8B F1 57 33 FF 89 7E 04 89 7E 08 81 66 ? ? ? ? ? C7 06 ? ? ? ? C7 46
			cflycam_cflycam = (_cflycam_cflycam)FindPatternInEXE(
				(BYTE*)"\x55\x8B\xEC\x83\xE4\xF0\x81\xEC\x00\x00\x00\x00\x53\x56\x8B\xF1\x57\x33\xFF\x89\x7E\x04\x89\x7E\x08\x81\x66\x00\x00\x00\x00\x00\xC7\x06\x00\x00\x00\x00\xC7\x46",
				"xxxxxxxx????xxxxxxxxxxxxxxx?????xx????xx");

			// 55 8B EC 83 E4 F8 A1 ? ? ? ? 85 C0 75 48 B8 ? ? ? ? 84 05 ? ? ? ? 75 18 09 05 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 04
			memory = (_memory)FindPatternInEXE(
				(BYTE*)"\x55\x8B\xEC\x83\xE4\xF8\xA1\x00\x00\x00\x00\x85\xC0\x75\x48\xB8\x00\x00\x00\x00\x84\x05\x00\x00\x00\x00\x75\x18\x09\x05\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x68\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x83\xC4\x04",
				"xxxxxxx????xxxxx????xx????xxxx????x????x????x????xxx");

			// 53 8B 5C 24 0C 55 56 33 ED 57 85 C0 74 19 3D ? ? ? ? 73 12 83 C0 03 83 E0 FC 8B E8 83 F8 0C 77 05 BD
			tlsf_memalign = (void*)FindPatternInEXE(
				(BYTE*)"\x53\x8B\x5C\x24\x0C\x55\x56\x33\xED\x57\x85\xC0\x74\x19\x3D\x00\x00\x00\x00\x73\x12\x83\xC0\x03\x83\xE0\xFC\x8B\xE8\x83\xF8\x0C\x77\x05\xBD",
				"xxxxxxxxxxxxxxx");

			// 83 EC 10 53 56 8B 74 24 1C 8B 06 8B 50 44 57 8B F9 8B 4C 24 2C 51 8B CE FF D2 F3 0F 10 44 24 ? 8B 06 8B 50 6C 83 EC 08 F3 0F 11 44 24
			camera_manager_play_track = (_camera_manager_play_track)FindPatternInEXE(
				(BYTE*)"\x83\xEC\x10\x53\x56\x8B\x74\x24\x1C\x8B\x06\x8B\x50\x44\x57\x8B\xF9\x8B\x4C\x24\x2C\x51\x8B\xCE\xFF\xD2\xF3\x0F\x10\x44\x24\x00\x8B\x06\x8B\x50\x6C\x83\xEC\x08\xF3\x0F\x11\x44\x24",
				"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx?xxxxxxxxxxxxx");
		}
#endif
	}
}

void Fly::clevel_r_on_key_press(int action, int key, int state, int resending)
{
	if (key == 65)
	{
#ifdef _WIN64
		LPCRITICAL_SECTION mem = memory();
		++*(DWORD*)((DWORD64)mem + (Utils::GetGame() == GAME::REDUX ? 0x100 : 0x140));
		EnterCriticalSection(mem);
		DWORD64 tlsf = *(DWORD64*)((DWORD64)mem + (Utils::GetGame() == GAME::REDUX ? 0x38 : 0x30));
		void* cflycam_this = tlsf_memalign(tlsf, 0x10, Utils::GetGame() == GAME::EXODUS ? 0x1F0 : 0x120);
		LeaveCriticalSection(mem);

		// TODO: ƒл€ исхода по идее нужно ещЄ 2 float параметра
		_track track = cflycam_cflycam(cflycam_this, "1", 0.2f, 0.125f);

		if (g_game == NULL)
		{
			// читаем адрес инструкции mov rax, cs:g_game
			DWORD64 mov = NULL;
			if (Utils::GetGame() == GAME::REDUX) {
				// 48 8B 05 ? ? ? ? 4C 8D 85 ? ? ? ? 48 8D 95 ? ? ? ? 48 8B 58 10 48 8D 4C 24 ? E8 ? ? ? ? 48 8B CB 48 89 7C 24 ? 0F 57 DB 0F 57 D2 48 8B D0 E8 ? ? ? ? 48 8D 8D ? ? ? ? E9 - Redux STEAM
				mov = FindPatternInEXE(
					(BYTE*)"\x48\x8B\x05\x00\x00\x00\x00\x4C\x8D\x85\x00\x00\x00\x00\x48\x8D\x95\x00\x00\x00\x00\x48\x8B\x58\x10\x48\x8D\x4C\x24\x00\xE8\x00\x00\x00\x00\x48\x8B\xCB\x48\x89\x7C\x24\x00\x0F\x57\xDB\x0F\x57\xD2\x48\x8B\xD0\xE8\x00\x00\x00\x00\x48\x8D\x8D\x00\x00\x00\x00\xE9",
					"xxx????xxx????xxx????xxxxxxxx?x????xxxxxxx?xxxxxxxxxx????xxx????x");

				if (mov == NULL) {
					// 48 8B 05 ? ? ? ? 4C 8D 85 ? ? ? ? 48 8D 95 ? ? ? ? 48 8D 4C 24 ? 48 8B 58 10 E8 ? ? ? ? 48 8B D0 48 89 7C 24 ? 0F 57 DB 0F 57 D2 48 8B CB E8 ? ? ? ? B8 ? ? ? ? E9 ? ? ? ? 48 8B 05 ? ? ? ? 48 85 C0 0F 84 ? ? ? ? 48 8B 88 - Redux EGS
					mov = FindPatternInEXE(
						(BYTE*)"\x48\x8B\x05\x00\x00\x00\x00\x4C\x8D\x85\x00\x00\x00\x00\x48\x8D\x95\x00\x00\x00\x00\x48\x8D\x4C\x24\x00\x48\x8B\x58\x10\xE8\x00\x00\x00\x00\x48\x8B\xD0\x48\x89\x7C\x24\x00\x0F\x57\xDB\x0F\x57\xD2\x48\x8B\xCB\xE8\x00\x00\x00\x00\xB8\x00\x00\x00\x00\xE9\x00\x00\x00\x00\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x0F\x84\x00\x00\x00\x00\x48\x8B\x88",
						"xxx????xxx????xxx????xxxx?xxxxx????xxxxxxx?xxxxxxxxxx????x????x????xxx????xxxxx????xxx");
				}
			} else if (Utils::GetGame() == GAME::ARKTIKA) {
				// 48 8B 05 ? ? ? ? 4C 8D 87 ? ? ? ? 48 8D 97 ? ? ? ? 48 8D 4C 24 ? 48 8B 58 10 E8 ? ? ? ? 48 8B D0 48 89 74 24 ? 0F 57 DB 0F 57 D2 48 8B CB E8 ? ? ? ? 48 8D 8F ? ? ? ? EB 07 48 8D 8F ? ? ? ? E8 - Arktika
				mov = FindPatternInEXE(
					(BYTE*)"\x48\x8B\x05\x00\x00\x00\x00\x4C\x8D\x87\x00\x00\x00\x00\x48\x8D\x97\x00\x00\x00\x00\x48\x8D\x4C\x24\x00\x48\x8B\x58\x10\xE8\x00\x00\x00\x00\x48\x8B\xD0\x48\x89\x74\x24\x00\x0F\x57\xDB\x0F\x57\xD2\x48\x8B\xCB\xE8\x00\x00\x00\x00\x48\x8D\x8F\x00\x00\x00\x00\xEB\x07\x48\x8D\x8F\x00\x00\x00\x00\xE8",
					"xxx????xxx????xxx????xxxx?xxxxx????xxxxxxx?xxxxxxxxxx????xxx????xxxxx????x");
			} else if (Utils::GetGame() == GAME::EXODUS) {
				// 48 8B 05 ? ? ? ? 4C 8D 87 ? ? ? ? 48 8D 97 ? ? ? ? 48 8D 4C 24 ? 48 8B 58 10 E8 ? ? ? ? 48 8B D0 48 89 74 24 ? 0F 57 DB 0F 57 D2 48 8B CB E8 ? ? ? ? B8 ? ? ? ? EB 05 B8 ? ? ? ? 48 8D 0C 38 E8 ? ? ? ? 48 8B 4C 24 - Exodus
				mov = FindPatternInEXE(
					(BYTE*)"\x48\x8B\x05\x00\x00\x00\x00\x4C\x8D\x87\x00\x00\x00\x00\x48\x8D\x97\x00\x00\x00\x00\x48\x8D\x4C\x24\x00\x48\x8B\x58\x10\xE8\x00\x00\x00\x00\x48\x8B\xD0\x48\x89\x74\x24\x00\x0F\x57\xDB\x0F\x57\xD2\x48\x8B\xCB\xE8\x00\x00\x00\x00\xB8\x00\x00\x00\x00\xEB\x05\xB8\x00\x00\x00\x00\x48\x8D\x0C\x38\xE8\x00\x00\x00\x00\x48\x8B\x4C\x24",
					"xxx????xxx????xxx????xxxx?xxxxx????xxxxxxx?xxxxxxxxxx????x????xxx????xxxxx????xxxx");
			}

			// вычисл€ем адрес и получаем g_game
			g_game = *(DWORD64*)(mov + 7 + *(DWORD*)(mov + 3));
		}
		DWORD64 _cameras = *(DWORD64*)(g_game + 0x10);

		// € так и не пон€л дл€ чего этот конструктор, т.к. работает и без его вызова, ну пусть будет...
		(**track)(track); // _constructor

		void* owner = nullptr;
		if (Utils::GetGame() == GAME::REDUX) {
			((_camera_manager_play_track_redux)camera_manager_play_track)(_cameras, track, 0.0, 0.0, nullptr, nullptr, nullptr, nullptr, &owner);
		} else {
			((_camera_manager_play_track_arktika_and_exodus)camera_manager_play_track)(_cameras, track, 0.0, 0.0, &owner);
		}
#else
		if (isLL) {
			LPCRITICAL_SECTION mem = memory();
			++*(DWORD*)((DWORD)mem + 0x84);
			EnterCriticalSection(mem);
			DWORD tlsf = *(DWORD*)((DWORD)mem + 0x20);
			// суд€ по всему, в редуксе выравнивание 0x10. “очный размер пам€ти хз, выставил такой-же как в арктике
			void* cflycam_this = nullptr;

			__asm
			{
				mov esi, tlsf
				push 0x10
				push esi
				mov eax, 0x120
				call tlsf_memalign
				mov cflycam_this, eax
				add esp, 8
			}
			LeaveCriticalSection(mem);

			_track track = cflycam_cflycam(cflycam_this, "1");

			if (g_game == NULL)
			{
				// читаем адрес инструкции mov eax, g_game
				// A1 ? ? ? ? 0F 57 C0 56
				DWORD mov = FindPatternInEXE(
					(BYTE*)"\xA1\x00\x00\x00\x00\x0F\x57\xC0\x56",
					"x????xxxx");

				// вычисн€ем адрес и получаем g_game
				g_game = *(DWORD*)(*(DWORD*)(mov + 1));
			}
			DWORD _cameras = *(DWORD*)(g_game + 0x8);

			// € так и не пон€л дл€ чего этот конструктор, т.к. работает и без его вызова, ну пусть будет...
			(**track)(track); // _constructor

			void* owner = nullptr;
			((_camera_manager_play_track)camera_manager_play_track)(_cameras, track, 0, &owner);
		} else {
			uconsole_server** console = (uconsole_server**) Utils::GetConsole();
			(*console)->execute_deferred(console, "fly 1");
		}
#endif
	}
}

#ifdef _WIN64
// восстанавливаем выпиленный в релизе исхода cflycam::r_on_key_press
void __fastcall Fly::exodus_cflycam_r_on_key_press(void* _cflycam, int action, int key, int state, int resending)
{
	switch (key) {
	case 1: // ESC
		*(DWORD*)((DWORD64)_cflycam - 0x4) |= 0x40000000u; // выход из режима полЄта
		break;
	case 28: { // ENTER
		// телепорт
		float _speed = *(float*)((DWORD64)_cflycam + 0x100);
		
		DWORD64 _startup_entity = *(DWORD64*)(Utils::GetGLevel() + 0x20);
		DWORD64 _control_entity = *(DWORD64*)(Utils::GetGLevel() + 0x28);
		DWORD64 _view_entity = *(DWORD64*)(Utils::GetGLevel() + 0x30);
		
		if (Utils::GetTimeGlobalMS() >= LODWORD(_speed)  && _control_entity)
		{
			DWORD64 entities = Utils::GetGEntities();
			entities_core__ser_lock((void*)entities, "cflycam::r_on_key_press");

			_force_transform force_transform = *(_force_transform*) ((*((DWORD64*)_control_entity)) + 0x1E8);
			force_transform((void*)_control_entity, (float*)((DWORD64)_cflycam + 0x20), 0);

			*(DWORD*)((DWORD64)_cflycam - 0x4) |= 0x40000000u; // выход из режима полЄта

			// аналог entities_core::ser_unlock
			volatile long* _ser_block = (volatile long*)(entities + 0xE0);
			while (_InterlockedCompareExchange(_ser_block, 0, -1) != -1) {}
		}
		break;
	}
	case 41: { // TILDE
		if (Utils::GetBool("other", "unlock_dev_console", false)) {
			uconsole_server_exodus** console = (uconsole_server_exodus**)Utils::GetConsole();
			(*console)->show(console);
		}
		break;
	}
	case 197: { // PAUSE
		uconsole_server_exodus** console = (uconsole_server_exodus**)Utils::GetConsole();
		(*console)->execute_deferred(console, "pause");
		break;
	}
	default:
		return;
	}
}
#endif
