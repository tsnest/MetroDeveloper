#pragma once

#include "Patcher.h"
#include "RestoreCommands.h"
#include "Utils.h"

typedef void(__thiscall* _command_add) (void* _console, void* cmd);
typedef void(__thiscall* _show_or_hide)(void* _console);
typedef void(__cdecl* _execute) (void* _console, const char* cmd, ...);

struct uconsole_server {
	void* _console;

	void* render;
#ifdef _WIN64
	void* on_frame; // Redux
#endif
	_command_add command_add;
	void* command_remove;
	void* command_find;
	_show_or_hide show;
	_show_or_hide hide;
	_execute execute;
	_execute execute_deferred;
	void* execute_commit;
	void* enumerate;
	void* get_token;
	void* get_float;
	void* get_integer;
#ifdef _WIN64
	void* get_integer_value; // Redux
#endif
};

#ifdef _WIN64
struct uconsole_server_exodus {
	void* _console;

	void* render;
	void* render_finder; // Exodus
	void* on_frame; // Redux
	_command_add command_add;
	void* command_remove;
	void* command_find;
	_show_or_hide show;
	_show_or_hide hide;
	_execute execute;
	_execute execute_deferred;
	void* execute_commit;
	void* enumerate;
	void* get_token;
	void* get_float;
	void* get_integer;
	void* get_integer_value; // Redux
};
#endif

struct uconsole_command_vtbl {
	void* __vftable;

	void* execute;
	void* status;
	void* info;
	void* enum_values;
	void* save;
	void* enabled;
};

#ifdef _WIN64
struct uconsole_command_exodus_vtbl : public uconsole_command_vtbl {
	// данная фигня появилась в каком то из dlc исхода
	// но ничего страшного не произойдёт, если эту структуру мы будем юзать в версии без dlc
	void* default_storage;
};
#endif

#ifndef _WIN64
struct uconsole_command_2033 {
	uconsole_command_vtbl* __vftable;

	const char* _name;
	bool _enabled;
	bool _lower_case_args;
	bool _empty_args_handled;
	bool _save;
	bool _option;
};
#endif

struct uconsole_command_ll {
	uconsole_command_vtbl* __vftable;

	const char* _name;
	bool _enabled : 1;
	bool _lower_case_args : 1;
	bool _empty_args_handled : 1;
	bool _save : 1;
	bool _option : 1;
};

#ifdef _WIN64
enum uconsole_cm_type_a1 {
	cm_volatile,
	cm_user,
	cm_shared,
	cm_action
};

struct uconsole_command_a1 {
	uconsole_command_vtbl* __vftable;

	const char* _name;
	char _type : 2;
	bool _enabled : 1;
	bool _lower_case_args : 1;
	bool _empty_args_handled : 1;
	bool _option : 1;
};
#endif

#ifndef _WIN64
struct cmd_float_struct_2033 : public uconsole_command_2033 {
	float* value; // float pointer
	float min;
	float max;

	void construct(uconsole_command_vtbl* vtable_ptr, const char* name, float* float_ptr, float _min, float _max, bool save)
	{
		__vftable = vtable_ptr;

		_name = name;
		_enabled = true;
		_lower_case_args = true;
		_empty_args_handled = false;
		_save = save;
		_option = false;

		value = float_ptr;
		min = _min;
		max = _max;
	}
};

struct cmd_executor_struct_2033 : public uconsole_command_2033 {
	void construct(uconsole_command_vtbl* vtable_ptr_dst, uconsole_command_vtbl* vtable_ptr_src, const char* name, void* executor_ptr)
	{
		__vftable = vtable_ptr_dst;

		__vftable->__vftable = vtable_ptr_src->__vftable;
		__vftable->execute = executor_ptr;
		__vftable->status = vtable_ptr_src->status;
		__vftable->info = vtable_ptr_src->info;
		__vftable->enum_values = vtable_ptr_src->enum_values;
		__vftable->save = vtable_ptr_src->save;
		__vftable->enabled = vtable_ptr_src->enabled;

		_name = name;
		_enabled = true;
		_lower_case_args = true;
		_empty_args_handled = true;
		_save = false;
		_option = false;
	}
};
#endif

struct cmd_mask_struct_ll : public uconsole_command_ll {
	unsigned int *value; // flags
	unsigned int mask;
	unsigned int mask_on;
	unsigned int mask_off;

	void construct(uconsole_command_vtbl* vtable_ptr, const char *name, unsigned *flags_ptr, unsigned flags_mask, bool save)
	{
		__vftable			= vtable_ptr;

		_name               = name;
		_enabled            = true;
		_lower_case_args    = true;
		_empty_args_handled = false;
		_save				= save;
		_option             = false;
		
		value    = flags_ptr;
		mask     = flags_mask;
		mask_on  = flags_mask;
		mask_off = flags_mask;
	}
};

struct cmd_float_struct_ll : public uconsole_command_ll {
	float* value; // float pointer
	float min;
	float max;

	void construct(uconsole_command_vtbl* vtable_ptr, const char* name, float* float_ptr , float _min, float _max, bool save)
	{
		__vftable			= vtable_ptr;

		_name				= name;
		_enabled			= true;
		_lower_case_args	= true;
		_empty_args_handled = false;
		_save				= save;
		_option				= false;

		value	= float_ptr;
		min		= _min;
		max		= _max;
	}
};

struct cmd_executor_struct_ll : public uconsole_command_ll {
	void construct(uconsole_command_vtbl* vtable_ptr_dst, uconsole_command_vtbl* vtable_ptr_src, const char* name, void* executor_ptr)
	{
		__vftable = vtable_ptr_dst;

		__vftable->__vftable = vtable_ptr_src->__vftable;
		__vftable->execute = executor_ptr;
		__vftable->status = vtable_ptr_src->status;
		__vftable->info = vtable_ptr_src->info;
		__vftable->enum_values = vtable_ptr_src->enum_values;
		__vftable->save = vtable_ptr_src->save;
		__vftable->enabled = vtable_ptr_src->enabled;

		_name = name;
		_enabled = true;
		_lower_case_args = true;
		_empty_args_handled = true;
		_save = false;
		_option = false;
	}
};

#ifdef _WIN64
struct cmd_mask_struct_a1 : public uconsole_command_a1 {
	unsigned int* value; // flags
	unsigned int mask;
	unsigned int mask_on;
	unsigned int mask_off;

	void construct(uconsole_command_vtbl* vtable_ptr, const char* name, unsigned* flags_ptr, unsigned flags_mask, uconsole_cm_type_a1 type)
	{
		__vftable = vtable_ptr;

		_name = name;
		_type = type;
		_enabled = true;
		_lower_case_args = true;
		_empty_args_handled = false;
		_option = false;

		value = flags_ptr;
		mask = flags_mask;
		mask_on = flags_mask;
		mask_off = flags_mask;
	}
};

struct cmd_float_struct_a1 : public uconsole_command_a1 {
	float* value; // float pointer
	float min;
	float max;

	void construct(uconsole_command_vtbl* vtable_ptr, const char* name, float* float_ptr, float _min, float _max, uconsole_cm_type_a1 type)
	{
		__vftable = vtable_ptr;

		_name = name;
		_type = type;
		_enabled = true;
		_lower_case_args = true;
		_empty_args_handled = false;
		_option = false;

		value = float_ptr;
		min = _min;
		max = _max;
	}
};

struct cmd_integer_struct_a1 : public uconsole_command_a1 {
	int* value; // int pointer
	int min;
	int max;

	void construct(uconsole_command_vtbl* vtable_ptr, const char* name, int* int_ptr, int _min, int _max, uconsole_cm_type_a1 type)
	{
		__vftable = vtable_ptr;

		_name = name;
		_type = type;
		_enabled = true;
		_lower_case_args = true;
		_empty_args_handled = false;
		_option = false;

		value = int_ptr;
		min = _min;
		max = _max;
	}
};

struct cmd_executor_struct_a1 : public uconsole_command_a1 {
	void construct(uconsole_command_vtbl* vtable_ptr_dst, uconsole_command_vtbl* vtable_ptr_src, const char* name, void* executor_ptr)
	{
		__vftable = vtable_ptr_dst;

		__vftable->__vftable	= vtable_ptr_src->__vftable;
		__vftable->execute		= executor_ptr;
		__vftable->status		= vtable_ptr_src->status;
		__vftable->info			= vtable_ptr_src->info;
		__vftable->enum_values	= vtable_ptr_src->enum_values;
		__vftable->save			= vtable_ptr_src->save;
		__vftable->enabled		= vtable_ptr_src->enabled;

		if(Utils::isExodus())
			((uconsole_command_exodus_vtbl*)__vftable)->_unk1 = ((uconsole_command_exodus_vtbl*)vtable_ptr_src)->_unk1;

		_name				= name;
		_type				= uconsole_cm_type_a1::cm_user;
		_enabled			= true;
		_lower_case_args	= true;
		_empty_args_handled = true;
		_option				= false;
	}
};
#endif

typedef void(__thiscall* _cmd_mask) (void* cmd_mask, const char* name, void* value, unsigned int mask, bool save);

class uconsole : Patcher
{
private:
	void** console = nullptr;

public:
	uconsole(void** console);

	_cmd_mask cmd_mask = nullptr;

	void command_add(void* C);
};

