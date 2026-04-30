#include "MultiplayerFix.h"
#include "Utils.h"
#include "stdio.h"
#include <intrin.h>

_net_core_update_input MultiplayerFix::net_core_update_input_Orig = nullptr;
_send_props_pack MultiplayerFix::send_props_pack_Orig = nullptr;
_uvector_base_inner_try_reserve MultiplayerFix::uvector_base_inner_try_reserve = nullptr;
_net_player MultiplayerFix::net_player = nullptr;
static vector_base* p_players = nullptr;

MultiplayerFix::MultiplayerFix()
{
	if (Utils::isRedux() && !Utils::isReduxEGS && Utils::GetBool("other", "multiplayer_fix", false)) {
		// 48 89 74 24 ? 57 48 83 ec ? 0f b7 41 ? 8b fa 48 8b f1 3b d0 0f 84 ? ? ? ? 0f 82 ? ? ? ? 48 89 5c 24 ? 85 d2 74 ? 8b d8 d1 eb 03 d8 3b da 73 ? 8b da 48 89 6c 24 ? 81 fb ? ? ? ? 76 ? 8b da 81 fa ? ? ? ? 77 ? 8b d3 e8 ? ? ? ? 48 8b e8 48 85 c0 75 ? 3b df 76 ? 8b d7 8b df e8 ? ? ? ? 48 8b e8 48 85 c0 75 ? 33 c0 48 8b 6c 24 ? 48 8b 5c 24 ? 48 8b 74 24 ? 48 83 c4 ? 5f c3 48 8b 16 0f b7 7e ? 48 3b d5 74 ? 8b cf 6b c9 - Redux STEAM
		uvector_base_inner_try_reserve = (_uvector_base_inner_try_reserve)FindPatternInEXE(
			"\x48\x89\x74\x24\x00\x57\x48\x83\xec\x00\x0f\xb7\x41\x00\x8b\xfa\x48\x8b\xf1\x3b\xd0\x0f\x84\x00\x00\x00\x00\x0f\x82\x00\x00\x00\x00\x48\x89\x5c\x24\x00\x85\xd2\x74\x00\x8b\xd8\xd1\xeb\x03\xd8\x3b\xda\x73\x00\x8b\xda\x48\x89\x6c\x24\x00\x81\xfb\x00\x00\x00\x00\x76\x00\x8b\xda\x81\xfa\x00\x00\x00\x00\x77\x00\x8b\xd3\xe8\x00\x00\x00\x00\x48\x8b\xe8\x48\x85\xc0\x75\x00\x3b\xdf\x76\x00\x8b\xd7\x8b\xdf\xe8\x00\x00\x00\x00\x48\x8b\xe8\x48\x85\xc0\x75\x00\x33\xc0\x48\x8b\x6c\x24\x00\x48\x8b\x5c\x24\x00\x48\x8b\x74\x24\x00\x48\x83\xc4\x00\x5f\xc3\x48\x8b\x16\x0f\xb7\x7e\x00\x48\x3b\xd5\x74\x00\x8b\xcf\x6b\xc9",
			"xxxx?xxxx?xxx?xxxxxxxxx????xx????xxxx?xxx?xxxxxxxxx?xxxxxx?xx????x?xxxx????x?xxx????xxxxxxx?xxx?xxxxx????xxxxxxx?xxxxxx?xxxx?xxxx?xxx?xxxxxxxx?xxxx?xxxx");

		// 40 53 48 83 ec ? 48 8b d9 33 c9 48 8d 05 ? ? ? ? 48 89 4b - Redux STEAM
		net_player = (_net_player)FindPatternInEXE(
			"\x40\x53\x48\x83\xec\x00\x48\x8b\xd9\x33\xc9\x48\x8d\x05\x00\x00\x00\x00\x48\x89\x4b",
			"xxxxx?xxxxxxxx????xxx");
	}
}

void __fastcall MultiplayerFix::net_core_update_input_Hook(DWORD64 _this)
{
	p_players = (vector_base*)(_this + 0x18);

	net_core_update_input_Orig(_this);
}

void __fastcall MultiplayerFix::send_props_pack_Hook(unsigned int pid, void* props)
{
	if (*(DWORD*)_ReturnAddress() == 0x6BCD8B41 && pid == p_players->_size) {
		net_player_push_back(p_players);
	}

	send_props_pack_Orig(pid, props);
}

void MultiplayerFix::net_player_push_back(vector_base* _this)
{
	unsigned int size = _this->_size;

	if (_this->_capacity >= size + 1 || uvector_base_inner_try_reserve(_this, size + 1)) {
		void* p = (void*)((char*)_this->_array + 112 * size);

		if (p) {
			net_player(p);
		}

		_this->_size = size + 1;
	}
}
