#pragma once
#ifndef _WIN64
#include "Patcher.h"

class NavMapGen : public Patcher
{
public:
	NavMapGen();
	static DWORD WINAPI NavMapThread(LPVOID);
	static void __fastcall slog(const char* s);
};
#endif
