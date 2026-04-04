#include "Patcher.h"

MODULEINFO Patcher::mi = Patcher::GetModuleData(NULL);

#ifdef _WIN64
DWORD64 Patcher::FindPattern(DWORD64 start_address, DWORD64 length, char* pattern, char* mask)
{
	for (DWORD64 i = 0; i < length; i++)
		if (DataCompare((BYTE*)(start_address + i), (BYTE*)pattern, mask))
			return (DWORD64)(start_address + i);
	return NULL;
}

DWORD64 Patcher::FindPatternInEXE(char* pattern, char* mask)
{
	return Patcher::FindPattern((DWORD64)mi.lpBaseOfDll, mi.SizeOfImage, pattern, mask);
}

#else

DWORD Patcher::FindPattern(DWORD start_address, DWORD length, char* pattern, char* mask)
{
	for (DWORD i = 0; i < length; i++)
		if (DataCompare((BYTE*)(start_address + i), (BYTE*)pattern, mask))
			return (DWORD)(start_address + i);
	return NULL;
}

DWORD Patcher::FindPatternInEXE(char* pattern, char* mask)
{
	return Patcher::FindPattern((DWORD)mi.lpBaseOfDll, mi.SizeOfImage, pattern, mask);
}
#endif

MODULEINFO Patcher::GetModuleData(char* moduleName)
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

bool Patcher::DataCompare(BYTE* pData, BYTE* pattern, char* mask)
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
