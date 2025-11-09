#define _CRT_SECURE_NO_WARNINGS 1

#include <windows.h>

#include "Patcher.h"
#include "Utils.h"
#include "Hooks.h"
#include "NoVideocardMsg.h"
#include "BadQuitReset.h"
#include "NoIntro.h"
#include "ConsoleUnlocker.h"
#include "ContentUnlocker.h"
#include "AllowDDS.h"
#include "RestoreCommands.h"
#include "Unlock3rdPerson.h"
#include "QuickSave.h"
#include "Fly.h"
#include "LogFile.h"
#include "NavMapGen.h"
#include "DisableFiltersCostMod.h"

#ifdef _WIN64
#include "MenuHack.h"
#include "WeaponHack.h"
#include "UpgradesHack.h"
#include "ShadersHack.h"
#endif

#include "stdio.h"

BOOL APIENTRY DllMain(HINSTANCE hInstDLL, DWORD reason, LPVOID reserved)
{
	if(reason == DLL_PROCESS_ATTACH)
	{
		//AllocConsole();
		//freopen("CONOUT$", "w", stdout);

		if (Utils::GetBool("other", "beep", true)) {
			Beep(1000, 200);
		}

		Patcher::mi = Patcher::GetModuleData(NULL);
		Utils::Utils();

		DisableFiltersCostMod::DisableFiltersCostMod();
		BadQuitReset::BadQuitReset();
		NoIntro::NoIntro();
		AllowDDS::AllowDDS();
		Fly::Fly();
		Unlock3rdPerson::Unlock3rdPerson();
		RestoreCommands::RestoreCommands();
		ContentUnlocker::ContentUnlocker();
		LogFile::LogFile();

#ifndef _WIN64
		NoVideocardMsg::NoVideocardMsg();
		NavMapGen::NavMapGen();
#else
		if (Utils::GetGame() == GAME::ARKTIKA) {
			MenuHack::MenuHack();
			ShadersHack::ShadersHack();
			UpgradesHack::UpgradesHack();
			WeaponHack::WeaponHack();
		}
#endif

		Hooks::Hooks();
	}
	
	return TRUE;
}