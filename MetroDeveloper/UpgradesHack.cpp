#include "UpgradesHack.h"
#include "Utils.h"

UpgradesHack::UpgradesHack()
{
	// upgrades hack
	// почему-то как всегда в 4А движке апгрейды оружия и всего остального сделаны очень сложно
	// поэтому аж в трёх местах предусмотреы специальные хаки для арктики
	// нужно их отключить, инаке ресурсы не подойдут при запуске с ключом -build_key m3

	if (Utils::GetBool("arktika1", "upgrades_hack", false)) {
		// cplayer::load_dynamic, апгрейды костюма
		// пробовал добавлять секции которых там нехватает - ничего хорошего из этого не вышло
		// 0f 85 ? ? ? ? 48 8b 15 ? ? ? ? 48 85 d2 74 ? 49 8b c6 4c 8d 05 ? ? ? ? 0f b6 4c 02
		LPVOID addr1 = (LPVOID)FindPatternInEXE(
			(BYTE*)"\x0f\x85\x00\x00\x00\x00\x48\x8b\x15\x00\x00\x00\x00\x48\x85\xd2\x74\x00\x49\x8b\xc6\x4c\x8d\x05\x00\x00\x00\x00\x0f\xb6\x4c\x02",
			"xx????xxx????xxxx?xxxxxx????xxxx");

		BYTE mem1[] = { 0xE9, 0xC6, 0x02, 0x00, 0x00, 0x90 };
		ASMWrite(addr1, mem1, sizeof(mem1));

		// загружать конфиг upgradables_registry_ovr вместо upgradables_registry
		// можно конечно его добавить в config.bin, но мне лень
		// 48 0f 44 c2 48 8d 4c 24 ? ba
		LPVOID addr2 = (LPVOID)FindPatternInEXE(
			(BYTE*)"\x48\x0f\x44\xc2\x48\x8d\x4c\x24\x00\xba",
			"xxxxxxxx?x");

		BYTE mem2[] = { 0x90, 0x90, 0x90, 0x90 };
		ASMWrite(addr2, mem2, sizeof(mem2));

		// ещё какая-то хрень вылетающая при спауне оружия
		// даже в теории не знаю как нужно это фиксить, так что просто отключаем
		// 0f 94 c0 85 c0 74 ? 48 8b 03 48 85 c0
		LPVOID addr3 = (LPVOID)FindPatternInEXE(
			(BYTE*)"\x0f\x94\xc0\x85\xc0\x74\x00\x48\x8b\x03\x48\x85\xc0",
			"xxxxxx?xxxxxx");

		BYTE mem3[] = { 0x0F, 0x95, 0xC0 };
		ASMWrite(addr3, mem3, sizeof(mem3));
	}
}
