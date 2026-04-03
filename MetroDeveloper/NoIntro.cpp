#include "NoIntro.h"
#include "Utils.h"

NoIntro::NoIntro()
{
	bool isNavMapEnabled = false;

#ifndef _WIN64
	bool is2033 = Utils::is2033();
	isNavMapEnabled = (is2033 && strstr(GetCommandLine(), "-navmap"));
#endif

	if (isNavMapEnabled || Utils::GetBool("other", "nointro", false))
	{
#ifndef _WIN64
		DWORD IntroAddress;

		if (is2033) {
			// 51 0F B7 05 - ORIG 2033
			IntroAddress = FindPatternInEXE("\x51\x0F\xB7\x05", "xxxx");
		} else {
			// 66 A1 ? ? ? ? 66 83 F8 06 73 15 - Last Light
			IntroAddress = FindPatternInEXE("\x66\xA1\x00\x00\x00\x00\x66\x83\xF8\x06\x73\x15", "xx????xxxxxx");
		}

		BYTE ret[] = { 0xC3 };
		ASMWrite((void*)IntroAddress, ret, sizeof(ret));
#else
		DWORD64 IntroAddress = NULL;

		if (Utils::isRedux()) {
			// 73 1D 33 C9 - Redux STEAM
			IntroAddress = FindPatternInEXE("\x73\x1D\x33\xC9", "xxxx");

			if (IntroAddress == NULL) {
				// 73 15 0F B7 C0 48 8D 8D - Redux EGS
				IntroAddress = FindPatternInEXE("\x73\x15\x0F\xB7\xC0\x48\x8D\x8D", "xxxxxxxx");
			}
		} else if (Utils::isExodus()) {
			// 73 ? 0F B7 C0 48 8D 8D - Exodus
			IntroAddress = FindPatternInEXE("\x73\x00\x0F\xB7\xC0\x48\x8D\x8D", "x?xxxxxx");
		}

		if (IntroAddress != NULL) {
			BYTE jmp[] = { 0xEB };
			ASMWrite((void*)IntroAddress, jmp, sizeof(jmp));
		}
#endif
	}
}
