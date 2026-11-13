#include "AllowDDS.h"
#include "Utils.h"

AllowDDS::AllowDDS()
{
	if (Utils::GetBool("other", "allow_dds", false))
	{
		LPVOID jne = NULL;

#ifndef _WIN64
		if (Utils::isLL()) {
			// 75 31 8B 3F
			jne = (LPVOID)FindPatternInEXE("\x75\x31\x8B\x3F", "xxxx");

			BYTE JMP[] = { 0xEB };
			ASMWrite(jne, JMP, sizeof(JMP));
		}
#else

		if (Utils::isRedux()) {
			if (Utils::isReduxRelease() || Utils::isReduxE3Demo()) {
				// 75 ? 49 8B 45 ? 48 8D 50 ? 48 85 C0 75 ? 48 8B D6 48 8D 0D - Redux STEAM & E3DEMO
				jne = (LPVOID)FindPatternInEXE(
					"\x75\x00\x49\x8B\x45\x00\x48\x8D\x50\x00\x48\x85\xC0\x75\x00\x48\x8B\xD6\x48\x8D\x0D",
					"x?xxx?xxx?xxxx?xxxxxx");
			} else if (Utils::isReduxEGS()) {
				// 75 6D 48 8B 06 48 8D 0D ? ? ? ? 33 F6 48 85 C0 48 8D 50 14 48 0F 44 D6 E8 ? ? ? ? 49 8B 1C 24 4C 8D 05 ? ? ? ? 48 8D 95 - Redux EGS
				jne = (LPVOID)FindPatternInEXE(
					"\x75\x6D\x48\x8B\x06\x48\x8D\x0D\x00\x00\x00\x00\x33\xF6\x48\x85\xC0\x48\x8D\x50\x14\x48\x0F\x44\xD6\xE8\x00\x00\x00\x00\x49\x8B\x1C\x24\x4C\x8D\x05\x00\x00\x00\x00\x48\x8D\x95",
					"xxxxxxxx????xxxxxxxxxxxxxx????xxxxxxx????xxx");
			} else if (Utils::isReduxNextGen()) {
				// 75 70 49 8B 45 00 48 8D 0D ? ? ? ? 45 33 F6 48 85 C0 48 8D 50 14 49 0F 44 D6 E8 ? ? ? ? 49 8B 34 24 4C 8D 05 ? ? ? ? 48 8D 94 24 - Redux NextGen
				jne = (LPVOID)FindPatternInEXE(
					"\x75\x70\x49\x8B\x45\x00\x48\x8D\x0D\x00\x00\x00\x00\x45\x33\xF6\x48\x85\xC0\x48\x8D\x50\x14\x49\x0F\x44\xD6\xE8\x00\x00\x00\x00\x49\x8B\x34\x24\x4C\x8D\x05\x00\x00\x00\x00\x48\x8D\x94\x24",
					"xxxxxxxxx????xxxxxxxxxxxxxxx????xxxxxxx????xxxx");
			}

			BYTE JMP[] = { 0xEB };
			ASMWrite(jne, JMP, sizeof(JMP));
		} else if (Utils::isArktika() || Utils::isExodus()) {
			// 0F 85 ? ? ? ? 48 8B ? ? ? ? ? 48 8D 1D - Arktika & Exodus
			jne = (LPVOID)FindPatternInEXE(
				"\x0F\x85\x00\x00\x00\x00\x48\x8B\x00\x00\x00\x00\x00\x48\x8D\x1D",
				"xx????xx?????xxx");

			BYTE JMP[] = { 0x90, 0xE9 };
			ASMWrite(jne, JMP, sizeof(JMP));
		}
#endif
	}
}
