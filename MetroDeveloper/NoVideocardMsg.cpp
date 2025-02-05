#include "NoVideocardMsg.h"
#include "Utils.h"

NoVideocardMsg::NoVideocardMsg()
{
#ifndef _WIN64
	if ((Utils::GetGame() == GAME::ORIG2033) && Utils::GetBool("other", "no_videocard_msg", false)) // Only metro 2033
	{
		// 68 ? ? ? ? BF ? ? ? ? 8D 74 24 ? E8 ? ? ? ? 83 C4 ? 80 7C 24 ? ? 75 - для версии с патчами
		LPVOID VideoMsgAddress = (LPVOID)FindPattern(
			(DWORD)mi.lpBaseOfDll,
			mi.SizeOfImage,
			(BYTE*)"\x68\x00\x00\x00\x00\xBF\x00\x00\x00\x00\x8D\x74\x24\x00\xE8\x00\x00\x00\x00\x83\xC4\x00\x80\x7C\x24\x00\x00\x75",
			"x????x????xxx?x????xx?xxx??x");
		BYTE VideoMsgJMP[] = { 0xEB, 0x5B };
		if (VideoMsgAddress == NULL)
		{
			// 68 ? ? ? ? BF ? ? ? ? 8D 74 24 ? E8 ? ? ? ? 83 C4 ? 80 7C 24 ? ? 74 ? 8D 7C 24 - для версии без патчей
			VideoMsgAddress = (LPVOID)FindPattern(
				(DWORD)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\x68\x00\x00\x00\x00\xBF\x00\x00\x00\x00\x8D\x74\x24\x00\xE8\x00\x00\x00\x00\x83\xC4\x00\x80\x7C\x24\x00\x00\x74\x00\x8D\x7C\x24",
				"x????x????xxx?x????xx?xxx??x?xxx");
			VideoMsgJMP[1] = 0x78;
		}
		ASMWrite(VideoMsgAddress, VideoMsgJMP, sizeof(VideoMsgJMP));
	}
#endif
}
