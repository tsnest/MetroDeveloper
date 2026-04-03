#pragma once
#ifndef _WIN64
#include "Patcher.h"
#include "Utils.h"

class SmallFontReplacer2033 : public Patcher
{
public:
	SmallFontReplacer2033();

	static char* font_arial;
	static char* font_console;
};

#endif