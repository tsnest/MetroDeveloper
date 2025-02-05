#include "QuickSave.h"
#include "uconsole.h"
#include "Utils.h"

void QuickSave::clevel_r_on_key_press(int action, int key, int state, int resending)
{
	if (key == 63)
	{
#ifdef _WIN64
		bool isExodus = (Utils::GetGame() == GAME::EXODUS);
#else
		bool isExodus = false;
#endif

		if (isExodus) {
			uconsole_server_exodus** console = (uconsole_server_exodus**) Utils::GetConsole();
			(*console)->execute_deferred(console, "gamesave auto_save");
		} else {
			uconsole_server** console = (uconsole_server**) Utils::GetConsole();
			(*console)->execute_deferred(console, "gamesave auto_save");
		}
	}
}
