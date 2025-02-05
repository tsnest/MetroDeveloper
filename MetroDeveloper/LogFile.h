#pragma once
#include "Patcher.h"

class LogFile : public Patcher
{
public:
	LogFile();
	static void __fastcall slog(const char* s);
};

