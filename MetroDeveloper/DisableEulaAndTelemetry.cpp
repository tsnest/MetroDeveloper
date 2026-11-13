#include "DisableEulaAndTelemetry.h"
#include "Utils.h"

DisableEulaAndTelemetry::DisableEulaAndTelemetry()
{
#ifdef _WIN64
	if (Utils::isRedux() && Utils::isReduxNextGen())
	{
		if (Utils::GetBool("other", "disable_eula_and_telemetry_msg", false)) {
			// 75 61 38 8E - Redux NextGen
			LPVOID jnz = (LPVOID)FindPatternInEXE("\x75\x61\x38\x8E", "xxxx");

			BYTE JMP[] = { 0xEB };
			ASMWrite(jnz, JMP, sizeof(JMP));
		}
	}
#endif
}
