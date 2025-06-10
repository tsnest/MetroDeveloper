#include "DisableFiltersCostMod.h"
#include "Utils.h"

DisableFiltersCostMod::DisableFiltersCostMod()
{
#ifdef _WIN64
	if (Utils::GetBool("other", "disable_filters_cost_mod", false))
	{
		if (Utils::GetGame() == GAME::REDUX) {
			// f3 0f 59 f0 48 8b 5c 24 - Redux STEAM
			LPVOID mulss = (LPVOID)FindPatternInEXE(
				(BYTE*)"\xf3\x0f\x59\xf0\x48\x8b\x5c\x24",
				"xxxxxxxx");

			if (mulss == NULL) {
				// f3 0f 59 f0 45 0f b7 87 - Redux EGS
				mulss = (LPVOID)FindPatternInEXE(
					(BYTE*)"\xf3\x0f\x59\xf0\x45\x0f\xb7\x87",
					"xxxxxxxx");
			}

			BYTE NOP[] = { 0x90, 0x90, 0x90, 0x90 };
			ASMWrite(mulss, NOP, sizeof(NOP));
		}
	}
#endif
}
