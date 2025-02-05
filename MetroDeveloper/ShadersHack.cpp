#include "ShadersHack.h"
#include "Utils.h"

ShadersHack::ShadersHack()
{
	// shaders hack
	// в арктике.1 по умолчанию ко всем именам шейдеров дописывается /_OCULUS_=1, именно под такими именами шейдеры записаны в блоб
	// однако если запустить игру с параметром -build_key m3 то /_OCULUS_=1 к именам шейдеров не дописывается, и движок крашится
	// т.к. не может найти в блобе нужные шейдеры.
	// ввиду того что переделывать блоб напряжно уберём проверку чтобы /_OCULUS_=1 дописывался всегда, не зависимо от ключа

	if (Utils::GetBool("arktika1", "shaders_hack", false)) {
		// 0f 84 ? ? ? ? 49 8b 06 4c 8d 40
		LPVOID addr = (LPVOID)FindPatternInEXE(
			(BYTE*)"\x0f\x84\x00\x00\x00\x00\x49\x8b\x06\x4c\x8d\x40",
			"xx????xxxxxx");

		BYTE mem[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
		ASMWrite(addr, mem, sizeof(mem));
	}
}
