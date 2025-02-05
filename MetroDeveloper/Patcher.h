#pragma once
#define _CRT_SECURE_NO_WARNINGS 1 // MSVS dno

#include <Windows.h>

#define PSAPI_VERSION 1
#include <psapi.h>
#pragma comment (lib, "psapi.lib")

class Patcher
{
public:
	static MODULEINFO mi;
	static MODULEINFO GetModuleData(const char* moduleName);

private:
	static bool DataCompare(const BYTE* pData, const BYTE* pattern, const char* mask);

protected:
#ifdef _WIN64
	static DWORD64 FindPattern(DWORD64 start_address, DWORD64 length, BYTE* pattern, const char* mask);
	static DWORD64 FindPatternInEXE(BYTE* pattern, char* mask);
#else
	static DWORD FindPattern(DWORD start_address, DWORD length, BYTE* pattern, const char* mask);
	static DWORD FindPatternInEXE(BYTE* pattern, char* mask);
#endif

	void ASMWrite(void* address, BYTE* code, size_t size);
};

