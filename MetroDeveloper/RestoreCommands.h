#pragma once
#include "Patcher.h"

class RestoreCommands : public Patcher
{
public:
	RestoreCommands();
	static void cmd_register_commands();
#ifdef _WIN64
	static void __fastcall signal_execute(void* _this, const char* name);
	static void __fastcall fly_execute(void* _this, const char* name);
	static void __fastcall refly_execute(void* _this, const char* args);
#else
	static void __thiscall signal_execute(void* _this, const char* name);
	static void __thiscall fly_execute(void* _this, const char* name);
	static void __thiscall refly_execute(void* _this, const char* args);
#endif
};

