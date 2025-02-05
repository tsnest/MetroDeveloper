#include "MenuHack.h"
#include "Utils.h"

MenuHack::MenuHack()
{
	// menu hack
	// вылет при открытии меню. Даже не разбирался почему происходит.

	if (Utils::GetBool("arktika1", "menu_hack", false)) {
		// 0f 84 ? ? ? ? 0f 28 83 ? ? ? ? 0f 29 83
		LPVOID addr = (LPVOID)FindPatternInEXE(
			(BYTE*)"\x0f\x84\x00\x00\x00\x00\x0f\x28\x83\x00\x00\x00\x00\x0f\x29\x83",
			"xx????xxx????xxx");

		BYTE mem[] = { 0xE9, 0x8D, 0x00, 0x00, 0x00, 0x90 };
		ASMWrite(addr, mem, sizeof(mem));
	}
}
