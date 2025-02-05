#include "ContentUnlocker.h"
#include "Utils.h"
#include "stdio.h"

#ifdef _WIN64
typedef void* (__fastcall* _vfs_ropen_os)(void* result, const char* fn);
typedef void* (__fastcall* _vfs_ropen_osExodus)(void* result, const char* fn, void* _unused, void* _unknown);
void* vfs_ropen_os = nullptr;
#endif

static bool isLL = true;

ContentUnlocker::ContentUnlocker()
{
#ifndef _WIN64
	isLL = Utils::isLL();
#else
	if (Utils::GetGame() == GAME::REDUX) {
		// 48 8B C4 53 55 57 41 56 41 57 48 81 EC - Redux STEAM
		vfs_ropen_os = (_vfs_ropen_os)FindPatternInEXE(
			(BYTE*)"\x48\x8B\xC4\x53\x55\x57\x41\x56\x41\x57\x48\x81\xEC",
			"xxxxxxxxxxxxx");

		if (vfs_ropen_os == NULL) {
			// 48 8B C4 48 89 58 10 55 56 57 41 54 41 57 48 81 EC ? ? ? ? 33 DB 4C 8B F9 48 8D 4C 24 ? 48 89 58 20 48 89 58 18 8B FB 8B F3 E8 - Redux EGS
			vfs_ropen_os = (_vfs_ropen_os)FindPatternInEXE(
				(BYTE*)"\x48\x8B\xC4\x48\x89\x58\x10\x55\x56\x57\x41\x54\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x33\xDB\x4C\x8B\xF9\x48\x8D\x4C\x24\x00\x48\x89\x58\x20\x48\x89\x58\x18\x8B\xFB\x8B\xF3\xE8",
				"xxxxxxxxxxxxxxxxx????xxxxxxxxx?xxxxxxxxxxxxx");
		}
	} else if (Utils::GetGame() == GAME::ARKTIKA) {
		// 48 8B C4 53 55 56 57 41 56 48 81 EC ? ? ? ? 4C 8B F1 33 DB 48 8D 4C 24 ? 48 89 58 20 48 89 58 18 E8 ? ? ? ? 83 CD FF 48 85 C0 0F 84 ? ? ? ? BA ? ? ? ? 48 89 5C 24 ? 48 8D 4C 24 ? 8B F3 FF 15 ? ? ? ? 89 44 24 20 8B F8 3B C5 75 23 8D 4B 0A - Arktika
		vfs_ropen_os = (_vfs_ropen_os)FindPatternInEXE(
			(BYTE*)"\x48\x8B\xC4\x53\x55\x56\x57\x41\x56\x48\x81\xEC\x00\x00\x00\x00\x4C\x8B\xF1\x33\xDB\x48\x8D\x4C\x24\x00\x48\x89\x58\x20\x48\x89\x58\x18\xE8\x00\x00\x00\x00\x83\xCD\xFF\x48\x85\xC0\x0F\x84\x00\x00\x00\x00\xBA\x00\x00\x00\x00\x48\x89\x5C\x24\x00\x48\x8D\x4C\x24\x00\x8B\xF3\xFF\x15\x00\x00\x00\x00\x89\x44\x24\x20\x8B\xF8\x3B\xC5\x75\x23\x8D\x4B\x0A",
			"xxxxxxxxxxxx????xxxxxxxxx?xxxxxxxxx????xxxxxxxx????x????xxxx?xxxx?xxxx????xxxxxxxxxxxxx");
	} else if (Utils::GetGame() == GAME::EXODUS) {
		// 48 8B C4 48 89 58 10 48 89 68 18 48 89 70 20 57 41 54 41 56 48 81 EC ? ? ? ? 45 33 E4 48 8B F9 4C 8B C2 4C 89 60 28 BA ? ? ? ? 4C 89 64 24 - Exodus
		vfs_ropen_os = (_vfs_ropen_os)FindPatternInEXE(
			(BYTE*)"\x48\x8B\xC4\x48\x89\x58\x10\x48\x89\x68\x18\x48\x89\x70\x20\x57\x41\x54\x41\x56\x48\x81\xEC\x00\x00\x00\x00\x45\x33\xE4\x48\x8B\xF9\x4C\x8B\xC2\x4C\x89\x60\x28\xBA\x00\x00\x00\x00\x4C\x89\x64\x24",
			"xxxxxxxxxxxxxxxxxxxxxxx????xxxxxxxxxxxxxx????xxxx");
	}
#endif
}

#ifdef _WIN64
void* __fastcall ContentUnlocker::vfs_ropen_package(void* result, void* package, const char* fn, const int force_raw, unsigned int* uncompressed_size)
{
	//printf("%s\n", fn);

	if (GetFileAttributes(fn) != 0xFFFFFFFF)
	{
		return Utils::GetGame() == GAME::EXODUS ? ((_vfs_ropen_osExodus)vfs_ropen_os)(result, fn, NULL, NULL) : ((_vfs_ropen_os)vfs_ropen_os)(result, fn);
	} else {
		return nullptr;
	}
}

void __fastcall ContentUnlocker::vfs_rbuffered_package(const char* fn, fastdelegate* cb)
{
	//printf("%s\n", fn);

	if (GetFileAttributes(fn) != 0xFFFFFFFF)
	{
		size_t size;
		void* buffer = malloc(0x30000);
		if (buffer)
		{
			FILE* f = fopen(fn, "rb");
			if (f)
			{
				while (size = fread(buffer, 1, 0x30000, f))
					cb->method(cb->object, buffer, size);

				fclose(f);
			}
			free(buffer);
			return;
		}
	}
}

bool __fastcall ContentUnlocker::vfs_package_registry_level_downloaded(bool orig_ret, const char* map_name)
{
	if (!orig_ret)
	{
		char map_path[256];
		strcpy(map_path, "content\\maps\\");
		strcat(map_path, map_name);
		strcat(map_path, "\\level");
		if (Utils::FileExists(map_path))
			orig_ret = true;
	}

	return orig_ret;
}

#else

void __cdecl ContentUnlocker::vfs_rbuffered(const char* fn, void* a1, _method method)
{
	//printf("%s\n", fn);

	if (GetFileAttributes(fn) != 0xFFFFFFFF)
	{
		size_t size;

		size_t buffer_size = (isLL ? 0x10000 : 0x20000);
		void* buffer = malloc(buffer_size);

		if (buffer)
		{
			FILE* f = fopen(fn, "rb");
			if (f)
			{
				while (size = fread(buffer, 1, buffer_size, f))
					method(a1, isLL ? &buffer : (void**)buffer, size);

				fclose(f);
			}
			free(buffer);
			return;
		}
	}
}
#endif
