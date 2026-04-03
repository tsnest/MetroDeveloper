#ifndef _WIN64
#include "SmallFontReplacer2033.h"

char* SmallFontReplacer2033::font_arial = "arial";
char* SmallFontReplacer2033::font_console = "console";

SmallFontReplacer2033::SmallFontReplacer2033()
{
	if (Utils::is2033()) {
		string256 str;
		Utils::GetString("other", "replace_small_font_2033", "small", str, sizeof(str));

		// ba ? ? ? ? e8 ? ? ? ? 68 ? ? ? ? 8d 7e - console font
		DWORD mov1 = FindPatternInEXE(
			"\xba\x00\x00\x00\x00\xe8\x00\x00\x00\x00\x68\x00\x00\x00\x00\x8d\x7e",
			"x????x????x????xx") + 1;

		DWORD mov2 = NULL;

		if (Utils::is2033Patched) {
			// BA ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 33 C0 - stats font patched
			mov2 = FindPatternInEXE(
				"\xBA\x00\x00\x00\x00\xE8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x33\xC0",
				"x????x????x????xx") + 1;
		} else {
			// BA ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 5F - stats font non patched
			mov2 = FindPatternInEXE(
				"\xBA\x00\x00\x00\x00\xE8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x5F",
				"x????x????x????x") + 1;
		}

		if (strcmp(str, "arial") == 0) {
			ASMWrite((void*)mov1, (BYTE*)&font_arial, sizeof(char*));
			ASMWrite((void*)mov2, (BYTE*)&font_arial, sizeof(char*));
		} else if (strcmp(str, "console") == 0) {
			ASMWrite((void*)mov1, (BYTE*)&font_console, sizeof(char*));
			ASMWrite((void*)mov2, (BYTE*)&font_console, sizeof(char*));
		}
	}
}
#endif