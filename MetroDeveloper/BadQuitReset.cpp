#include "BadQuitReset.h"
#include "Utils.h"

BadQuitReset::BadQuitReset()
{
	if (Utils::GetBool("other", "badquit_reset", false))
	{
		HKEY hKey;
		DWORD disposition;
		if (RegCreateKeyEx(HKEY_CURRENT_USER,
#ifndef _WIN64
		(Utils::GetGame() == GAME::ORIG2033) ? "Software\\4A-Games\\Metro2033" : "Software\\4A-Games\\Metro2034"
#else
			(Utils::GetGame() == GAME::REDUX) ? "Software\\4A-Games\\Metro Redux" : (Utils::GetGame() == GAME::ARKTIKA ? "Software\\4A-Games\\arktika1" : "Software\\4A-Games\\Metro Exodus")
#endif
			, 0, NULL, 0, KEY_SET_VALUE, 0, &hKey,
			&disposition) == ERROR_SUCCESS)
		{
			RegDeleteValue(hKey, "BadQuit");
			RegCloseKey(hKey);
		}
	}
}
