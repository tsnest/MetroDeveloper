#pragma once

struct uconsole_command {
	void* __vftable;

	const char* _name;
	char _enabled : 1;
	char _lower_case_args : 1;
	char _empty_args_handled : 1;
	char _save : 1;
	char _option : 1;
};

typedef void(__thiscall* _command_add) (void* _console, uconsole_command* C);
typedef void(__thiscall* _show)(void* _console);
typedef void(__cdecl* _execute_deferred) (void* _console, const char* cmd, ...);

#pragma pack(push, 1)
struct uconsole_server {
	void* _console;

	void* render;
#ifdef _WIN64
	void* on_frame; // redux
#endif
	_command_add command_add;
	void* command_remove;
	void* command_find;
	_show show;
	void* hide;
	void* execute;
	_execute_deferred execute_deferred;
	void* execute_commit;
	void* enumerate;
	void* get_token;
	void* get_float;
	void* get_integer;
#ifdef _WIN64
	void* get_integer_value; // redux
#endif
};
#pragma pack(pop)

struct cmd_mask_struct : public uconsole_command {
	unsigned int *value; // flags
	unsigned int mask;
	unsigned int mask_on;
	unsigned int mask_off;
	
#ifdef _WIN64
	// arktika.1
	// Modera: was ist das? I dont see anything like that in arktika.1
	void* unk1;
	void* unk2;
	void* unk3;
	void* unk4;
#endif

	void construct(void *vtable_ptr, const char *name, unsigned *flags_ptr, unsigned flags_mask)
	{
		__vftable           = vtable_ptr;
		_name               = name;
		_enabled            = 1;
		_lower_case_args    = 1;
		_empty_args_handled = 0;
		_save               = 1;
		_option             = 0;
		
		value    = flags_ptr;
		mask     = flags_mask;
		mask_on  = flags_mask;
		mask_off = flags_mask;
	}
};

class uconsole
{
private:
	uconsole_server** console = nullptr;
	void* cmd_mask_address = nullptr;

public:
	uconsole(uconsole_server** console, void* cmd_mask_address);

	void cmd_mask(cmd_mask_struct* cmd_mask, const char* name, void* value, unsigned int mask, bool isSave);
	void command_add(uconsole_command* C);
};

