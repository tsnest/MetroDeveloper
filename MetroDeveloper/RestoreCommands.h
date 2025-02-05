#pragma once
#include "Patcher.h"

class RestoreCommands : public Patcher
{
public:
	RestoreCommands();
	static void cmd_register_commands();
};

