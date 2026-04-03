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
		(Utils::is2033()) ? "Software\\4A-Games\\Metro2033" : "Software\\4A-Games\\Metro2034"
#else
			(Utils::isRedux()) ? "Software\\4A-Games\\Metro Redux" : (Utils::isArktika() ? "Software\\4A-Games\\arktika1" : "Software\\4A-Games\\Metro Exodus")
#endif
			, 0, NULL, 0, KEY_SET_VALUE, 0, &hKey,
			&disposition) == ERROR_SUCCESS)
		{
			RegDeleteValue(hKey, "BadQuit");
			RegCloseKey(hKey);
		}
	}
}
