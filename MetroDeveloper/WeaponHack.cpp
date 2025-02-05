#include "WeaponHack.h"
#include "Utils.h"

WeaponHack::WeaponHack()
{
	// weapon hack
	// как я понял вылет происходит из за того что отсутствует дополнительный ентить магазина для оружия
	// вот это уже очень плохо и нужно разобраться как магазин заспаунить. Но как-нибудь потом

	if (Utils::GetBool("arktika1", "weapon_hack", false)) {
		// 0f 84 ? ? ? ? 4c 8b 00 48 8b c8 41 ff 90
		LPVOID addr = (LPVOID)FindPatternInEXE(
			(BYTE*)"\x0f\x84\x00\x00\x00\x00\x4c\x8b\x00\x48\x8b\xc8\x41\xff\x90",
			"xx????xxxxxxxxx");

		BYTE mem[] = { 0xE9, 0xC6, 0x00, 0x00, 0x00, 0x90 };
		ASMWrite(addr, mem, sizeof(mem));
	}
}
