#include "NoIntro.h"
#include "Utils.h"

static bool isNavMapEnabled = false;
static bool isLL = false;

NoIntro::NoIntro()
{
#ifndef _WIN64
	isLL = Utils::isLL();
	isNavMapEnabled = (!isLL && strstr(GetCommandLine(), "-navmap"));
#endif

	if (isNavMapEnabled || Utils::GetBool("other", "nointro", false))
	{
		LPVOID IntroAddress;
#ifndef _WIN64
		if (!isLL)
		{
			// 51 0F B7 05 - ORIG 2033
			IntroAddress = (LPVOID)FindPatternInEXE(
				(BYTE*)"\x51\x0F\xB7\x05",
				"xxxx");
		}
		else
		{
			// 66 A1 ? ? ? ? 66 83 F8 06 73 15 - Last Light
			IntroAddress = (LPVOID)FindPatternInEXE(
				(BYTE*)"\x66\xA1\x00\x00\x00\x00\x66\x83\xF8\x06\x73\x15",
				"xx????xxxxxx");
		}

		BYTE ret[] = { 0xC3 };
		ASMWrite(IntroAddress, ret, sizeof(ret));
#else
		if (Utils::GetGame() == GAME::REDUX) {
			// 73 1D 33 C9 - Redux STEAM
			IntroAddress = (LPVOID)FindPatternInEXE(
				(BYTE*)"\x73\x1D\x33\xC9",
				"xxxx");

			if (IntroAddress == NULL) {
				// 73 15 0F B7 C0 48 8D 8D - Redux EGS
				IntroAddress = (LPVOID)FindPatternInEXE(
					(BYTE*)"\x73\x15\x0F\xB7\xC0\x48\x8D\x8D",
					"xxxxxxxx");
			}

			BYTE jmp[] = { 0xEB };
			ASMWrite(IntroAddress, jmp, sizeof(jmp));
		} else if (Utils::GetGame() == GAME::EXODUS) {
			// 73 3B 0F B7 C0 48 8D 8D - Exodus
			IntroAddress = (LPVOID)FindPatternInEXE(
				(BYTE*)"\x73\x3B\x0F\xB7\xC0\x48\x8D\x8D",
				"xxxxxxxx");
			BYTE jmp[] = { 0xEB };
			ASMWrite(IntroAddress, jmp, sizeof(jmp));
		}
#endif
	}
}
