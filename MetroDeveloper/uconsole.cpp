#include "uconsole.h"
#include "Utils.h"

uconsole::uconsole(void** console)
{
	this->console = console;

#ifndef _WIN64
	if (Utils::isLL()) {
		// 8A 54 24 10 8B C1 - Last Light
		this->cmd_mask = (_cmd_mask)FindPatternInEXE("\x8A\x54\x24\x10\x8B\xC1", "xxxxxx");
	} else {
		this->cmd_mask = NULL;
	}
#else
	this->cmd_mask = NULL;
#endif
}

void uconsole::command_add(void* cmd)
{
#ifdef _WIN64
	if (Utils::isExodus()) {
		((uconsole_server_exodus*) *this->console)->command_add(this->console, cmd);
	} else {
		((uconsole_server*) *this->console)->command_add(this->console, cmd);
	}
#else
	((uconsole_server*)*this->console)->command_add(this->console, cmd);
#endif
}
