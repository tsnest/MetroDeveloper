#include "Unlock3rdPerson.h"
#include "Utils.h"

typedef void(__thiscall* _base_npc_cameras_cam_set)(void* _this, int camera_style, float speed, int preserve_attach);
typedef void(__cdecl* _set_camera_2033)(...); // this function has weird calling convention; 'this' passed in EDI, and two parameters passed through stack
_base_npc_cameras_cam_set base_npc_cameras_cam_set = nullptr;
_set_camera_2033 set_camera_2033 = nullptr;
static bool isLL = false;

enum enpc_cameras_old // 2033, Last Light, Redux (changed a bit in Arktika.1)
{
	enc_first_eye,
	enc_ladder,
	enc_look_at,
	enc_free_look,
	enc_station,
	enc_locked,
	//enc_max_cam // not used in game
};

enum enpc_cameras_new // Arktika.1 and Exodus bitmask
{
	bit_enc_first_eye = 0,
	bit_enc_look_at = 1 << 0,
	bit_enc_free_look = 1 << 1,
	bit_enc_station = (1 << 2) - 1,
	bit_enc_locked = 1 << 2,
	//bit_enc_max_cam = 5 // not used in game
};

Unlock3rdPerson::Unlock3rdPerson()
{
	if (Utils::GetBool("other", "unlock_3rd_person_camera", false)) {
#ifdef _WIN64
		if (Utils::GetGame() == GAME::REDUX) {
			// 48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 56 48 83 EC 30 48 63 41 68 48 8B F1 0F 29 74 24 ? 4C 8B 74 C1 ? 89 51 68 48 63 C2 0F 28 F2 48 8B 5C C1 ? 49 8B 06 49 8B CE 41 8B F9 FF 50 28 48 8B 03 44 8B C7 49 8B D6 48 8B CB FF 50 20 8B 05 ? ? ? ? A8 01 75 20 - Redux STEAM
			base_npc_cameras_cam_set = (_base_npc_cameras_cam_set)FindPatternInEXE(
				(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x41\x56\x48\x83\xEC\x30\x48\x63\x41\x68\x48\x8B\xF1\x0F\x29\x74\x24\x00\x4C\x8B\x74\xC1\x00\x89\x51\x68\x48\x63\xC2\x0F\x28\xF2\x48\x8B\x5C\xC1\x00\x49\x8B\x06\x49\x8B\xCE\x41\x8B\xF9\xFF\x50\x28\x48\x8B\x03\x44\x8B\xC7\x49\x8B\xD6\x48\x8B\xCB\xFF\x50\x20\x8B\x05\x00\x00\x00\x00\xA8\x01\x75\x20",
				"xxxx?xxxx?xxxx?xxxxxxxxxxxxxxxxx?xxxx?xxxxxxxxxxxxx?xxxxxxxxxxxxxxxxxxxxxxxxxxxxx????xxxx");

			if (base_npc_cameras_cam_set == NULL) {
				// 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 30 48 63 41 68 48 8B E9 0F 29 74 24 ? 41 8B F9 0F 28 F2 48 8B 74 C1 ? 89 51 68 48 63 C2 48 8B 5C C1 ? 48 8B CE 48 8B 06 FF 50 28 48 8B 03 44 8B C7 48 8B D6 48 8B CB FF 50 20 E8 ? ? ? ? 48 8B D8 BF ? ? ? ? E8 - Redux EGS
				base_npc_cameras_cam_set = (_base_npc_cameras_cam_set)FindPatternInEXE(
					(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x30\x48\x63\x41\x68\x48\x8B\xE9\x0F\x29\x74\x24\x00\x41\x8B\xF9\x0F\x28\xF2\x48\x8B\x74\xC1\x00\x89\x51\x68\x48\x63\xC2\x48\x8B\x5C\xC1\x00\x48\x8B\xCE\x48\x8B\x06\xFF\x50\x28\x48\x8B\x03\x44\x8B\xC7\x48\x8B\xD6\x48\x8B\xCB\xFF\x50\x20\xE8\x00\x00\x00\x00\x48\x8B\xD8\xBF\x00\x00\x00\x00\xE8",
					"xxxx?xxxx?xxxx?xxxxxxxxxxxxxxxx?xxxxxxxxxx?xxxxxxxxxx?xxxxxxxxxxxxxxxxxxxxxxxxx????xxxx????x");
			}
		} else if (Utils::GetGame() == GAME::ARKTIKA) {
			// 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 30 48 63 41 60 48 8B E9 0F 29 74 24 ? 41 8B F9 0F 28 F2 48 8B 74 C1 ? 89 51 60 48 63 C2 48 8B 5C C1 ? 48 8B CE 48 8B 06 FF 50 28 48 8B 03 44 8B C7 - Arktika
			base_npc_cameras_cam_set = (_base_npc_cameras_cam_set)FindPatternInEXE(
				(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x30\x48\x63\x41\x60\x48\x8B\xE9\x0F\x29\x74\x24\x00\x41\x8B\xF9\x0F\x28\xF2\x48\x8B\x74\xC1\x00\x89\x51\x60\x48\x63\xC2\x48\x8B\x5C\xC1\x00\x48\x8B\xCE\x48\x8B\x06\xFF\x50\x28\x48\x8B\x03\x44\x8B\xC7",
				"xxxx?xxxx?xxxx?xxxxxxxxxxxxxxxx?xxxxxxxxxx?xxxxxxxxxx?xxxxxxxxxxxxxxx");
		} else if (Utils::GetGame() == GAME::EXODUS) {
			// 48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 56 48 83 EC 30 48 63 81 ? ? ? ? 48 8B F1 0F 29 74 24 ? 41 8B F9 0F 28 F2 4C 8B B4 C1 ? ? ? ? 89 91 ? ? ? ? 48 63 C2 48 8B 9C C1 ? ? ? ? 49 8B CE 49 8B 06 FF 50 28 48 8B 03 44 8B C7 49 8B D6 48 8B CB FF 50 20 85 FF 75 24 - Exodus
			base_npc_cameras_cam_set = (_base_npc_cameras_cam_set)FindPatternInEXE(
				(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x41\x56\x48\x83\xEC\x30\x48\x63\x81\x00\x00\x00\x00\x48\x8B\xF1\x0F\x29\x74\x24\x00\x41\x8B\xF9\x0F\x28\xF2\x4C\x8B\xB4\xC1\x00\x00\x00\x00\x89\x91\x00\x00\x00\x00\x48\x63\xC2\x48\x8B\x9C\xC1\x00\x00\x00\x00\x49\x8B\xCE\x49\x8B\x06\xFF\x50\x28\x48\x8B\x03\x44\x8B\xC7\x49\x8B\xD6\x48\x8B\xCB\xFF\x50\x20\x85\xFF\x75\x24",
				"xxxx?xxxx?xxxx?xxxxxxxxx????xxxxxxx?xxxxxxxxxx????xx????xxxxxxx????xxxxxxxxxxxxxxxxxxxxxxxxxxxx");
		}
#else
		isLL = Utils::isLL();

		if (isLL)
		{
			// 53 56 8B F1 8B 46 4C 57 8B 7C 86 34 8B 44 24 10 89 46 4C 8B 17 8B 5C 86 34 8B 42 14 8B CF - Last Light
			base_npc_cameras_cam_set = (_base_npc_cameras_cam_set)FindPatternInEXE(
				(BYTE*)"\x53\x56\x8B\xF1\x8B\x46\x4C\x57\x8B\x7C\x86\x34\x8B\x44\x24\x10\x89\x46\x4C\x8B\x17\x8B\x5C\x86\x34\x8B\x42\x14\x8B\xCF",
				"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
		}
		else
		{
			// B8 ? ? ? ? 84 05 ? ? ? ? 56 75 1D 09 05 ? ? ? ? 68 ? ? ? ? C7 05 ? ? ? ? ? ? ? ? E8 ? ? ? ? 83 C4 04 8B 47 6C - ORIG 2033
			set_camera_2033 = (_set_camera_2033)FindPatternInEXE(
				(BYTE*)"\xB8\x00\x00\x00\x00\x84\x05\x00\x00\x00\x00\x56\x75\x1D\x09\x05\x00\x00\x00\x00\x68\x00\x00\x00\x00\xC7\x05\x00\x00\x00\x00\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x83\xC4\x04\x8B\x47\x6C",
				"x????xx????xxxxx????x????xx????????x????xxxxxx");
		}
#endif
	}
}

void Unlock3rdPerson::clevel_r_on_key_press(void* _this, int action, int key, int state, int resending)
{
	if (key <= 61 && key >= 59) {
#ifdef _WIN64
		void* startup_entity;
		void* control_entity;
		void* view_entity;

		void* base_npc_cameras;

		// _this == g_level + 0x8 (+0x8 due to multiple inheritance)
		// which one is better to use here ??
		if (Utils::GetGame() == GAME::REDUX) {
			startup_entity = *((void**)((char*)_this + 0x28));
			control_entity = *((void**)((char*)_this + 0x30));
			view_entity = *((void**)((char*)_this + 0x38));

			base_npc_cameras = *((void**)((char*)control_entity + 0x640));

			if (key == 59) // F1
				base_npc_cameras_cam_set(base_npc_cameras, enc_first_eye, 1.f, 1);
			if (key == 60) // F2
				base_npc_cameras_cam_set(base_npc_cameras, enc_look_at, 1.f, 1);
			if (key == 61) // F3
				base_npc_cameras_cam_set(base_npc_cameras, enc_free_look, 1.f, 1);
		} else if (Utils::GetGame() == GAME::ARKTIKA || Utils::GetGame() == GAME::EXODUS) {
			startup_entity = *((void**)((char*)_this + 0x18));
			control_entity = *((void**)((char*)_this + 0x20));
			view_entity = *((void**)((char*)_this + 0x28));

			base_npc_cameras = (Utils::GetGame() == GAME::ARKTIKA ? *((void**)((char*)control_entity + 0x7C8)) : *((void**)((char*)control_entity + 0x928)));

			if (key == 59) // F1
				base_npc_cameras_cam_set(base_npc_cameras, bit_enc_first_eye, 1.f, 1);
			if (key == 60) // F2
				base_npc_cameras_cam_set(base_npc_cameras, bit_enc_look_at, 1.f, 1);
			if (key == 61) // F3
				base_npc_cameras_cam_set(base_npc_cameras, bit_enc_free_look, 1.f, 1);
		}
#else
		if (isLL) {
			// _this == g_level + 0x4 (+0x4 due to multiple inheritance)

			// which one is better to use here ??
			void* startup_entity = *((void**)((char*)_this + 0x18));
			void* control_entity = *((void**)((char*)_this + 0x1C));
			void* view_entity = *((void**)((char*)_this + 0x20));

			void* base_npc_cameras = *((void**)((char*)control_entity + 0x3A4));

			if (key == 59) // F1
				base_npc_cameras_cam_set(base_npc_cameras, enc_first_eye, 1.f, 1);
			if (key == 60) // F2
				base_npc_cameras_cam_set(base_npc_cameras, enc_look_at, 1.f, 1);
			if (key == 61) // F3
				base_npc_cameras_cam_set(base_npc_cameras, enc_free_look, 1.f, 1);
		} else {
			// _this == g_level + 0x4 (+0x4 due to multiple inheritance)

			// which one is better to use here ??
			void* control_entity = *((void**)((char*)_this + 0x10));
			void* view_entity = *((void**)((char*)_this + 0x14));

			void* base_npc_cameras = *((void**)((char*)control_entity + 0x348));

			if (key == 59) // F1
			{
				__asm
				{
					push 3F800000h              // speed = 1.f
					push enc_first_eye          // camera_style = enc_first_eye
					mov edi, [base_npc_cameras] // 'this' pointer passed in EDI
					call[set_camera_2033]
				}
			}

			if (key == 60) // F2
			{
				__asm
				{
					push 3F800000h              // speed = 1.f
					push enc_look_at            // camera_style = enc_look_at
					mov edi, [base_npc_cameras] // 'this' pointer passed in EDI
					call[set_camera_2033]
				}
			}

			if (key == 61) // F3
			{
				__asm
				{
					push 3F800000h              // speed = 1.f
					push enc_free_look          // camera_style = enc_free_look
					mov edi, [base_npc_cameras] // 'this' pointer passed in EDI
					call[set_camera_2033]
				}
			}
		}
#endif
	}
}
