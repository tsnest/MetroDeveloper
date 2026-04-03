#include "RestoreCommands.h"
#include "Utils.h"
#include "uconsole.h"
#include "Fly.h"

void* cmd_mask_Address = nullptr;
unsigned int* ps_actor_flags = nullptr;
unsigned int* igame_hud__flags = nullptr;

uconsole_command_vtbl signal_vftable;
uconsole_command_vtbl fly_vftable;
uconsole_command_vtbl refly_vftable;

cmd_mask_struct_ll g_god_old;
cmd_mask_struct_ll g_global_god_old;
cmd_mask_struct_ll g_notarget_old;
cmd_mask_struct_ll g_unlimitedammo_old;
cmd_mask_struct_ll g_unlimitedfilters_old;
cmd_mask_struct_ll r_hud_weapon_old;
cmd_float_struct_ll r_base_fov; // LL no patches
cmd_executor_struct_ll fly_old;
cmd_executor_struct_ll refly_old;
cmd_executor_struct_ll signal_old;

#ifdef _WIN64
uconsole_command_exodus_vtbl signal_vftable_exodus;
uconsole_command_exodus_vtbl fly_vftable_exodus;
uconsole_command_exodus_vtbl refly_vftable_exodus;

cmd_mask_struct_a1 g_god_new;
cmd_mask_struct_a1 g_global_god_new;
cmd_mask_struct_a1 g_notarget_new;
cmd_mask_struct_a1 g_unlimitedammo_new;
cmd_mask_struct_a1 g_unlimitedfilters_new;
cmd_mask_struct_a1 g_godbless_new;
cmd_executor_struct_a1 fly_new;
cmd_executor_struct_a1 refly_new;
cmd_executor_struct_a1 signal_new;

DWORD64 igame_level_signal;
_unknown_exodus unknown_exodus;
int* refly_default_cycles_exodus = nullptr;
float* refly_default_speed_exodus = nullptr;

#else

DWORD igame_level_signal;

cmd_executor_struct_2033 signal_2033;
cmd_executor_struct_ll signal_ll;
#endif

RestoreCommands::RestoreCommands()
{
	if (Utils::GetBool("other", "restore_deleted_commands", false)) {
#ifndef _WIN64
		if (Utils::isLL()) {
			// c7 05 ? ? ? ? ? ? ? ? 89 1d ? ? ? ? 89 1d ? ? ? ? 89 1d ? ? ? ? e8 ? ? ? ? 83 c4 ? e8 - Last Light
			ps_actor_flags = *(unsigned int**)(FindPatternInEXE(
				"\xc7\x05\x00\x00\x00\x00\x00\x00\x00\x00\x89\x1d\x00\x00\x00\x00\x89\x1d\x00\x00\x00\x00\x89\x1d\x00\x00\x00\x00\xe8\x00\x00\x00\x00\x83\xc4\x00\xe8",
				"xx????????xx????xx????xx????x????xx?x") + 6);

			// 8B 0D ? ? ? ? C1 E9 03 - Last Light
			igame_hud__flags = *(unsigned int**)(FindPatternInEXE("\x8B\x0D\x00\x00\x00\x00\xC1\xE9\x03", "xx????xxx") + 2);

			// 53 55 8B 6C 24 0C 56 8B 35 - LL
			igame_level_signal = FindPatternInEXE(
				"\x53\x55\x8B\x6C\x24\x0C\x56\x8B\x35",
				"xxxxxxxxx"
			);
		} else {
			// 83 EC 14 53 55 56 8B 35 DC 0D A1 00 8B 86 F0 01 00 00 57 8D BE F0 01 00 00 0F B6 C8 - orig 2033
			igame_level_signal = FindPatternInEXE(
				"\x83\xEC\x14\x53\x55\x56\x8B\x35\xDC\x0D\xA1\x00\x8B\x86\xF0\x01\x00\x00\x57\x8D\xBE\xF0\x01\x00\x00\x0F\xB6\xC8",
				"xx?xxxxx????xx????xxx????xxx"
			);
		}
#else
		if (Utils::isRedux()) {
			// 8B 05 ? ? ? ? C1 E8 03 - Redux
			DWORD64 mov = FindPatternInEXE("\x8B\x05\x00\x00\x00\x00\xC1\xE8\x03", "xx????xxx");

			igame_hud__flags = (unsigned int*)Utils::GetAddrFromRelativeInstr(mov, 6, 2);

			// 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC 30 48 8B 2D ? ? ? ? 45 8B F1 4D 8B F8 48 8B FA FF 15 - Redux STEAM
			igame_level_signal = FindPatternInEXE(
				"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x41\x56\x41\x57\x48\x83\xEC\x30\x48\x8B\x2D\x00\x00\x00\x00\x45\x8B\xF1\x4D\x8B\xF8\x48\x8B\xFA\xFF\x15",
				"xxxx?xxxx?xxxx?xxxxxxxxxxxx????xxxxxxxxxxx");

			if (igame_level_signal == NULL) {
				// 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 56 48 83 EC 30 48 8B 3D ? ? ? ? 41 8B E9 4D 8B F0 48 8B F2 FF 15 - Redux EGS
				igame_level_signal = FindPatternInEXE(
					"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x41\x56\x48\x83\xEC\x30\x48\x8B\x3D\x00\x00\x00\x00\x41\x8B\xE9\x4D\x8B\xF0\x48\x8B\xF2\xFF\x15",
					"xxxx?xxxx?xxxx?xxxx?xxxxxxxxx????xxxxxxxxxxx");
			}
		}
		else if (Utils::isArktika()) {
			// 48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 56 48 83 EC 30 C7 44 24 ? ? ? ? ? 41 8B F1 4C 8B 35 ? ? ? ? 49 8B E8 48 8B FA FF 15 - Arktika
			igame_level_signal = FindPatternInEXE(
				"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x56\x57\x41\x56\x48\x83\xEC\x30\xC7\x44\x24\x00\x00\x00\x00\x00\x41\x8B\xF1\x4C\x8B\x35\x00\x00\x00\x00\x49\x8B\xE8\x48\x8B\xFA\xFF\x15",
				"xxxx?xxxx?xxxxxxxxxxx?????xxxxxx????xxxxxxxx");
		}
		else if (Utils::isExodus()) {
			DWORD64 call_igame_level_signal;
			DWORD64 call_unknown_exodus;

			if (Utils::isExodusPatched) {
				// E8 ? ? ? ? 40 88 35
				call_igame_level_signal = FindPatternInEXE("\xE8\x00\x00\x00\x00\x40\x88\x35", "x????xxx");

				// E8 ? ? ? ? 4C 8B 38
				call_unknown_exodus = FindPatternInEXE("\xE8\x00\x00\x00\x00\x4C\x8B\x38", "x????xxx");
			} else {
				// E8 ? ? ? ? E9 ? ? ? ? 48 8D 51 14
				call_igame_level_signal = FindPatternInEXE("\xE8\x00\x00\x00\x00\xE9\x00\x00\x00\x00\x48\x8D\x51\x14", "x????x????xxxx");

				// E8 ? ? ? ? 4C 89 65 58
				call_unknown_exodus = FindPatternInEXE("\xE8\x00\x00\x00\x00\x4C\x89\x65\x58", "x????xxxx");
			}

			igame_level_signal = Utils::GetAddrFromRelativeInstr(call_igame_level_signal, 5, 1);
			unknown_exodus = (_unknown_exodus) Utils::GetAddrFromRelativeInstr(call_unknown_exodus, 5, 1);
		}
#endif
	}
}

#ifdef _WIN64
void __fastcall RestoreCommands::signal_execute(void* _this, const char* name)
#else
void __thiscall RestoreCommands::signal_execute(void* _this, const char* name)
#endif
{
	void* s = Utils::str_shared(name);

#ifdef _WIN64
	if (Utils::isRedux()) {
		((_igame_level_signal)igame_level_signal)(NULL, &s, NULL, 0);
	} else if (Utils::isArktika()) {
		((_igame_level_signal_a1)igame_level_signal)(NULL, &s, NULL, 0);
	} else {
		((_igame_level_signal_ex)igame_level_signal)(NULL, &s, NULL, 0, unknown_exodus());
	}
	
#else

	if (Utils::is2033()) {
		((_igame_level_signal_2033)igame_level_signal)(&s, 0);
	} else {
		((_igame_level_signal_LL)igame_level_signal)(&s, NULL, 0);
	}
#endif
}

#ifdef _WIN64
void __fastcall RestoreCommands::fly_execute(void* _this, const char* name)
#else
void __thiscall RestoreCommands::fly_execute(void* _this, const char* name)
#endif
{
#ifdef _WIN64
	if (Utils::isExodus()) {
		uconsole_server_exodus** console = (uconsole_server_exodus**)Utils::GetConsole();
		(*console)->hide(console);
	} else {
		uconsole_server** console = (uconsole_server**)Utils::GetConsole();
		(*console)->hide(console);
	}
#else
	uconsole_server** console = (uconsole_server**)Utils::GetConsole();
	(*console)->hide(console);
#endif

	Fly::fly(name, false, 0, 0);
}

#ifdef _WIN64
void __fastcall RestoreCommands::refly_execute(void* _this, const char* args)
#else
void __thiscall RestoreCommands::refly_execute(void* _this, const char* args)
#endif
{
	if (args[1] != 0) {
#ifdef _WIN64
		if (Utils::isExodus()) {
			uconsole_server_exodus** console = (uconsole_server_exodus**)Utils::GetConsole();
			(*console)->hide(console);
		} else {
			uconsole_server** console = (uconsole_server**)Utils::GetConsole();
			(*console)->hide(console);
		}
#else
		uconsole_server** console = (uconsole_server**)Utils::GetConsole();
		(*console)->hide(console);
#endif
		char name[272];

#ifdef _WIN64
		if (!Utils::isExodus()) {
			goto suda;
		} else {
			sscanf(args, "%[^,],%f,%d", name, refly_default_speed_exodus, refly_default_cycles_exodus);
			Fly::fly(name, true, *refly_default_speed_exodus, *refly_default_cycles_exodus);
			return;
		}
#endif

	suda:
		Fly::refly_speed = 1.f;
		Fly::refly_cycles = 2;

		sscanf(args, "%[^,],%f,%d", name, &Fly::refly_speed, &Fly::refly_cycles);
		Fly::fly(name, true, Fly::refly_speed, Fly::refly_cycles);
	}
}

void RestoreCommands::cmd_register_commands() {
	uconsole cu = uconsole::uconsole(Utils::GetConsole());

#ifdef _WIN64
	// 1. find constant string
	DWORD64 str_g_toggle_aim = FindPatternInEXE("g_toggle_aim\0", "xxxxxxxxxxxxx");
	DWORD64 str_save_player = FindPatternInEXE("save_player\0", "xxxxxxxxxxxx");

	// 2. find reference to that string
	DWORD64 xref = FindPatternInEXE((char*)&str_g_toggle_aim, "xxxxxxxx");
	DWORD64 xref1 = FindPatternInEXE((char*)&str_save_player, "xxxxxxxx");

	if (Utils::GetGame() == GAME::REDUX) {
		// 3. find pointer to existing command object based on xref
		cmd_mask_struct_ll* pCmd = (cmd_mask_struct_ll*)(xref - offsetof(cmd_mask_struct_ll, _name));
		uconsole_command_ll* pCmd1 = (uconsole_command_ll*)(xref1 - offsetof(uconsole_command_ll, _name));

		ps_actor_flags = pCmd->value;

		// 4. register new commands
		g_god_old.construct(pCmd->__vftable, "g_god", ps_actor_flags, 1, true);
		cu.command_add(&g_god_old);

		g_global_god_old.construct(pCmd->__vftable, "g_global_god", ps_actor_flags, 2, true);
		cu.command_add(&g_global_god_old);

		g_notarget_old.construct(pCmd->__vftable, "g_notarget", ps_actor_flags, 4, true);
		cu.command_add(&g_notarget_old);

		g_unlimitedammo_old.construct(pCmd->__vftable, "g_unlimitedammo", ps_actor_flags, 8, true);
		cu.command_add(&g_unlimitedammo_old);

		g_unlimitedfilters_old.construct(pCmd->__vftable, "g_unlimitedfilters", ps_actor_flags, 128, true);
		cu.command_add(&g_unlimitedfilters_old);

		r_hud_weapon_old.construct(pCmd->__vftable, "r_hud_weapon", igame_hud__flags, 3, false);
		cu.command_add(&r_hud_weapon_old);

		fly_old.construct(&fly_vftable, pCmd1->__vftable, "fly", &fly_execute);
		cu.command_add(&fly_old);

		refly_old.construct(&refly_vftable, pCmd1->__vftable, "refly", &refly_execute);
		cu.command_add(&refly_old);

		signal_old.construct(&signal_vftable, pCmd1->__vftable, "signal", &signal_execute);
		cu.command_add(&signal_old);
	} else {
		// 3. find pointer to existing command object based on xref
		cmd_mask_struct_a1* pCmd = (cmd_mask_struct_a1*)(xref - offsetof(cmd_mask_struct_a1, _name));
		uconsole_command_a1* pCmd1 = (uconsole_command_a1*)(xref1 - offsetof(uconsole_command_a1, _name));

		if (Utils::isExodus()) {
			DWORD64 str_refly_default_cycles = FindPatternInEXE("refly_default_cycles\0", "xxxxxxxxxxxxxxxxxxxxx");
			DWORD64 xref2 = FindPatternInEXE((char*)&str_refly_default_cycles, "xxxxxxxx");
			cmd_integer_struct_a1* pCmd2 = (cmd_integer_struct_a1*)(xref2 - offsetof(cmd_integer_struct_a1, _name));

			DWORD64 str_refly_default_speed = FindPatternInEXE("refly_default_speed\0", "xxxxxxxxxxxxxxxxxxxx");
			DWORD64 xref3 = FindPatternInEXE((char*)&str_refly_default_speed, "xxxxxxxx");
			cmd_float_struct_a1* pCmd3 = (cmd_float_struct_a1*)(xref3 - offsetof(cmd_float_struct_a1, _name));

			refly_default_cycles_exodus = pCmd2->value;
			refly_default_speed_exodus = pCmd3->value;
		}

		ps_actor_flags = pCmd->value;

		// 4. register new commands
		g_god_new.construct(pCmd->__vftable, "g_god", ps_actor_flags, 1, uconsole_cm_type_a1::cm_user);
		cu.command_add(&g_god_new);

		g_global_god_new.construct(pCmd->__vftable, "g_global_god", ps_actor_flags, 2, uconsole_cm_type_a1::cm_user);
		cu.command_add(&g_global_god_new);

		g_notarget_new.construct(pCmd->__vftable, "g_notarget", ps_actor_flags, 4, uconsole_cm_type_a1::cm_user);
		cu.command_add(&g_notarget_new);

		g_unlimitedammo_new.construct(pCmd->__vftable, "g_unlimitedammo", ps_actor_flags, 8, uconsole_cm_type_a1::cm_user);
		cu.command_add(&g_unlimitedammo_new);

		g_unlimitedfilters_new.construct(pCmd->__vftable, "g_unlimitedfilters", ps_actor_flags, 128, uconsole_cm_type_a1::cm_user);
		cu.command_add(&g_unlimitedfilters_new);

		g_godbless_new.construct(pCmd->__vftable, "g_godbless", ps_actor_flags, 1024, uconsole_cm_type_a1::cm_user);
		cu.command_add(&g_godbless_new);

		fly_new.construct(Utils::isExodus() ? &fly_vftable_exodus : &fly_vftable, pCmd1->__vftable, "fly", &fly_execute);
		cu.command_add(&fly_new);

		refly_new.construct(Utils::isExodus() ? &refly_vftable_exodus : &refly_vftable, pCmd1->__vftable, "refly", &refly_execute);
		cu.command_add(&refly_new);

		signal_new.construct(Utils::isExodus() ? &signal_vftable_exodus : &signal_vftable, pCmd1->__vftable, "signal", &signal_execute);
		cu.command_add(&signal_new);
	}
#else
	if (Utils::isLL())
	{
		cu.cmd_mask(&g_god_old, "g_god", ps_actor_flags, 1, true);
		cu.command_add(&g_god_old);

		cu.cmd_mask(&g_global_god_old, "g_global_god", ps_actor_flags, 2, true);
		cu.command_add(&g_global_god_old);

		cu.cmd_mask(&g_notarget_old, "g_notarget", ps_actor_flags, 4, true);
		cu.command_add(&g_notarget_old);

		cu.cmd_mask(&g_unlimitedammo_old, "g_unlimitedammo", ps_actor_flags, 8, true);
		cu.command_add(&g_unlimitedammo_old);

		cu.cmd_mask(&g_unlimitedfilters_old, "g_unlimitedfilters", ps_actor_flags, 128, true);
		cu.command_add(&g_unlimitedfilters_old);

		cu.cmd_mask(&r_hud_weapon_old, "r_hud_weapon", igame_hud__flags, 3, false);
		cu.command_add(&r_hud_weapon_old);

		// LL restore fly cmd
		// Вычисляем указатель ucmd_save_player save_player что-бы вытащить из него __vftable
		// C7 05 ? ? ? ? ? ? ? ? E8 ? ? ? ? 83 C4 04 E8 ? ? ? ? 8B 10 8B C8 8B 42 08 68 ? ? ? ? FF D0 84 1D ? ? ? ? 75 39 8A 0D ? ? ? ? 09 1D ? ? ? ? 80 E1 E7 80 C9 07 68 ? ? ? ? C7 05 ? ? ? ? ? ? ? ? 88 0D ? ? ? ? C7 05 ? ? ? ? ? ? ? ? E8 ? ? ? ? 83 C4 04 E8 ? ? ? ? 8B 10 8B C8 8B 42 08 68 ? ? ? ? FF D0 5B C3
		DWORD mov = FindPatternInEXE(
			"\xC7\x05\x00\x00\x00\x00\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x83\xC4\x04\xE8\x00\x00\x00\x00\x8B\x10\x8B\xC8\x8B\x42\x08\x68\x00\x00\x00\x00\xFF\xD0\x84\x1D\x00\x00\x00\x00\x75\x39\x8A\x0D\x00\x00\x00\x00\x09\x1D\x00\x00\x00\x00\x80\xE1\xE7\x80\xC9\x07\x68\x00\x00\x00\x00\xC7\x05\x00\x00\x00\x00\x00\x00\x00\x00\x88\x0D\x00\x00\x00\x00\xC7\x05\x00\x00\x00\x00\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x83\xC4\x04\xE8\x00\x00\x00\x00\x8B\x10\x8B\xC8\x8B\x42\x08\x68\x00\x00\x00\x00\xFF\xD0\x5B\xC3",
			"xx????????x????xxxx????xxxxxxxx????xxxx????xxxx????xx????xxxxxxx????xx????????xx????xx????????x????xxxx????xxxxxxxx????xxxx");

		uconsole_command_ll* pCmd1 = (uconsole_command_ll*)(*(DWORD*)(mov + 2));

		fly_old.construct(&fly_vftable, pCmd1->__vftable, "fly", &fly_execute);
		cu.command_add(&fly_old);

		refly_old.construct(&refly_vftable, pCmd1->__vftable, "refly", &refly_execute);
		cu.command_add(&refly_old);

		signal_ll.construct(&signal_vftable, pCmd1->__vftable, "signal", &signal_execute);
		cu.command_add(&signal_ll);

		// LL no patches restore r_base_fov
		DWORD str_r_base_fov = FindPatternInEXE("r_base_fov\0", "xxxxxxxxxxx");
		if (str_r_base_fov == NULL)
		{
			// 89 35 ? ? ? ? C7 05 ? ? ? ? ? ? ? ? F3 0F 11 05 ? ? ? ? E8 ? ? ? ? 59 E8 ? ? ? ? 8B 10 68 ? ? ? ? 8B C8 FF 52 08 B8 ? ? ? ? 85 05 ? ? ? ? 75 59 09 05
			mov = FindPatternInEXE(
				"\x89\x35\x00\x00\x00\x00\xC7\x05\x00\x00\x00\x00\x00\x00\x00\x00\xF3\x0F\x11\x05\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x59\xE8\x00\x00\x00\x00\x8B\x10\x68\x00\x00\x00\x00\x8B\xC8\xFF\x52\x08\xB8\x00\x00\x00\x00\x85\x05\x00\x00\x00\x00\x75\x59\x09\x05",
				"xx????xx????????xxxx????x????xx????xxx????xxxxxx????xx????xxxx");

			// Вычисляем указатель cmd_float r_sun_tsm_projection что-бы вытащить из него __vftable
			cmd_float_struct_ll* pCmd = (cmd_float_struct_ll*)(*(DWORD*)(mov + 2));

			// F3 0F 5E 05 ? ? ? ? 0F 28 CC
			DWORD divss = FindPatternInEXE("\xF3\x0F\x5E\x05\x00\x00\x00\x00\x0F\x28\xCC", "xxxx????xxx");

			// Вычисляем адрес fov
			float* r_base_fov_Address = (float*)(*(DWORD*)(divss + 4));

			// Создаём команду
			r_base_fov.construct(pCmd->__vftable, "r_base_fov", r_base_fov_Address, 45.0f, 90.0f, true);
			cu.command_add(&r_base_fov);
		}
	} else {
		// Вычисляем указатель ucmd_save_player save_player что-бы вытащить из него __vftable
		// c7 05 ? ? ? ? ? ? ? ? e8 ? ? ? ? 83 c4 ? 8b 0d ? ? ? ? 3b cb 75 ? e8 ? ? ? ? 3b c3 74 ? 8b f8 e8 ? ? ? ? eb ? 33 c0 8b f0 a3 ? ? ? ? e8 ? ? ? ? 8b 0d ? ? ? ? 8b 01 8b 50 ? 68 ? ? ? ? ff d2 5f
		DWORD mov = FindPatternInEXE(
			"\xc7\x05\x00\x00\x00\x00\x00\x00\x00\x00\xe8\x00\x00\x00\x00\x83\xc4\x00\x8b\x0d\x00\x00\x00\x00\x3b\xcb\x75\x00\xe8\x00\x00\x00\x00\x3b\xc3\x74\x00\x8b\xf8\xe8\x00\x00\x00\x00\xeb\x00\x33\xc0\x8b\xf0\xa3\x00\x00\x00\x00\xe8\x00\x00\x00\x00\x8b\x0d\x00\x00\x00\x00\x8b\x01\x8b\x50\x00\x68\x00\x00\x00\x00\xff\xd2\x5f",
			"xx????????x????xx?xx????xxx?x????xxx?xxx????x?xxxxx????x????xx????xxxx?x????xxx");

		uconsole_command_2033* pCmd1 = (uconsole_command_2033*)(*(DWORD*)(mov + 2));

		signal_2033.construct(&signal_vftable, pCmd1->__vftable, "signal", &signal_execute);
		cu.command_add(&signal_2033);
	}
#endif
}
