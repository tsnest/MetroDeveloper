#define _CRT_SECURE_NO_WARNINGS 1

#include <windows.h>
#include "stdio.h"

#include "Patcher.h"
#include "Utils.h"
#include "Hooks.h"
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

#ifdef _WIN64
#include "MenuHack.h"
#include "WeaponHack.h"
#include "UpgradesHack.h"
#include "ShadersHack.h"

#include "DisableFiltersCostMod.h"

#else

#include "NavMapGen.h"
#include "NoVideocardMsg.h"
#include "wpn_bobbing_la.h"
#include "SmallFontReplacer2033.h"
#endif

void InitMetroDeveloper()
{
	Utils::Utils();

	if (!Utils::isInited) {
		return;
	}

	if (Utils::GetBool("other", "beep", true)) {
		Beep(1000, 200);
	}

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
	SmallFontReplacer2033::SmallFontReplacer2033();
	install_wpn_bobbing();
	NavMapGen::NavMapGen();
#else
	DisableFiltersCostMod::DisableFiltersCostMod();

	if (Utils::isArktika()) {
		MenuHack::MenuHack();
		ShadersHack::ShadersHack();
		UpgradesHack::UpgradesHack();
		WeaponHack::WeaponHack();
	}
#endif

	Hooks::Hooks();
}

uintptr_t g_ModuleBase = 0;
size_t    g_ModuleSize = 0;
uintptr_t g_EntryPoint = 0;

bool IsExecutable(DWORD protect)
{
	return protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY);
}

bool IsInsideMainModuleRange(uintptr_t addr, size_t size)
{
	uintptr_t end = addr + size;

	return !(end < g_ModuleBase || addr > g_ModuleBase + g_ModuleSize);
}

typedef BOOL(WINAPI* _VirtualProtect)(LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect);
void* VirtualProtect_Address = nullptr;
_VirtualProtect VirtualProtect_Orig = nullptr;

BOOL WINAPI VirtualProtect_Hook(LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect)
{
	BOOL result = VirtualProtect_Orig(lpAddress, dwSize, flNewProtect, lpflOldProtect);

	if (IsExecutable(flNewProtect) && IsInsideMainModuleRange((uintptr_t)lpAddress, dwSize))
	{
		if (!Utils::isInited) {
			InitMetroDeveloper();
		} else {
			MH_RemoveHook(VirtualProtect_Address);
		}
	}

	return result;
}

void InitModuleInfo()
{
	HMODULE hModule = GetModuleHandle(NULL);

	auto dos = (IMAGE_DOS_HEADER*)hModule;
	auto nt = (IMAGE_NT_HEADERS*)((BYTE*)hModule + dos->e_lfanew);

	g_ModuleBase = (uintptr_t)hModule;
	g_ModuleSize = nt->OptionalHeader.SizeOfImage;
	g_EntryPoint = g_ModuleBase + nt->OptionalHeader.AddressOfEntryPoint;
}

BOOL APIENTRY DllMain(HINSTANCE hInstDLL, DWORD reason, LPVOID reserved)
{
	if(reason == DLL_PROCESS_ATTACH)
	{
		//AllocConsole();
		//freopen("CONOUT$", "w", stdout);

		// удаляем старую asi версию
		DeleteFile("MetroDeveloper.asi");

		MH_Initialize();
		InitMetroDeveloper();

		if (!Utils::isInited) {
			InitModuleInfo();

			HMODULE kb = GetModuleHandle("KernelBase.dll");
			VirtualProtect_Address = GetProcAddress(kb, "VirtualProtect");

			Hooks::SetHook("VirtualProtect", VirtualProtect_Address, &VirtualProtect_Hook, (void**)&VirtualProtect_Orig);
		}
	}
	
	return TRUE;
}

typedef HRESULT(WINAPI* _DirectInput8Create)(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter);

#pragma comment(linker, "/export:DirectInput8Create@20=DirectInput8Create")
extern "C" __declspec(dllexport) HRESULT WINAPI DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter)
{
	char path[MAX_PATH];
	GetSystemDirectory(path, MAX_PATH);
	strcat(path, "\\dinput8.dll");

	HMODULE dinput8 = LoadLibrary(path);
	_DirectInput8Create DirectInput8Create_O = (_DirectInput8Create)GetProcAddress(dinput8, "DirectInput8Create");

	return DirectInput8Create_O(hinst, dwVersion, riidltf, ppvOut, punkOuter);
}
