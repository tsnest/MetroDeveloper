#include "Fly.h"
#include "Utils.h"
#include "uconsole.h"

typedef void(__fastcall*** _icamera_track_constructor)(void*);

#ifdef _WIN64
typedef void (__fastcall* _force_transform)(void* _this, float* m, int on_load);

typedef _icamera_track_constructor(__fastcall* _cflycam_cflycam)(void* _this, const char* name, float slide, float scale);
typedef _icamera_track_constructor(__fastcall* _cflycam_player)(void* _this, const char* name, float speed, unsigned int cycles);
typedef LPCRITICAL_SECTION(__fastcall* _memory)();
typedef void* (__fastcall* _tlsf_memalign)(DWORD64 tlsf, DWORD align, DWORD size);
typedef void* (__fastcall* _camera_manager_play_track_redux)(DWORD64 _this, void* t, float accrue, float start_pos, void* unused3, void* e, void* unused4, void* unused5, void* owner);
typedef void* (__fastcall* _camera_manager_play_track_arktika_and_exodus)(DWORD64 _this, void* t, float accrue, float start_pos, void* owner);
_tlsf_memalign tlsf_memalign = nullptr;
bool isTimeIncreased = false;

#else

typedef _icamera_track_constructor(__thiscall* _cflycam_cflycam)(void* _this, const char* name);
typedef _icamera_track_constructor(__thiscall* _cflycam_player)(void* _this, const char* name, float speed, unsigned int cycles);
typedef LPCRITICAL_SECTION(__cdecl* _memory)();
typedef void* (__thiscall* _camera_manager_play_track)(DWORD _this, void* t, double hz, void* owner);
void* tlsf_memalign = nullptr;
#endif

_cflycam_cflycam cflycam_cflycam = nullptr;
_cflycam_player cflycam_player = nullptr;
_memory memory = nullptr;
void* camera_manager_play_track = nullptr;

float Fly::refly_speed;
unsigned int Fly::refly_cycles;

Fly::Fly()
{
	if (Utils::GetBool("other", "fly", false) || Utils::GetBool("other", "restore_deleted_commands", false))
	{
#ifdef _WIN64
		// Так получилось, что сигнатуры в некоторых случаях подходят к разным версиям движка
		if (Utils::isRedux() || Utils::isArktika()) {
			// 48 89 5C 24 ? 55 56 41 54 41 56 41 57 48 81 EC - Redux STEAM and Arktika
			cflycam_player = (_cflycam_player)FindPatternInEXE(
				"\x48\x89\x5C\x24\x00\x55\x56\x41\x54\x41\x56\x41\x57\x48\x81\xEC",
				"xxxx?xxxxxxxxxxx");
		}

		if (Utils::isRedux() || Utils::isExodus()) {
			if (cflycam_player == NULL) {
				// 40 53 55 57 41 54 41 55 48 81 EC - Redux EGS and Exodus ALL
				cflycam_player = (_cflycam_player)FindPatternInEXE(
					"\x40\x53\x55\x57\x41\x54\x41\x55\x48\x81\xEC",
					"xxxxxxxxxxx");
			}
		}

		if (Utils::isRedux()) {
			// 48 8B C4 48 89 58 10 48 89 68 18 56 57 41 54 41 56 41 57 48 81 EC ? ? ? ? 0F 29 70 C8 0F 29 78 B8 44 0F 29 40 - Redux STEAM
			cflycam_cflycam = (_cflycam_cflycam)FindPatternInEXE(
				"\x48\x8B\xC4\x48\x89\x58\x10\x48\x89\x68\x18\x56\x57\x41\x54\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x0F\x29\x70\xC8\x0F\x29\x78\xB8\x44\x0F\x29\x40",
				"xxxxxxxxxxxxxxxxxxxxxx????xxxxxxxxxxxx");

			if (cflycam_cflycam == NULL) {
				// 48 8B C4 48 89 58 10 48 89 68 18 48 89 70 20 57 41 56 41 57 48 81 EC ? ? ? ? 0F 29 70 D8 48 8D B1 - Redux EGS
				cflycam_cflycam = (_cflycam_cflycam)FindPatternInEXE(
					"\x48\x8B\xC4\x48\x89\x58\x10\x48\x89\x68\x18\x48\x89\x70\x20\x57\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x0F\x29\x70\xD8\x48\x8D\xB1",
					"xxxxxxxxxxxxxxxxxxxxxxx????xxxxxxx");
			}

			// 48 83 EC 28 48 8B 05 ? ? ? ? 48 85 C0 75 7F 8B 05 ? ? ? ? 48 89 5C 24 ? A8 01 75 1A 83 C8 01 89 05 ? ? ? ? E8 ? ? ? ? 48 8D 0D ? ? ? ? E8 - Redux STEAM
			memory = (_memory)FindPatternInEXE(
				"\x48\x83\xEC\x28\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x75\x7F\x8B\x05\x00\x00\x00\x00\x48\x89\x5C\x24\x00\xA8\x01\x75\x1A\x83\xC8\x01\x89\x05\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\xE8",
				"xxxxxxx????xxxxxxx????xxxx?xxxxxxxxx????x????xxx????x");

			if (memory == NULL) {
				// 48 83 EC 28 48 8B 05 ? ? ? ? 48 85 C0 0F 85 ? ? ? ? 65 48 8B 04 25 ? ? ? ? 48 89 5C 24 ? 48 89 6C 24 ? 48 8D 2D ? ? ? ? 48 89 74 24 ? 33 F6 48 8B 08 BA ? ? ? ? 48 89 7C 24 ? 8B 04 0A 39 05 ? ? ? ? 0F 8F - Redux EGS
				memory = (_memory)FindPatternInEXE(
					"\x48\x83\xEC\x28\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x0F\x85\x00\x00\x00\x00\x65\x48\x8B\x04\x25\x00\x00\x00\x00\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x8D\x2D\x00\x00\x00\x00\x48\x89\x74\x24\x00\x33\xF6\x48\x8B\x08\xBA\x00\x00\x00\x00\x48\x89\x7C\x24\x00\x8B\x04\x0A\x39\x05\x00\x00\x00\x00\x0F\x8F",
					"xxxxxxx????xxxxx????xxxxx????xxxx?xxxx?xxx????xxxx?xxxxxx????xxxx?xxxxx????xx");
			}

			// 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 20 49 8D 40 FF 41 B9 ? ? ? ? 33 F6 48 8B FA 48 8B E9 49 3B C1 77 14 - Redux STEAM
			tlsf_memalign = (_tlsf_memalign)FindPatternInEXE(
				"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\x49\x8D\x40\xFF\x41\xB9\x00\x00\x00\x00\x33\xF6\x48\x8B\xFA\x48\x8B\xE9\x49\x3B\xC1\x77\x14",
				"xxxx?xxxx?xxxx?xxxxxxxxxxx????xxxxxxxxxxxxx");

			if (tlsf_memalign == NULL) {
				// 48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8B DA 49 8D 40 FF 33 D2 41 B9 ? ? ? ? 48 8B F1 8B FA 49 3B C1 77 14 BF ? ? ? ? 49 8D 40 07 48 83 E0 F8 48 3B C7 48 0F 47 F8 - Redux EGS
				tlsf_memalign = (_tlsf_memalign)FindPatternInEXE(
					"\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\x48\x8B\xDA\x49\x8D\x40\xFF\x33\xD2\x41\xB9\x00\x00\x00\x00\x48\x8B\xF1\x8B\xFA\x49\x3B\xC1\x77\x14\xBF\x00\x00\x00\x00\x49\x8D\x40\x07\x48\x83\xE0\xF8\x48\x3B\xC7\x48\x0F\x47\xF8",
					"xxxx?xxxx?xxxxxxxxxxxxxxxx????xxxxxxxxxxx????xxxxxxxxxxxxxxx");
			}

			// 48 89 5C 24 ? 48 89 54 24 ? 57 48 83 EC 60 48 8B 02 48 8B DA 48 8B 94 24 ? ? ? ? 0F 29 74 24 ? 0F 29 7C 24 ? 0F 28 F3 48 8B F9 48 8B CB 0F 28 FA FF 90 - Redux STEAM
			camera_manager_play_track = (void*)FindPatternInEXE(
				"\x48\x89\x5C\x24\x00\x48\x89\x54\x24\x00\x57\x48\x83\xEC\x60\x48\x8B\x02\x48\x8B\xDA\x48\x8B\x94\x24\x00\x00\x00\x00\x0F\x29\x74\x24\x00\x0F\x29\x7C\x24\x00\x0F\x28\xF3\x48\x8B\xF9\x48\x8B\xCB\x0F\x28\xFA\xFF\x90",
				"xxxx?xxxx?xxxxxxxxxxxxxxx????xxxx?xxxx?xxxxxxxxxxxxxx");

			if (camera_manager_play_track == NULL) {
				// 48 89 5C 24 ? 48 89 74 24 ? 48 89 54 24 ? 57 48 83 EC 50 48 8B 02 48 8B DA 48 8B 94 24 ? ? ? ? 48 8B F9 0F 29 74 24 ? 48 8B CB 0F 29 7C 24 ? 0F 28 F2 0F 28 FB FF 90 - Redux EGS
				camera_manager_play_track = (void*)FindPatternInEXE(
					"\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x54\x24\x00\x57\x48\x83\xEC\x50\x48\x8B\x02\x48\x8B\xDA\x48\x8B\x94\x24\x00\x00\x00\x00\x48\x8B\xF9\x0F\x29\x74\x24\x00\x48\x8B\xCB\x0F\x29\x7C\x24\x00\x0F\x28\xF2\x0F\x28\xFB\xFF\x90",
					"xxxx?xxxx?xxxx?xxxxxxxxxxxxxxx????xxxxxxx?xxxxxxx?xxxxxxxx");
			}
		} else if (Utils::isArktika()) {
			// 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? F3 0F 10 1D - Arktika
			cflycam_cflycam = (_cflycam_cflycam)FindPatternInEXE(
				"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00\xF3\x0F\x10\x1D",
				"xxxx?xxxx?xxxx?xxxxxxxxxxxx????xxxx");

			// 48 83 EC 28 48 8B 05 ? ? ? ? 48 85 C0 75 52 48 89 5C 24 ? E8 ? ? ? ? E8 ? ? ? ? 33 DB 48 85 C0 74 0A 48 8B C8 E8 ? ? ? ? EB 03 48 8B C3 48 89 05 - Arktika
			memory = (_memory)FindPatternInEXE(
				"\x48\x83\xEC\x28\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x75\x52\x48\x89\x5C\x24\x00\xE8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x33\xDB\x48\x85\xC0\x74\x0A\x48\x8B\xC8\xE8\x00\x00\x00\x00\xEB\x03\x48\x8B\xC3\x48\x89\x05",
				"xxxxxxx????xxxxxxxxx?x");

			// 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 20 33 F6 49 8D 40 FF 41 B9 ? ? ? ? 48 8B FA 48 8B E9 49 3B C1 77 14 BE ? ? ? ? 49 8D 40 07 48 83 E0 F8 48 3B C6 48 0F 47 F0 48 8D 0C 16 33 D2 48 8D 41 1F 49 3B C1 - Arktika
			tlsf_memalign = (_tlsf_memalign)FindPatternInEXE(
				"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\x33\xF6\x49\x8D\x40\xFF\x41\xB9\x00\x00\x00\x00\x48\x8B\xFA\x48\x8B\xE9\x49\x3B\xC1\x77\x14\xBE\x00\x00\x00\x00\x49\x8D\x40\x07\x48\x83\xE0\xF8\x48\x3B\xC6\x48\x0F\x47\xF0\x48\x8D\x0C\x16\x33\xD2\x48\x8D\x41\x1F\x49\x3B\xC1",
				"xxxx?xxxx?xxxx?xxxxxxxxxxxxx????xxxxxxxxxxxx????xxxxxxxxxxxxxxxxxxxxxxxxxxxx");

			// 48 89 5C 24 ? 48 89 54 24 ? 57 48 83 EC 50 48 8B 02 48 8B DA 48 8B 94 24 ? ? ? ? 48 8B F9 0F 29 74 24 ? 48 8B CB 0F 29 7C 24 ? 0F 28 F3 0F 28 FA FF 90 ? ? ? ? 48 8B 03 0F 28 D6 0F 28 CF 48 8B CB FF 90 ? ? ? ? 48 8D 05 - Arktika
			camera_manager_play_track = (void*)FindPatternInEXE(
				"\x48\x89\x5C\x24\x00\x48\x89\x54\x24\x00\x57\x48\x83\xEC\x50\x48\x8B\x02\x48\x8B\xDA\x48\x8B\x94\x24\x00\x00\x00\x00\x48\x8B\xF9\x0F\x29\x74\x24\x00\x48\x8B\xCB\x0F\x29\x7C\x24\x00\x0F\x28\xF3\x0F\x28\xFA\xFF\x90\x00\x00\x00\x00\x48\x8B\x03\x0F\x28\xD6\x0F\x28\xCF\x48\x8B\xCB\xFF\x90\x00\x00\x00\x00\x48\x8D\x05",
				"xxxx?xxxx?xxxxxxxxxxxxxxx????xxxxxxx?xxxxxxx?xxxxxxxx????xxxxxxxxxxxxxx????xxx");
		} else if (Utils::isExodus()) {
			// 40 53 55 56 57 41 54 41 56 41 57 48 81 EC ? ? ? ? F3 0F 10 1D - Exodus ALL
			cflycam_cflycam = (_cflycam_cflycam)FindPatternInEXE(
				"\x40\x53\x55\x56\x57\x41\x54\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00\xF3\x0F\x10\x1D",
				"xxxxxxxxxxxxxx????xxxx");

			// 48 83 EC 28 48 8B 05 ? ? ? ? 48 85 C0 0F 85 ? ? ? ? 65 48 8B 04 25 - Exodus ALL
			memory = (_memory)FindPatternInEXE(
				"\x48\x83\xEC\x28\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x0F\x85\x00\x00\x00\x00\x65\x48\x8B\x04\x25",
				"xxxxxxx????xxxxx????xxxxx");

			if (Utils::isExodusPatched) {
				// 48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8B DA 49 8D 40 FF 33 D2 41 B9 ? ? ? ? 48 8B F1 8B FA 49 3B C1 77 14 BF - Exodus NEW
				tlsf_memalign = (_tlsf_memalign)FindPatternInEXE(
					"\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\x48\x8B\xDA\x49\x8D\x40\xFF\x33\xD2\x41\xB9\x00\x00\x00\x00\x48\x8B\xF1\x8B\xFA\x49\x3B\xC1\x77\x14\xBF",
					"xxxx?xxxx?xxxxxxxxxxxxxxxx????xxxxxxxxxxx");

				// 48 89 6C 24 ? 48 89 54 24 ? 57 48 83 EC 50 48 8B 02 48 8B F9 48 89 5C 24 ? 48 8B DA 48 8B 94 24 ? ? ? ? 48 8B CB 0F 29 74 24 ? 0F 28 F2 0F 29 7C 24 - Exodus
				camera_manager_play_track = (void*)FindPatternInEXE(
					"\x48\x89\x6C\x24\x00\x48\x89\x54\x24\x00\x57\x48\x83\xEC\x50\x48\x8B\x02\x48\x8B\xF9\x48\x89\x5C\x24\x00\x48\x8B\xDA\x48\x8B\x94\x24\x00\x00\x00\x00\x48\x8B\xCB\x0F\x29\x74\x24\x00\x0F\x28\xF2\x0F\x29\x7C\x24",
					"xxxx?xxxx?xxxxxxxxxxxxxxx?xxxxxxx????xxxxxxx?xxxxxxx");
			} else {
				// 48 89 5c 24 ? 48 89 74 24 ? 57 48 83 ec ? 48 89 d3 49 8d 40 - Exodus OLD
				tlsf_memalign = (_tlsf_memalign)FindPatternInEXE(
					"\x48\x89\x5c\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xec\x00\x48\x89\xd3\x49\x8d\x40",
					"xxxx?xxxx?xxxx?xxxxxx");

				// 48 89 5c 24 ? 48 89 74 24 ? 48 89 54 24 ? 57 48 83 ec ? 48 8b 02 - Exodus OLD
				camera_manager_play_track = (void*)FindPatternInEXE(
					"\x48\x89\x5c\x24\x00\x48\x89\x74\x24\x00\x48\x89\x54\x24\x00\x57\x48\x83\xec\x00\x48\x8b\x02",
					"xxxx?xxxx?xxxx?xxxx?xxx");
			}
		}

#else
		if (Utils::isLL())
		{
			// 55 8B EC 83 E4 F0 81 EC ? ? ? ? 53 56 8B F1 57 33 FF 89 7E 04 89 7E 08 81 66 ? ? ? ? ? C7 06 ? ? ? ? C7 46
			cflycam_cflycam = (_cflycam_cflycam)FindPatternInEXE(
				"\x55\x8B\xEC\x83\xE4\xF0\x81\xEC\x00\x00\x00\x00\x53\x56\x8B\xF1\x57\x33\xFF\x89\x7E\x04\x89\x7E\x08\x81\x66\x00\x00\x00\x00\x00\xC7\x06\x00\x00\x00\x00\xC7\x46",
				"xxxxxxxx????xxxxxxxxxxxxxxx?????xx????xx");

			// 81 ec ? ? ? ? 53 56 8b f1 33 c0 89 46
			cflycam_player = (_cflycam_player)FindPatternInEXE(
				"\x81\xec\x00\x00\x00\x00\x53\x56\x8b\xf1\x33\xc0\x89\x46",
				"xx????xxxxxxxx");

			// 55 8B EC 83 E4 F8 A1 ? ? ? ? 85 C0 75 48 B8 ? ? ? ? 84 05 ? ? ? ? 75 18 09 05 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 04
			memory = (_memory)FindPatternInEXE(
				"\x55\x8B\xEC\x83\xE4\xF8\xA1\x00\x00\x00\x00\x85\xC0\x75\x48\xB8\x00\x00\x00\x00\x84\x05\x00\x00\x00\x00\x75\x18\x09\x05\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x68\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x83\xC4\x04",
				"xxxxxxx????xxxxx????xx????xxxx????x????x????x????xxx");

			// 53 8B 5C 24 0C 55 56 33 ED 57 85 C0 74 19 3D ? ? ? ? 73 12 83 C0 03 83 E0 FC 8B E8 83 F8 0C 77 05 BD
			tlsf_memalign = (void*)FindPatternInEXE(
				"\x53\x8B\x5C\x24\x0C\x55\x56\x33\xED\x57\x85\xC0\x74\x19\x3D\x00\x00\x00\x00\x73\x12\x83\xC0\x03\x83\xE0\xFC\x8B\xE8\x83\xF8\x0C\x77\x05\xBD",
				"xxxxxxxxxxxxxxx");

			// 83 EC 10 53 56 8B 74 24 1C 8B 06 8B 50 44 57 8B F9 8B 4C 24 2C 51 8B CE FF D2 F3 0F 10 44 24 ? 8B 06 8B 50 6C 83 EC 08 F3 0F 11 44 24
			camera_manager_play_track = (_camera_manager_play_track)FindPatternInEXE(
				"\x83\xEC\x10\x53\x56\x8B\x74\x24\x1C\x8B\x06\x8B\x50\x44\x57\x8B\xF9\x8B\x4C\x24\x2C\x51\x8B\xCE\xFF\xD2\xF3\x0F\x10\x44\x24\x00\x8B\x06\x8B\x50\x6C\x83\xEC\x08\xF3\x0F\x11\x44\x24",
				"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx?xxxxxxxxxxxxx");
		}
#endif
	}
}

void Fly::fly(const char* name, bool refly, float refly_speed, unsigned int refly_cycles)
{
#ifdef _WIN64
	LPCRITICAL_SECTION mem = memory();
	++*(DWORD*)((DWORD64)mem + (Utils::isRedux() ? 0x100 : 0x140));
	EnterCriticalSection(mem);
	DWORD64 tlsf = *(DWORD64*)((DWORD64)mem + (Utils::isRedux() ? 0x38 : 0x30));
	void* _this = nullptr;
	if(!refly)
		_this = tlsf_memalign(tlsf, 0x10, Utils::isExodus() ? 0x1F0 : 0x120);
	else
		_this = tlsf_memalign(tlsf, 0x10, 0x100); // TODO: нужно найти количество памяти для исхода
	LeaveCriticalSection(mem);

	// в cflycam_cflycam последние два параметра slide и scale нужны только в исходе
	_icamera_track_constructor track = nullptr;
	if (!refly)
		track = cflycam_cflycam(_this, name, 0.2f, 0.125f);
	else
		track = cflycam_player(_this, name, refly_speed, refly_cycles);

	DWORD64 _cameras = *(DWORD64*)(Utils::GetGGame() + 0x10);

	// я так и не понял для чего этот конструктор, т.к. работает и без его вызова, ну пусть будет...
	(**track)(track); // _constructor

	void* owner = nullptr;
	if (Utils::GetGame() == GAME::REDUX) {
		((_camera_manager_play_track_redux)camera_manager_play_track)(_cameras, track, 0.0, 0.0, nullptr, nullptr, nullptr, nullptr, &owner);
	} else {
		((_camera_manager_play_track_arktika_and_exodus)camera_manager_play_track)(_cameras, track, 0.0, 0.0, &owner);
	}
#else
	LPCRITICAL_SECTION mem = memory();
	++*(DWORD*)((DWORD)mem + 0x84);
	EnterCriticalSection(mem);
	DWORD tlsf = *(DWORD*)((DWORD)mem + 0x20);
	// судя по всему, в редуксе выравнивание 0x10. Точный размер памяти хз, выставил такой-же как в арктике
	void* _this = nullptr;

	__asm
	{
		mov esi, tlsf
		push 0x10
		push esi
		mov eax, 0x120
		call tlsf_memalign
		mov _this, eax
		add esp, 8
	}
	LeaveCriticalSection(mem);

	_icamera_track_constructor track = nullptr;
	if (!refly)
		track = cflycam_cflycam(_this, name);
	else
		track = cflycam_player(_this, name, refly_speed, refly_cycles);

	DWORD _cameras = *(DWORD*)(Utils::GetGGame() + 0x8);

	// я так и не понял для чего этот конструктор, т.к. работает и без его вызова, ну пусть будет...
	(**track)(track); // _constructor

	void* owner = nullptr;
	((_camera_manager_play_track)camera_manager_play_track)(_cameras, track, 0, &owner);
#endif
}

void Fly::clevel_r_on_key_press(int action, int key, int state, int resending)
{
	if (key == 62)
	{
#ifndef _WIN64
		if (Utils::isLL()) {
			fly("", false, 0, 0);
		} else {
			uconsole_server** console = (uconsole_server**)Utils::GetConsole();
			(*console)->execute_deferred(console, "fly 1");
		}
#else
		fly("", false, 0, 0);
#endif
	}
}

#ifdef _WIN64
// восстанавливаем выпиленный в патчах исхода cflycam::r_on_key_press
void __fastcall Fly::exodus_cflycam_r_on_key_press(void* _this, int action, int key, int state, int resending)
{
	switch (key) {
	case 1: // ESC
		*(DWORD*)((DWORD64)_this - 0x4) |= 0x40000000u; // выход из режима полёта
		break;
	case 28: { // ENTER
		// телепорт
		float _speed = *(float*)((DWORD64)_this + 0x100);
		
		DWORD64 _startup_entity = *(DWORD64*)(Utils::GetGLevel() + 0x20);
		DWORD64 _control_entity = *(DWORD64*)(Utils::GetGLevel() + 0x28);
		DWORD64 _view_entity = *(DWORD64*)(Utils::GetGLevel() + 0x30);
		
		if (Utils::GetTimeGlobalMS() >= LODWORD(_speed)  && _control_entity)
		{
			_force_transform force_transform = *(_force_transform*) ((*((DWORD64*)_control_entity)) + 0x1E8);
			force_transform((void*)_control_entity, (float*)((DWORD64)_this + 0x20), 0);

			*(DWORD*)((DWORD64)_this - 0x4) |= 0x40000000u; // выход из режима полёта
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
	case 78: { // NUMPAD PLUS
		isTimeIncreased = true;
		Utils::slowmo_debug_increase();
		break;
	}
	case 74: { // NUMPAD MINUS
		if (isTimeIncreased) {
			isTimeIncreased = false;
			Utils::slowmo_debug(1.0f);
		} else {
			Utils::slowmo_debug_decrease();
		}
		break;
	}
	case 57: { // SPACE
		// данный код предназначен для записи позиций камеры в .fly файл
		typedef DWORD64(__fastcall* VFunc)(DWORD64);

		DWORD64 obj = (DWORD64)_this - 32;

		DWORD64* vtable = *(DWORD64**)obj;
		VFunc func = (VFunc)vtable[32];  // 256 / 8 = 32

		func(obj);
		break;
	}
	default:
		return;
	}
}
#endif
