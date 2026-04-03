#include "ContentUnlocker.h"
#include "Utils.h"
#include "stdio.h"

#ifdef _WIN64
void* vfs_ropen_os = nullptr;
void* asset_manager_load = nullptr;
void** ContentUnlocker::g_texture_manager = nullptr;

#else

vfs_ropen_os_LL vfs_ropen_os = nullptr;

#endif

ContentUnlocker::ContentUnlocker()
{
#ifdef _WIN64
	if (Utils::isRedux()) {
		// 48 8B C4 53 55 57 41 56 41 57 48 81 EC - Redux STEAM
		vfs_ropen_os = (_vfs_ropen_os)FindPatternInEXE(
			"\x48\x8B\xC4\x53\x55\x57\x41\x56\x41\x57\x48\x81\xEC",
			"xxxxxxxxxxxxx");

		if (vfs_ropen_os == NULL) {
			// 48 8B C4 48 89 58 10 55 56 57 41 54 41 57 48 81 EC ? ? ? ? 33 DB 4C 8B F9 48 8D 4C 24 ? 48 89 58 20 48 89 58 18 8B FB 8B F3 E8 - Redux EGS
			vfs_ropen_os = (_vfs_ropen_os)FindPatternInEXE(
				"\x48\x8B\xC4\x48\x89\x58\x10\x55\x56\x57\x41\x54\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x33\xDB\x4C\x8B\xF9\x48\x8D\x4C\x24\x00\x48\x89\x58\x20\x48\x89\x58\x18\x8B\xFB\x8B\xF3\xE8",
				"xxxxxxxxxxxxxxxxx????xxxxxxxxx?xxxxxxxxxxxxx");
		}

		// 4c 89 44 24 ? 55 53 56 57 41 55 41 56 48 8d 6c 24 - Redux STEAM
		asset_manager_load = (void*)FindPatternInEXE(
			"\x4c\x89\x44\x24\x00\x55\x53\x56\x57\x41\x55\x41\x56\x48\x8d\x6c\x24",
			"xxxx?xxxxxxxxxxxx");

		if (asset_manager_load == NULL) {
			// 48 89 5C 24 ? 4C 89 44 24 ? 55 56 57 41 55 41 56 48 8D 6C 24 ? 48 81 EC - Redux EGS
			asset_manager_load = (void*)FindPatternInEXE(
				"\x48\x89\x5C\x24\x00\x4C\x89\x44\x24\x00\x55\x56\x57\x41\x55\x41\x56\x48\x8D\x6C\x24\x00\x48\x81\xEC",
				"xxxx?xxxx?xxxxxxxxxxx?xxx");
		}
	} else if (Utils::isArktika()) {
		// 48 8B C4 53 55 56 57 41 56 48 81 EC ? ? ? ? 4C 8B F1 33 DB 48 8D 4C 24 ? 48 89 58 20 48 89 58 18 E8 ? ? ? ? 83 CD FF 48 85 C0 0F 84 ? ? ? ? BA ? ? ? ? 48 89 5C 24 ? 48 8D 4C 24 ? 8B F3 FF 15 ? ? ? ? 89 44 24 20 8B F8 3B C5 75 23 8D 4B 0A - Arktika
		vfs_ropen_os = (_vfs_ropen_os)FindPatternInEXE(
			"\x48\x8B\xC4\x53\x55\x56\x57\x41\x56\x48\x81\xEC\x00\x00\x00\x00\x4C\x8B\xF1\x33\xDB\x48\x8D\x4C\x24\x00\x48\x89\x58\x20\x48\x89\x58\x18\xE8\x00\x00\x00\x00\x83\xCD\xFF\x48\x85\xC0\x0F\x84\x00\x00\x00\x00\xBA\x00\x00\x00\x00\x48\x89\x5C\x24\x00\x48\x8D\x4C\x24\x00\x8B\xF3\xFF\x15\x00\x00\x00\x00\x89\x44\x24\x20\x8B\xF8\x3B\xC5\x75\x23\x8D\x4B\x0A",
			"xxxxxxxxxxxx????xxxxxxxxx?xxxxxxxxx????xxxxxxxx????x????xxxx?xxxx?xxxx????xxxxxxxxxxxxx");

		// 48 89 5C 24 ? 4C 89 4C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 33 FF 4D 8B F1 4D 8B E8 4C 8B FA 4C 8B E1 48 89 BD ? ? ? ? 39 BD - Arktika
		asset_manager_load = (void*)FindPatternInEXE(
			"\x48\x89\x5C\x24\x00\x4C\x89\x4C\x24\x00\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\x6C\x24\x00\x48\x81\xEC\x00\x00\x00\x00\x33\xFF\x4D\x8B\xF1\x4D\x8B\xE8\x4C\x8B\xFA\x4C\x8B\xE1\x48\x89\xBD\x00\x00\x00\x00\x39\xBD",
			"xxxx?xxxx?xxxxxxxxxxxxxxx?xxx????xxxxxxxxxxxxxxxxx????xx");
	} else if (Utils::isExodus()) {
		if (Utils::isExodusPatched) {
			// 48 8B C4 48 89 58 10 48 89 68 18 48 89 70 20 57 41 54 41 56 48 81 EC ? ? ? ? 45 33 E4 48 8B F9 4C 8B C2 4C 89 60 28 BA ? ? ? ? 4C 89 64 24 - Exodus NEW
			vfs_ropen_os = (_vfs_ropen_os)FindPatternInEXE(
				"\x48\x8B\xC4\x48\x89\x58\x10\x48\x89\x68\x18\x48\x89\x70\x20\x57\x41\x54\x41\x56\x48\x81\xEC\x00\x00\x00\x00\x45\x33\xE4\x48\x8B\xF9\x4C\x8B\xC2\x4C\x89\x60\x28\xBA\x00\x00\x00\x00\x4C\x89\x64\x24",
				"xxxxxxxxxxxxxxxxxxxxxxx????xxxxxxxxxxxxxx????xxxx");
		} else {
			// 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 54 41 56 48 81 EC ? ? ? ? 45 31 E4 - Exodus OLD
			vfs_ropen_os = (_vfs_ropen_os)FindPatternInEXE(
				"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x41\x54\x41\x56\x48\x81\xEC\x00\x00\x00\x00\x45\x31\xE4",
				"xxxx?xxxx?xxxx?xxxxxxxx????xxx");

		}
	}
#else
	if (Utils::is2033()) {
		// 55 8B EC 83 E4 ? 81 EC ? ? ? ? 53 8B 1D ? ? ? ? 56 8D 44 24 - ORIG 2033
		vfs_ropen_os = (vfs_ropen_os_LL)FindPatternInEXE(
			"\x55\x8B\xEC\x83\xE4\x00\x81\xEC\x00\x00\x00\x00\x53\x8B\x1D\x00\x00\x00\x00\x56\x8D\x44\x24",
			"xxxxx?xx????xxx????xxxx");
	} else {
		// 55 8B EC 83 E4 ? 81 EC ? ? ? ? 56 57 8B 3D ? ? ? ? 8D 44 24 - ORIG LL
		vfs_ropen_os = (vfs_ropen_os_LL)FindPatternInEXE(
			"\x55\x8B\xEC\x83\xE4\x00\x81\xEC\x00\x00\x00\x00\x56\x57\x8B\x3D\x00\x00\x00\x00\x8D\x44\x24",
			"xxxxx?xx????xxxx????xxx");
	}
#endif
}

#ifdef _WIN64
void* ContentUnlocker::vfs_ropen_package(void* result, const char* fn)
{
	//printf("%s\n", fn);

	if (Utils::FileExists(fn))
	{
		return Utils::isExodus() ? ((_vfs_ropen_osExodus)vfs_ropen_os)(result, fn, NULL, NULL) : ((_vfs_ropen_os)vfs_ropen_os)(result, fn);
	} else {
		return nullptr;
	}
}

void ContentUnlocker::vfs_rbuffered_package(const char* fn, fastdelegate* cb)
{
	//printf("%s\n", fn);

	if (Utils::FileExists(fn))
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

bool ContentUnlocker::vfs_package_registry_level_downloaded(bool orig_ret, const char* map_name)
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

// For redux & arktika
void ContentUnlocker::textures_bin_process_dir(void* asset_manager, const char* dir)
{
	WIN32_FIND_DATA fd;
	HANDLE hFind;

	char str[MAX_PATH];
	char* ext;
	void* s;

	// 1. Process subsequent directories
	strcpy(str, "content\\textures\\");
	if (dir[0] != '\0') {
		strcat(str, dir);
		strcat(str, "\\*");
	} else {
		strcat(str, "*");
	}

	hFind = FindFirstFile(str, &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (strcmp(fd.cFileName, ".") == 0)
				continue;
			if (strcmp(fd.cFileName, "..") == 0)
				continue;

			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				if (dir[0] != '\0') {
					strcpy(str, dir);
					strcat(str, "\\");
					strcat(str, fd.cFileName);
				} else {
					strcpy(str, fd.cFileName);
				}

				textures_bin_process_dir(asset_manager, str);
			}
		} while (FindNextFile(hFind, &fd));

		FindClose(hFind);
	}

	// 2. Process .bin files
	strcpy(str, "content\\textures\\");
	if (dir[0] != '\0') {
		strcat(str, dir);
		strcat(str, "\\*.bin");
	} else {
		strcat(str, "*.bin");
	}

	hFind = FindFirstFile(str, &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (strcmp(fd.cFileName, ".") == 0)
				continue;
			if (strcmp(fd.cFileName, "..") == 0)
				continue;

			if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
				if (dir[0] != '\0') {
					strcpy(str, dir);
					strcat(str, "\\");
					strcat(str, fd.cFileName);
				} else {
					strcpy(str, fd.cFileName);
				}

				// cut the extension
				// string is always end with ".bin", so it's safe
				ext = str + strlen(str) - 4;
				*ext = '\0';

				// make shared string and load texture options
				s = Utils::str_shared(str);

				if (Utils::isRedux()) {
					((_asset_manager_load_redux)asset_manager_load)(*g_texture_manager, &s, NULL);
				} else {
					void* result = _aligned_malloc(0x8, 0x8);
					void* _handles = (void*)((DWORD64)asset_manager + 0x8);

					((_asset_manager_load_arktika)asset_manager_load)(*g_texture_manager, &s, &result, _handles, NULL, 0, 0);
					_aligned_free(result);
				}
			}
		} while (FindNextFile(hFind, &fd));

		FindClose(hFind);
	}
}

#else

void* ContentUnlocker::vfs_ropen(void* result, const char* fn)
{
	//printf("%s\n", fn);

	void* ret = nullptr;

	if (Utils::FileExists(fn)) {
		if (Utils::isLL()) {
			ret = vfs_ropen_os(result, fn);
		} else {
			// â 2033 __usercall
			__asm {
				mov edi, result
				push fn
				call vfs_ropen_os
				add esp, 4
				mov ret, eax
			}
		}
	}

	return ret;
}

void ContentUnlocker::vfs_rbuffered(const char* fn, void* a1, _method method)
{
	//printf("%s\n", fn);

	if (Utils::FileExists(fn))
	{
		size_t size;

		bool isLL = Utils::isLL();
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
