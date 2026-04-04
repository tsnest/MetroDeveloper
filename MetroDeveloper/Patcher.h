#pragma once
#define _CRT_SECURE_NO_WARNINGS 1

#include <Windows.h>

#define PSAPI_VERSION 1
#include <psapi.h>
#pragma comment (lib, "psapi.lib")

class Patcher
{
public:
	static MODULEINFO GetModuleData(char* moduleName);

private:
	static MODULEINFO mi;
	static bool DataCompare(BYTE* pData, BYTE* pattern, char* mask);

protected:
#ifdef _WIN64
	static DWORD64 FindPattern(DWORD64 start_address, DWORD64 length, char* pattern, char* mask);
	static DWORD64 FindPatternInEXE(char* pattern, char* mask);
#else
	static DWORD FindPattern(DWORD start_address, DWORD length, char* pattern, char* mask);
	static DWORD FindPatternInEXE(char* pattern, char* mask);
#endif

	static void ASMWrite(void* address, BYTE* code, size_t size);
};

