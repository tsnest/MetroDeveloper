#include "Patcher.h"

MODULEINFO Patcher::mi;

#ifdef _WIN64
DWORD64 Patcher::FindPattern(DWORD64 start_address, DWORD64 length, BYTE* pattern, const char* mask)
{
	for (DWORD64 i = 0; i < length; i++)
		if (DataCompare((BYTE*)(start_address + i), pattern, mask))
			return (DWORD64)(start_address + i);
	return NULL;
}

DWORD64 Patcher::FindPatternInEXE(BYTE* pattern, char* mask)
{
	return Patcher::FindPattern((DWORD64)mi.lpBaseOfDll, mi.SizeOfImage, pattern, mask);
}

#else

DWORD Patcher::FindPattern(DWORD start_address, DWORD length, BYTE* pattern, const char* mask)
{
	for (DWORD i = 0; i < length; i++)
		if (DataCompare((BYTE*)(start_address + i), pattern, mask))
			return (DWORD)(start_address + i);
	return NULL;
}

DWORD Patcher::FindPatternInEXE(BYTE* pattern, char* mask)
{
	return Patcher::FindPattern((DWORD)mi.lpBaseOfDll, mi.SizeOfImage, pattern, mask);
}
#endif

MODULEINFO Patcher::GetModuleData(const char* moduleName)
{
	MODULEINFO currentModuleInfo = { 0 };
	HMODULE moduleHandle = GetModuleHandle(moduleName);
	if (moduleHandle == NULL)
	{
		return currentModuleInfo;
	}
	GetModuleInformation(GetCurrentProcess(), moduleHandle, &currentModuleInfo, sizeof(MODULEINFO));
	return currentModuleInfo;
}

bool Patcher::DataCompare(const BYTE* pData, const BYTE* pattern, const char* mask)
{
	for (; *mask; mask++, pData++, pattern++)
		if (*mask == 'x' && *pData != *pattern)
			return false;
	return (*mask) == NULL;
}

void Patcher::ASMWrite(void* address, BYTE* code, size_t size)
{
	DWORD OldProtect = NULL;
	VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &OldProtect);
	memcpy(address, code, size);
	VirtualProtect(address, size, OldProtect, &OldProtect);
}
