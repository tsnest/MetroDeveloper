#include "uconsole.h"
#include "Utils.h"

uconsole::uconsole(void** console, void* cmd_mask_address)
{
	this->console = console;
	this->cmd_mask_address = cmd_mask_address;
}

void uconsole::cmd_mask(void* cmd_mask, const char* name, void* value, unsigned int mask, bool isSave)
{
	typedef void(__thiscall * _cmd_mask) (void* cmd_mask, const char* name, void* value, unsigned int mask, bool isSave);
	((_cmd_mask)this->cmd_mask_address)(cmd_mask, name, value, mask, isSave);
}

void uconsole::command_add(void* C)
{
#ifdef _WIN64
	bool isExodus = (Utils::GetGame() == GAME::EXODUS);
#else
	bool isExodus = false;
#endif

	if (isExodus) {
		((uconsole_server_exodus*) *this->console)->command_add(this->console, C);
	} else {
		((uconsole_server*) *this->console)->command_add(this->console, C);
	}
}
