#include "DisableStartupCheck.h"
#include "Utils.h"

DisableStartupCheck::DisableStartupCheck()
{
#ifdef _WIN64
	if (Utils::isRedux() && !Utils::isReduxEGS) {
		// 74 1F 44 8D 49 10 - Redux STEAM
		LPVOID jz = (LPVOID)FindPatternInEXE("\x74\x1F\x44\x8D\x49\x10", "xxxxxx");

		BYTE JMP[] = { 0xEB };
		ASMWrite(jz, JMP, sizeof(JMP));
	}
#endif
}