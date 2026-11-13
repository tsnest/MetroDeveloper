#include "DisableFiltersCostMod.h"
#include "Utils.h"

DisableFiltersCostMod::DisableFiltersCostMod()
{
#ifdef _WIN64
	if (Utils::isRedux())
	{
		if (Utils::GetBool("other", "disable_filters_cost_mod", false)) {
			LPVOID mulss = NULL;

			if (Utils::isReduxRelease() || Utils::isReduxE3Demo()) {
				// f3 0f 59 f0 48 8b 5c 24 - Redux STEAM & E3DEMO
				mulss = (LPVOID)FindPatternInEXE("\xf3\x0f\x59\xf0\x48\x8b\x5c\x24", "xxxxxxxx");
			} else if (Utils::isReduxEGS()) {
				// f3 0f 59 f0 45 0f b7 87 - Redux EGS
				mulss = (LPVOID)FindPatternInEXE("\xf3\x0f\x59\xf0\x45\x0f\xb7\x87", "xxxxxxxx");
			} else if (Utils::isReduxNextGen()) {
				// F3 0F 59 F0 45 0F B7 8F - Redux NextGen
				mulss = (LPVOID)FindPatternInEXE("\xF3\x0F\x59\xF0\x45\x0F\xB7\x8F", "xxxxxxxx");
			}

			BYTE NOP[] = { 0x90, 0x90, 0x90, 0x90 };
			ASMWrite(mulss, NOP, sizeof(NOP));
		}
	}
#endif
}
