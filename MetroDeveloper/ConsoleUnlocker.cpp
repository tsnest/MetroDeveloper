#include "ConsoleUnlocker.h"
#include "uconsole.h"
#include "Utils.h"

void ConsoleUnlocker::clevel_r_on_key_press(int action, int key, int state, int resending)
{
	if (key == 41)
	{
#ifdef _WIN64
		bool isExodus = (Utils::GetGame() == GAME::EXODUS);
#else
		bool isExodus = false;
#endif

		if (isExodus) {
			uconsole_server_exodus** console = (uconsole_server_exodus**)Utils::GetConsole();
			(*console)->show(console);
		} else {
			uconsole_server** console = (uconsole_server**)Utils::GetConsole();
			(*console)->show(console);
		}
	}
}
