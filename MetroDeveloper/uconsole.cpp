#include "uconsole.h"

uconsole::uconsole(uconsole_server** console, void* cmd_mask_address)
{
	this->console = console;
	this->cmd_mask_address = cmd_mask_address;
}

void uconsole::cmd_mask(cmd_mask_struct* cmd_mask, const char* name, void* value, unsigned int mask, bool isSave)
{
	typedef void(__thiscall * _cmd_mask) (cmd_mask_struct* cmd_mask, const char* name, void* value, unsigned int mask, bool isSave);
	((_cmd_mask)this->cmd_mask_address)(cmd_mask, name, value, mask, isSave);
}

void uconsole::command_add(uconsole_command* C)
{
	(*this->console)->command_add(this->console, C);
}
