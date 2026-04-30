#pragma once
#include "Patcher.h"
#include "stdint.h"

typedef void (__fastcall* _net_core_update_input)(DWORD64 _this);
typedef DWORD64 (__fastcall* _send_props_pack)(unsigned int pid, void* props);
typedef void* (__fastcall* _uvector_base_inner_try_reserve)(void* _this, unsigned int c);
typedef void (__fastcall* _net_player)(void* _this);

#pragma pack(push, 1)
struct vector_base
{
	uint64_t _array;    // dq (8 байт)
	uint16_t _capacity; // dw (2 байта)
	uint16_t _size;     // dw (2 байта)

	uint8_t _pad[4];    // db ? (4 байта, чтобы добить до 0x10)
};
#pragma pack(pop)

class MultiplayerFix : public Patcher
{
public:
	MultiplayerFix();

	static _net_core_update_input net_core_update_input_Orig;
	static _send_props_pack send_props_pack_Orig;
	static _uvector_base_inner_try_reserve uvector_base_inner_try_reserve;
	static _net_player net_player;

	static void __fastcall net_core_update_input_Hook(DWORD64 _this);
	static void __fastcall send_props_pack_Hook(unsigned int pid, void* props);
	static void net_player_push_back(vector_base* _this);
};

