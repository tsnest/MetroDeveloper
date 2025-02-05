#pragma once

struct uconsole_command_old {
	void* __vftable;

	const char* _name;
	char _enabled : 1;
	char _lower_case_args : 1;
	char _empty_args_handled : 1;
	char _save : 1;
	char _option : 1;
};

struct uconsole_command_new {
	void* __vftable;

	const char* _name;
	char _unk1 : 1; // Arktika
	char _enabled : 1;
	char _lower_case_args : 1;
	char _empty_args_handled : 1;
	char _save : 1;
	char _option : 1;
};

typedef void(__thiscall* _command_add) (void* _console, void* C);
typedef void(__thiscall* _show)(void* _console);
typedef void(__cdecl* _execute_deferred) (void* _console, const char* cmd, ...);

#pragma pack(push, 1)
struct uconsole_server {
	void* _console;

	void* render;
#ifdef _WIN64
	void* on_frame; // Redux
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
	void* get_integer_value; // Redux
#endif
};
#pragma pack(pop)

#pragma pack(push, 1)
struct uconsole_server_exodus {
	void* _console;

	void* render;
	void* on_frame; // Redux
	void* unk1; // Exodus
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
	void* get_integer_value; // Redux
};
#pragma pack(pop)

struct cmd_mask_struct_old : public uconsole_command_old {
	unsigned int *value; // flags
	unsigned int mask;
	unsigned int mask_on;
	unsigned int mask_off;

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

struct cmd_mask_struct_new : public uconsole_command_new {
	unsigned int* value; // flags
	unsigned int mask;
	unsigned int mask_on;
	unsigned int mask_off;

	void construct(void* vtable_ptr, const char* name, unsigned* flags_ptr, unsigned flags_mask)
	{
		__vftable = vtable_ptr;
		_name = name;
		_enabled = 1;
		_lower_case_args = 1;
		_empty_args_handled = 0;
		_save = 1;
		_option = 0;

		value = flags_ptr;
		mask = flags_mask;
		mask_on = flags_mask;
		mask_off = flags_mask;
	}
};

class uconsole
{
private:
	void** console = nullptr;
	void* cmd_mask_address = nullptr;

public:
	uconsole(void** console, void* cmd_mask_address);

	void cmd_mask(void* cmd_mask, const char* name, void* value, unsigned int mask, bool isSave);
	void command_add(void* C);
};

