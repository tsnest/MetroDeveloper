#include "RestoreCommands.h"
#include "Utils.h"

void* cmd_mask_Address = nullptr;
unsigned int* ps_actor_flags_Address = nullptr;
void* cmd_mask_vftable_Address = nullptr;

cmd_mask_struct_old g_god_old;
cmd_mask_struct_old g_global_god_old;
cmd_mask_struct_old g_notarget_old;
cmd_mask_struct_old g_unlimitedammo_old;
cmd_mask_struct_old g_unlimitedfilters_old;

cmd_mask_struct_new g_god_new;
cmd_mask_struct_new g_global_god_new;
cmd_mask_struct_new g_notarget_new;
cmd_mask_struct_new g_unlimitedammo_new;
cmd_mask_struct_new g_unlimitedfilters_new;

RestoreCommands::RestoreCommands()
{
#ifndef _WIN64
	if (Utils::GetBool("other", "restore_deleted_commands", false)) {
		if (Utils::GetGame() == GAME::LL) {
			// 8A 54 24 10 8B C1 - Last Light
			cmd_mask_Address = (LPVOID)FindPatternInEXE(
				(BYTE*)"\x8A\x54\x24\x10\x8B\xC1",
				"xxxxxx");

			// c7 05 ? ? ? ? ? ? ? ? 89 1d ? ? ? ? 89 1d ? ? ? ? 89 1d ? ? ? ? e8 ? ? ? ? 83 c4 ? e8 - Last Light
			ps_actor_flags_Address = (unsigned int*)*(DWORD*)(FindPatternInEXE(
				(BYTE*)"\xc7\x05\x00\x00\x00\x00\x00\x00\x00\x00\x89\x1d\x00\x00\x00\x00\x89\x1d\x00\x00\x00\x00\x89\x1d\x00\x00\x00\x00\xe8\x00\x00\x00\x00\x83\xc4\x00\xe8",
				"xx????????xx????xx????xx????x????xx?x") + 6);
		}
	}
#endif
}

void RestoreCommands::cmd_register_commands() {
#ifdef _WIN64
	// 1. find constant string
	const char* str_g_toggle_aim = (const char*)FindPatternInEXE(
		(BYTE*)"g_toggle_aim\0",
		"xxxxxxxxxxxxx");

	// 2. find reference to that string
	const char** xref = (const char**)FindPatternInEXE(
		(BYTE*)&str_g_toggle_aim,
		"xxxxxxxx");

	if (Utils::GetGame() == GAME::REDUX) {
		// 3. find pointer to existing command object based on xref
		cmd_mask_struct_old* pCmd = (cmd_mask_struct_old*)(((char*)xref) - offsetof(cmd_mask_struct_old, _name));

		ps_actor_flags_Address = pCmd->value;
		cmd_mask_vftable_Address = pCmd->__vftable;

		// 4. register new commands
		uconsole cu = uconsole::uconsole(Utils::GetConsole(), NULL);

		g_god_old.construct(cmd_mask_vftable_Address, "g_god", ps_actor_flags_Address, 1);
		cu.command_add(&g_god_old);

		g_global_god_old.construct(cmd_mask_vftable_Address, "g_global_god", ps_actor_flags_Address, 2);
		cu.command_add(&g_global_god_old);

		g_notarget_old.construct(cmd_mask_vftable_Address, "g_notarget", ps_actor_flags_Address, 4);
		cu.command_add(&g_notarget_old);

		g_unlimitedammo_old.construct(cmd_mask_vftable_Address, "g_unlimitedammo", ps_actor_flags_Address, 8);
		cu.command_add(&g_unlimitedammo_old);

		g_unlimitedfilters_old.construct(cmd_mask_vftable_Address, "g_unlimitedfilters", ps_actor_flags_Address, 128);
		cu.command_add(&g_unlimitedfilters_old);
	} else {
		// 3. find pointer to existing command object based on xref
		cmd_mask_struct_new* pCmd = (cmd_mask_struct_new*)(((char*)xref) - offsetof(cmd_mask_struct_new, _name));

		ps_actor_flags_Address = pCmd->value;
		cmd_mask_vftable_Address = pCmd->__vftable;

		// 4. register new commands
		uconsole cu = uconsole::uconsole(Utils::GetConsole(), NULL);

		g_god_new.construct(cmd_mask_vftable_Address, "g_god", ps_actor_flags_Address, 1);
		cu.command_add(&g_god_new);

		g_global_god_new.construct(cmd_mask_vftable_Address, "g_global_god", ps_actor_flags_Address, 2);
		cu.command_add(&g_global_god_new);

		g_notarget_new.construct(cmd_mask_vftable_Address, "g_notarget", ps_actor_flags_Address, 4);
		cu.command_add(&g_notarget_new);

		g_unlimitedammo_new.construct(cmd_mask_vftable_Address, "g_unlimitedammo", ps_actor_flags_Address, 8);
		cu.command_add(&g_unlimitedammo_new);

		g_unlimitedfilters_new.construct(cmd_mask_vftable_Address, "g_unlimitedfilters", ps_actor_flags_Address, 128);
		cu.command_add(&g_unlimitedfilters_new);
	}
#else
	uconsole cu = uconsole::uconsole(Utils::GetConsole(), cmd_mask_Address);

	cu.cmd_mask(&g_god_old, "g_god", ps_actor_flags_Address, 1, false);
	cu.command_add(&g_god_old);

	cu.cmd_mask(&g_global_god_old, "g_global_god", ps_actor_flags_Address, 2, false);
	cu.command_add(&g_global_god_old);

	cu.cmd_mask(&g_notarget_old, "g_notarget", ps_actor_flags_Address, 4, false);
	cu.command_add(&g_notarget_old);

	cu.cmd_mask(&g_unlimitedammo_old, "g_unlimitedammo", ps_actor_flags_Address, 8, false);
	cu.command_add(&g_unlimitedammo_old);

	cu.cmd_mask(&g_unlimitedfilters_old, "g_unlimitedfilters", ps_actor_flags_Address, 128, false);
	cu.command_add(&g_unlimitedfilters_old);
#endif
}
