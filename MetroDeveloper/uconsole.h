#pragma once

#pragma pack(push, 1)
struct uconsole_command {
	void* __vftable;

	const char* _name;
	bool unk1;
	bool unk2;
	bool unk3;
	bool unk4;
};
#pragma pack(pop)

typedef void(__thiscall* _command_add) (void* _console, uconsole_command* C);
typedef void(__thiscall* _show)(void* _console);
typedef void(__cdecl* _execute_deffered) (void* _console, const char* cmd, ...);

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
	_execute_deffered execute_deferred;
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

#pragma pack(push, 1)
struct cmd_mask_struct {
	uconsole_command base;

	void* value;
	unsigned int mask;
	unsigned int mask_on;
	unsigned int mask_off;
#ifdef _WIN64
	// arktika.1
	void* unk1;
	void* unk2;
	void* unk3;
	void* unk4;
#endif
};
#pragma pack(pop)

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

