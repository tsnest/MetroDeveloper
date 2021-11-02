#define _CRT_SECURE_NO_WARNINGS 1 // MSVS dno

#include <windows.h>
#include <vector>
#include "i_pathengine.h"
#include "MinHook.h"
#include "model.hpp"

#define PSAPI_VERSION 1
#include <psapi.h>
#pragma comment (lib, "psapi.lib")

// settings
typedef char string256[256];

string256 logFilename;
bool isLogEnabled;

bool isNavMapEnabled;
bool isLL;

#ifndef _WIN64
string256 navmapFormat;
string256 navmapFilename;
string256 navmapResult;
bool navmap_bin_mode;
bool supply_debug_info_bin;
bool navmap_exit;
bool isNavMapThreadCreated = false;

extern void convert_tok_to_bin(const void* tok_data, size_t tok_size, void** bin_data, size_t* bin_size, int _debug);
#endif

// signature scanner
MODULEINFO mi;

MODULEINFO GetModuleData(const char* moduleName)
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

bool DataCompare(const BYTE* pData, const BYTE* pattern, const char* mask)
{
	for (; *mask; mask++, pData++, pattern++)
		if (*mask == 'x' && *pData != *pattern)
			return false;
	return (*mask) == NULL;
}

#ifndef _WIN64
DWORD FindPattern(DWORD start_address, DWORD length, BYTE* pattern, char* mask)
{
	for (DWORD i = 0; i < length; i++)
		if (DataCompare((BYTE*)(start_address + i), pattern, mask))
			return (DWORD)(start_address + i);
	return NULL;
}
#else
DWORD64 FindPattern(DWORD64 start_address, DWORD64 length, BYTE* pattern, char* mask)
{
	for (DWORD64 i = 0; i < length; i++)
		if (DataCompare((BYTE*)(start_address + i), pattern, mask))
			return (DWORD64)(start_address + i);
	return NULL;
}
#endif

void BadQuitReset()
{
	HKEY hKey;
	DWORD disposition;
	if (RegCreateKeyEx(HKEY_CURRENT_USER,
#ifndef _WIN64
		!isLL ? "Software\\4A-Games\\Metro2033" : "Software\\4A-Games\\Metro2034"
#else
		"Software\\4A-Games\\Metro Redux"
#endif
		, 0, NULL, 0, KEY_SET_VALUE, 0, &hKey,
		&disposition) == ERROR_SUCCESS)
	{
		RegDeleteValue(hKey, "BadQuit");
		RegCloseKey(hKey);
	}
}

#ifndef _WIN64
typedef void* (__stdcall* _getConsole)();
_getConsole getConsole = nullptr;

void uconsole_server_impl_execute_deffered(void* console, const char* fmt)
{
	if (console != nullptr)
	{
		typedef void(__cdecl* _execute_deffered) (void* console, const char* cmd, ...);
		_execute_deffered execute_deffered = (_execute_deffered) * (DWORD*)(*(DWORD*)console + 32);
		execute_deffered(console, fmt);
	}
}

void uconsole_server_impl_execute(void* console, const char* fmt)
{
	if (console != nullptr)
	{
		typedef void(__cdecl* _execute) (void* console, const char* cmd, ...);
		_execute execute = (_execute) * (DWORD*)(*(DWORD*)console + 28);
		execute(console, fmt);
	}
}

// nav_map part
class MemoryStreamImpl : public iOutputStream
{
	public:
	std::vector<char> m_data;

	bool save(const char *filename, bool isLL)
	{
		bool result;

		FILE* out = fopen(filename, "wb");
		if (!out)
			return false;

		if (!isLL)
		{
			size_t written = fwrite(&m_data.front(), 1, m_data.size(), out);
			result = (written == m_data.size());
		}
		else
		{
			void* bin_data;
			size_t bin_size;
			convert_tok_to_bin(&m_data[0], m_data.size(), &bin_data, &bin_size, supply_debug_info_bin);
			size_t written = fwrite(bin_data, bin_size, 1, out);
			result = (written == 1);
			free(bin_data);
		}

		fclose(out);

		return result;
	}

	const char *ptr()
	{
		return &m_data.front();
	}

	size_t size()
	{
		return m_data.size();
	}

	virtual void put(const char *data, tUnsigned32 dataSize)
	{
		size_t pos = m_data.size();
		m_data.resize(m_data.size() + dataSize);
		memcpy(&m_data[pos], data, dataSize);
	}

	void putInt(int value)
	{
		put((char*)&value, sizeof(value));
	}

	void putFloat(float value)
	{
		put((char*)&value, sizeof(value));
	}
};

iMesh* load_raw(iPathEngine* pathengine, const char* filename)
{
	FaceVertexMeshImpl m;

	if (!m.load_raw(filename))
	{
		printf("raw mesh '%s' load failed\n", filename);
		return NULL;
	}

	iFaceVertexMesh const* const pm = &m;
	iMesh* real_mesh = pathengine->buildMeshFromContent(&pm, 1, 0);

	return real_mesh;
}

iMesh* load_4a(iPathEngine* pathengine, const char* filename)
{
	FaceVertexMeshImpl m;

	if (!m.load_4a(filename))
	{
		printf("4A mesh '%s' load failed\n", filename);
		return NULL;
	}

	iFaceVertexMesh const* const pm = &m;
	iMesh* real_mesh = pathengine->buildMeshFromContent(&pm, 1, 0);

	return real_mesh;
}

iMesh* load_xml(iPathEngine* pathengine, const char* filename)
{
	FILE* f = fopen(filename, "rb");
	if (!f)
		return NULL;

	fseek(f, 0, SEEK_END);
	size_t length = ftell(f);
	fseek(f, 0, SEEK_SET);

	void* buffer = malloc(length);
	fread(buffer, 1, length, f);

	fclose(f);

	iMesh* real_mesh = pathengine->loadMeshFromBuffer("xml", (char*)buffer, length, 0);
	free(buffer);

	return real_mesh;
}

void create_shapes_2033(iPathEngine* pathengine, iShape** shape1, iShape** shape2, iShape** shape3)
{
	tSigned32 shape1_data[] = {
		-300, 0,
		-150, 259,
		150, 259,
		300, 0,
		150, -259,
		-150, -259
	};

	*shape1 = pathengine->newShape(6, shape1_data);

	tSigned32 shape2_data[] = {
		-600, 0,
		-300, 519,
		300, 519,
		600, 0,
		300, -519,
		-300, -519
	};

	*shape2 = pathengine->newShape(6, shape2_data);

	tSigned32 shape3_data[] = {
		-100, 0,
		-50, 86,
		50, 86,
		100, 0,
		50, -86,
		-50, -86
	};

	*shape3 = pathengine->newShape(6, shape3_data);
}

void create_shapes_ll_and_redux(iPathEngine* pathengine, iShape** shape1, iShape** shape2, iShape** shape3)
{
	tSigned32 shape1_data_ll[] = {
		-350, 0,
		-247, 247,
		0, 350,
		247, 247,
		350, 0,
		247, -247,
		0, -350,
		-247, -247
	};

	*shape1 = pathengine->newShape(8, shape1_data_ll);

	tSigned32 shape2_data_ll[] = {
		-600, 0,
		-424, 424,
		0, 600,
		424, 424,
		600, 0,
		424, -424,
		0, -600,
		-424, -424
	};

	*shape2 = pathengine->newShape(8, shape2_data_ll);

	tSigned32 shape3_data_ll[] = {
		-100, 0,
		-70, 70,
		0, 100,
		70, 70,
		100, 0,
		70, -70,
		0, -100,
		-70, -70
	};

	*shape3 = pathengine->newShape(8, shape3_data_ll);
}

void savemesh(iPathEngine *pathengine)
{
	iMesh *real_mesh;
	
	if (strcmp(navmapFormat, "4a") == 0)
		real_mesh = load_4a(pathengine, navmapFilename);
	else if(strcmp(navmapFormat, "raw") == 0)
		real_mesh = load_raw(pathengine, navmapFilename);
	else if(strcmp(navmapFormat, "xml") == 0)
		real_mesh = load_xml(pathengine, navmapFilename);
	else
		real_mesh = NULL;

	printf("real_mesh = %08X\n", real_mesh);
	if(!real_mesh)
		return;

	iCollisionContext *ctx = real_mesh->newContext();
	printf("ctx = %08X\n", ctx);
	if(!ctx)
	{
		delete real_mesh;
		return;
	}

	ctx->setSurfaceTypeTraverseCost(1, 0.1000f);
	real_mesh->burnContextIntoMesh(ctx);

	iShape* shape1, * shape2, * shape3;
	if (!navmap_bin_mode /*isLL*/)
	{
		create_shapes_2033(pathengine, &shape1, &shape2, &shape3);
	}
	else
	{
		create_shapes_ll_and_redux(pathengine, &shape1, &shape2, &shape3);
	}

	printf("shape1 = %08X\nshape2 = %08X\nshape3 = %08X\n", shape1, shape2, shape3);

	if (!shape1 || !shape2 || !shape3)
	{
		delete shape3;
		delete shape2;
		delete shape1;
		delete ctx;
		delete real_mesh;
		return;
	}

	const char *cp_options[] = {
		//"connectOverlappingShapeExpansions", "true",
		//"enableConnectedRegionQueries", "true",
		0
	};

	const char *pfp_options[] = {
		"enableConnectedRegionQueries", "true",
		0
	};

	real_mesh->generateCollisionPreprocessFor(shape1, cp_options);
	real_mesh->generateCollisionPreprocessFor(shape2, cp_options);
	real_mesh->generateCollisionPreprocessFor(shape3, cp_options);
	real_mesh->generatePathfindPreprocessFor(shape1, pfp_options);
	real_mesh->generatePathfindPreprocessFor(shape2, pfp_options);
	real_mesh->generatePathfindPreprocessFor(shape3, pfp_options);

	MemoryStreamImpl ms_mesh, ms_cp[3], ms_pfp[3];

	real_mesh->saveGround("tok", true, &ms_mesh);
	real_mesh->saveCollisionPreprocessFor(shape1, &ms_cp[0]);
	real_mesh->saveCollisionPreprocessFor(shape2, &ms_cp[1]);
	real_mesh->saveCollisionPreprocessFor(shape3, &ms_cp[2]);
	real_mesh->savePathfindPreprocessFor(shape1, &ms_pfp[0]);
	real_mesh->savePathfindPreprocessFor(shape2, &ms_pfp[1]);
	real_mesh->savePathfindPreprocessFor(shape3, &ms_pfp[2]);

	// compile together ;)
	MemoryStreamImpl result;

	result.putInt(ms_mesh.size());
	result.put(ms_mesh.ptr(), ms_mesh.size());

	result.putInt(3); // unknown, probably preprocess count

	result.putFloat(navmap_bin_mode /*isLL*/ ? 0.35f : 0.3f); // unknwown1
	result.putInt(ms_cp[0].size());
	result.put(ms_cp[0].ptr(), ms_cp[0].size());
	result.putInt(ms_pfp[0].size());
	result.put(ms_pfp[0].ptr(), ms_pfp[0].size());

	result.putFloat(0.6f); // unknwown2
	result.putInt(ms_cp[1].size());
	result.put(ms_cp[1].ptr(), ms_cp[1].size());
	result.putInt(ms_pfp[1].size());
	result.put(ms_pfp[1].ptr(), ms_pfp[1].size());

	result.putFloat(0.1f); // unknwown3
	result.putInt(ms_cp[2].size());
	result.put(ms_cp[2].ptr(), ms_cp[2].size());
	result.putInt(ms_pfp[2].size());
	result.put(ms_pfp[2].ptr(), ms_pfp[2].size());

	result.save(navmapResult, navmap_bin_mode /*isLL*/);

	// free resources and exit
	delete shape3;
	delete shape2;
	delete shape1;

	delete ctx;

	delete real_mesh;

	return;
}

DWORD WINAPI NavMapThread(LPVOID)
{
	typedef iPathEngine* (__stdcall* _getPathEngine)();

	// B8 ? ? ? ? A3 ? ? ? ? A3 ? ? ? ? B8 ? ? ? ? A3 ? ? ? ? A3 ? ? ? ? B8 ? ? ? ? A3 ? ? ? ? A3 ? ? ? ? B8 ? ? ? ? A3 ? ? ? ? A3 ? ? ? ? B8
	_getPathEngine getPathEngine = (_getPathEngine)FindPattern(
		(DWORD)mi.lpBaseOfDll,
		mi.SizeOfImage,
		(BYTE*)"\xB8\x00\x00\x00\x00\xA3\x00\x00\x00\x00\xA3\x00\x00\x00\x00\xB8\x00\x00\x00\x00\xA3\x00\x00\x00\x00\xA3\x00\x00\x00\x00\xB8\x00\x00\x00\x00\xA3\x00\x00\x00\x00\xA3\x00\x00\x00\x00\xB8\x00\x00\x00\x00\xA3\x00\x00\x00\x00\xA3\x00\x00\x00\x00\xB8",
		"x????x????x????x????x????x????x????x????x????x????x????x????x");

	iPathEngine* pathEngine = getPathEngine();

	printf("pPathEngine = %08X\n", pathEngine);
	printf("InterfaceMajorVersion %d\n", pathEngine->getInterfaceMajorVersion());
	printf("InterfaceMinorVersion %d\n", pathEngine->getInterfaceMinorVersion());

	tSigned32 i, j, k;
	pathEngine->getReleaseNumbers(i, j, k);
	printf("ReleaseNumbers %d %d %d\n", i, j, k);

	savemesh(pathEngine);

	if (navmap_exit)
	{
		uconsole_server_impl_execute_deffered(getConsole(), "quit");
	}

	return 0;
}
#endif

// log part
FILE* fLog;
CRITICAL_SECTION logCS;

typedef void(__thiscall* _slog)(const char* s);
_slog slog_Orig = nullptr;

void __fastcall slog_Hook(const char* s)
{
#ifndef _WIN64
	if (isNavMapEnabled && !isNavMapThreadCreated)
	{
		if (strstr(s, "* [loader] map loaded in "))
		{
			isNavMapThreadCreated = true;
			HANDLE thread = CreateThread(NULL, 0, NavMapThread, NULL, 0, NULL);
			CloseHandle(thread);
		}
	}
#endif

	if (isLogEnabled)
	{
		EnterCriticalSection(&logCS);

		fprintf(fLog, s);
		fputc('\n', fLog);
		fflush(fLog);

		LeaveCriticalSection(&logCS);
	}

	slog_Orig(s);
}

void ASMWrite(void* address, BYTE* code, size_t size)
{
	DWORD OldProtect = NULL;
	VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &OldProtect);
	memcpy(address, code, size);
	VirtualProtect(address, size, OldProtect, &OldProtect);
}

void getString(const char* section_name, const char* str_name, const char* default_str, char* result, DWORD size)
{
	GetPrivateProfileString(section_name, str_name, default_str, result, size, ".\\MetroDeveloper.ini");
}

bool getBool(const char* section_name, const char* bool_name, bool default_bool)
{
	string256 str;
	getString(section_name, bool_name, (default_bool ? "true" : "false"), str, sizeof(str));
	return (strcmp(str, "true") == 0) || (strcmp(str, "yes") == 0) || (strcmp(str, "on") == 0) || (strcmp(str, "1") == 0);
}

typedef void(__thiscall* _uconsole_server_impl_show)(void* console);
_uconsole_server_impl_show uconsole_server_impl_show = nullptr;
void* clevel_r_on_key_press_Orig = nullptr;

#ifndef _WIN64
void __fastcall clevel_r_on_key_press_Hook2033(void* _this, void* _unused, int action, int key, int state)
{
	//printf("action = %d, key = %d, state = %d\n", action, key, state);

	if (key == 41)
	{
		uconsole_server_impl_show(getConsole());
	}

	typedef void(__thiscall* _clevel_r_on_key_press_2033)(void* _this, int action, int key, int state);
	((_clevel_r_on_key_press_2033)clevel_r_on_key_press_Orig)(_this, action, key, state);
}

void __fastcall clevel_r_on_key_press_Hook(void* _this, void* _unused, int action, int key, int state, int resending)
#else
void** g_console = nullptr;
void __fastcall clevel_r_on_key_press_Hook(void* _this, int action, int key, int state, int resending)
#endif
{
	//printf("action = %d, key = %d, state = %d, resending = %d\n", action, key, state, resending);

	if (key == 41)
	{
#ifndef _WIN64
		uconsole_server_impl_show(getConsole());
#else
		uconsole_server_impl_show(*g_console);
#endif
	}

	typedef void(__thiscall* _clevel_r_on_key_press)(void* _this, int action, int key, int state, int resending);
	((_clevel_r_on_key_press)clevel_r_on_key_press_Orig)(_this, action, key, state, resending);
}

#ifndef _WIN64
LPVOID vfs_ropen_Orig = nullptr;
LPVOID vfs_ropen_os = nullptr;

typedef char(__cdecl* _method)(void* a1, void** buffer, size_t size);
typedef void(__cdecl* _vfs_rbuffered)(const char* fn, void* a1, _method method);
_vfs_rbuffered vfs_rbuffered_Orig = nullptr;

//char format[] = "%s\n";
__declspec(naked) void vfs_ropen_Hook(/*const char* fn*/)
{
	__asm
	{
		/*
		mov eax, [esp + 4]
		push eax
		mov eax, offset format
		push eax
		call printf
		add esp, 8
		*/

		mov eax, [esp + 4]
		push eax
		call GetFileAttributesA
		cmp eax, 0xFFFFFFFF
		je to_orig_code
	}

	if (!isLL)
	{
		__asm
		{
			push edi
			push esi
			mov edi, esi
			mov eax, [esp + 0x0C]
			push eax
			call vfs_ropen_os
			add esp, 4
			pop esi
			pop edi
			ret
		}
	}
	else
	{
		__asm
		{
			mov eax, [esp + 4]
			push eax
			push esi
			call vfs_ropen_os
			add esp, 8
			ret
		}
	}

	__asm
	{
	to_orig_code:
		jmp vfs_ropen_Orig
	}
}

void __cdecl vfs_rbuffered_Hook(const char* fn, void* a1, _method method)
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

	vfs_rbuffered_Orig(fn, a1, method);
}
#else
typedef void* (__fastcall* _vfs_ropen_package)(void* result, void* package, const char* fn, const int force_raw, unsigned int* uncompressed_size);
typedef void* (__fastcall* _vfs_ropen_os)(void* result, const char* fn);
typedef void (__fastcall* _vfs_rbuffered_package)(void* package, const char* fn, void* cb, const int force_raw);
typedef void* (__fastcall* _rblock_init)(const char* fn, unsigned int* f_offset, unsigned int* f_size, unsigned int not_packaged);

_vfs_ropen_package vfs_ropen_package_Orig = nullptr;
_vfs_ropen_os vfs_ropen_os = nullptr;
_vfs_rbuffered_package vfs_rbuffered_package_Orig = nullptr;
_rblock_init rblock_init_Orig = nullptr;

void* __fastcall vfs_ropen_package_Hook(void* result, void* package, const char* fn, const int force_raw, unsigned int* uncompressed_size)
{
	//printf("%s\n", fn);

	if (GetFileAttributes(fn) != 0xFFFFFFFF)
	{
		return vfs_ropen_os(result, fn);
	}

	return vfs_ropen_package_Orig(result, package, fn, force_raw, uncompressed_size);
}

struct fastdelegate
{
	void* object;
	bool (*method)(void* object, LPVOID& buffer, size_t size);
};

void __fastcall vfs_rbuffered_package_Hook(void* package, const char* fn, fastdelegate* cb, const int force_raw)
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

	vfs_rbuffered_package_Orig(package, fn, cb, force_raw);
}

// TEST
void* __fastcall rblock_init_Hook(const char* fn, unsigned int* f_offset, unsigned int* f_size, unsigned int not_packaged)
{
	printf("%s\n", fn);
	return rblock_init_Orig(fn, f_offset, f_size, not_packaged);
}
#endif

BOOL APIENTRY DllMain(HINSTANCE hInstDLL, DWORD reason, LPVOID reserved)
{
	if(reason == DLL_PROCESS_ATTACH)
	{
		//AllocConsole();
		//freopen("CONOUT$", "w", stdout);

		if (getBool("other", "beep", true))
		{
			Beep(1000, 200);
		}

		mi = GetModuleData(NULL);

		bool minhook = (MH_Initialize() == MH_OK);
		if (!minhook)
		{
			MessageBox(NULL, "MinHook not initialized!", "MinHook", MB_OK | MB_ICONERROR);
		}

#ifndef _WIN64
		isLL = (GetModuleHandle("MetroLL.exe") != NULL);
#else
		isLL = true;
#endif

		isLogEnabled = getBool("log", "enabled", false);
		if(isLogEnabled)
		{
			getString("log", "filename", "uengine.log", logFilename, sizeof(logFilename));
			fLog = fopen(logFilename, "w");
			if (fLog != NULL)
			{
				InitializeCriticalSection(&logCS);
			}
		}

		if (isLL && getBool("other", "allow_dds", false))
		{
#ifndef _WIN64
			// 75 31 8B 3F
			LPVOID jne = (LPVOID)FindPattern(
				(DWORD)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\x75\x31\x8B\x3F",
				"xxxx");
#else
			// 75 ? 49 8B 45 ? 48 8D 50 ? 48 85 C0 75 ? 48 8B D6 48 8D 0D
			LPVOID jne = (LPVOID)FindPattern(
				(DWORD64)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\x75\x00\x49\x8B\x45\x00\x48\x8D\x50\x00\x48\x85\xC0\x75\x00\x48\x8B\xD6\x48\x8D\x0D",
				"x?xxx?xxx?xxxx?xxxxxx");
#endif

			BYTE JMP[] = { 0xEB };
			ASMWrite(jne, JMP, sizeof(JMP));
		}

		if (getBool("other", "unlock_content_folder", false))
		{
#ifndef _WIN64
			// 55 8B EC 83 E4 ? 83 EC ? 53 57 8D 44 24
			LPVOID vfs_ropen_Address = (LPVOID)FindPattern(
				(DWORD)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\x55\x8B\xEC\x83\xE4\x00\x83\xEC\x00\x53\x57\x8D\x44\x24",
				"xxxxx?xx?xxxxx");

			LPVOID vfs_rbuffered_Address;

			if (!isLL)
			{
				// 55 8B EC 83 E4 ? 81 EC ? ? ? ? 53 8B 1D ? ? ? ? 56 8D 44 24
				vfs_ropen_os = (LPVOID)FindPattern(
					(DWORD)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x55\x8B\xEC\x83\xE4\x00\x81\xEC\x00\x00\x00\x00\x53\x8B\x1D\x00\x00\x00\x00\x56\x8D\x44\x24",
					"xxxxx?xx????xxx????xxxx");

				// 55 8B EC 83 E4 ? 83 EC ? 53 56 57 8D 44 24 ? 50
				vfs_rbuffered_Address = (LPVOID)FindPattern(
					(DWORD)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x55\x8B\xEC\x83\xE4\x00\x83\xEC\x00\x53\x56\x57\x8D\x44\x24\x00\x50",
					"xxxxx?xx?xxxxxx?x");
			}
			else
			{
				// 55 8B EC 83 E4 ? 81 EC ? ? ? ? 56 57 8B 3D ? ? ? ? 8D 44 24
				vfs_ropen_os = (LPVOID)FindPattern(
					(DWORD)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x55\x8B\xEC\x83\xE4\x00\x81\xEC\x00\x00\x00\x00\x56\x57\x8B\x3D\x00\x00\x00\x00\x8D\x44\x24",
					"xxxxx?xx????xxxx????xxx");

				// 83 EC ? 53 55 56 57 8D 44 24 ? 50
				vfs_rbuffered_Address = (LPVOID)FindPattern(
					(DWORD)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x83\xEC\x00\x53\x55\x56\x57\x8D\x44\x24\x00\x50",
					"xx?xxxxxxx?x");
			}

			if (MH_CreateHook(vfs_ropen_Address, &vfs_ropen_Hook, reinterpret_cast<LPVOID*>(&vfs_ropen_Orig)) == MH_OK) {
				if (MH_EnableHook(vfs_ropen_Address) != MH_OK) {
					MessageBox(NULL, "MH_EnableHook() != MH_OK", "vfs_ropen Hook", MB_OK | MB_ICONERROR);
				}
			} else {
				MessageBox(NULL, "MH_CreateHook() != MH_OK", "vfs_ropen Hook", MB_OK | MB_ICONERROR);
			}

			if (MH_CreateHook(vfs_rbuffered_Address, &vfs_rbuffered_Hook, reinterpret_cast<LPVOID*>(&vfs_rbuffered_Orig)) == MH_OK) {
				if (MH_EnableHook(vfs_rbuffered_Address) != MH_OK) {
					MessageBox(NULL, "MH_EnableHook() != MH_OK", "vfs_rbuffered Hook", MB_OK | MB_ICONERROR);
				}
			} else {
				MessageBox(NULL, "MH_CreateHook() != MH_OK", "vfs_rbuffered Hook", MB_OK | MB_ICONERROR);
			}
#else
			// 48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 54 41 56 41 57 48 83 EC ? 45 33 E4 45 8B F9
			LPVOID vfs_ropen_package_Address = (LPVOID)FindPattern(
				(DWORD64)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x56\x57\x41\x54\x41\x56\x41\x57\x48\x83\xEC\x00\x45\x33\xE4\x45\x8B\xF9",
				"xxxx?xxxx?xxxxxxxxxxx?xxxxxx");

			// 48 8B C4 53 55 57 41 56 41 57 48 81 EC
			vfs_ropen_os = (_vfs_ropen_os)FindPattern(
				(DWORD64)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\x48\x8B\xC4\x53\x55\x57\x41\x56\x41\x57\x48\x81\xEC",
				"xxxxxxxxxxxxx");

			// 48 89 5C 24 ? 48 89 74 24 ? 41 56 48 83 EC ? 83 79
			LPVOID vfs_rbuffered_package_Address = (LPVOID)FindPattern(
				(DWORD64)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x41\x56\x48\x83\xEC\x00\x83\x79",
				"xxxx?xxxx?xxxxx?xx");

			// 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 41 56 48 81 EC ? ? ? ? 33 ED
			LPVOID rblock_init_Address = (LPVOID)FindPattern(
				(DWORD64)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x41\x56\x48\x81\xEC\x00\x00\x00\x00\x33\xED",
				"xxxx?xxxx?xxxx?xxxxx????xx");

			if (MH_CreateHook(vfs_ropen_package_Address, &vfs_ropen_package_Hook, reinterpret_cast<LPVOID*>(&vfs_ropen_package_Orig)) == MH_OK) {
				if (MH_EnableHook(vfs_ropen_package_Address) != MH_OK) {
					MessageBox(NULL, "MH_EnableHook() != MH_OK", "vfs_ropen_package Hook", MB_OK | MB_ICONERROR);
				}
			} else {
				MessageBox(NULL, "MH_CreateHook() != MH_OK", "vfs_ropen_package Hook", MB_OK | MB_ICONERROR);
			}

			if (MH_CreateHook(vfs_rbuffered_package_Address, &vfs_rbuffered_package_Hook, reinterpret_cast<LPVOID*>(&vfs_rbuffered_package_Orig)) == MH_OK) {
				if (MH_EnableHook(vfs_rbuffered_package_Address) != MH_OK) {
					MessageBox(NULL, "MH_EnableHook() != MH_OK", "vfs_rbuffered_package Hook", MB_OK | MB_ICONERROR);
				}
			} else {
				MessageBox(NULL, "MH_CreateHook() != MH_OK", "vfs_rbuffered_package Hook", MB_OK | MB_ICONERROR);
			}

			/*if (MH_CreateHook(rblock_init_Address, &rblock_init_Hook, reinterpret_cast<LPVOID*>(&rblock_init_Orig)) == MH_OK) {
				if (MH_EnableHook(rblock_init_Address) != MH_OK) {
					MessageBox(NULL, "MH_EnableHook() != MH_OK", "rblock_init Hook", MB_OK | MB_ICONERROR);
				}
			} else {
				MessageBox(NULL, "MH_CreateHook() != MH_OK", "rblock_init Hook", MB_OK | MB_ICONERROR);
			}*/
#endif
		}

		isNavMapEnabled = (!isLL && strstr(GetCommandLine(), "-navmap"));

		if (isLogEnabled || isNavMapEnabled)
		{
			LPVOID slog_Address;

#ifndef _WIN64
			if (!isLL)
			{
				// B8 ? ? ? ? E8 ? ? ? ? 53 33 DB
				slog_Address = (LPVOID)FindPattern(
					(DWORD)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x53\x33\xDB",
					"x????x????xxx");
			}
			else
			{
				// B8 ? ? ? ? E8 ? ? ? ? 53 33 DB 56 33 C0
				slog_Address = (LPVOID)FindPattern(
					(DWORD)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x53\x33\xDB\x56\x33\xC0",
					"x????x????xxxxxx");
			}
#else
			// 40 53 B8 ? ? ? ? E8 ? ? ? ? 48 2B E0 33 C0
			slog_Address = (LPVOID)FindPattern(
				(DWORD64)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\x40\x53\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x2B\xE0\x33\xC0",
				"xxx????x????xxxxx");
#endif

			if (minhook) {
				if (MH_CreateHook(slog_Address, &slog_Hook, reinterpret_cast<LPVOID*>(&slog_Orig)) == MH_OK) {
					if (MH_EnableHook(slog_Address) != MH_OK) {
						MessageBox(NULL, "MH_EnableHook() != MH_OK", "slog Hook", MB_OK | MB_ICONERROR);
					}
				}
				else {
					MessageBox(NULL, "MH_CreateHook() != MH_OK", "slog Hook", MB_OK | MB_ICONERROR);
				}
			}
		}

		if (getBool("other", "badquit_reset", false))
		{
			BadQuitReset();
		}

#ifndef _WIN64
		if (!isLL && getBool("other", "no_videocard_msg", false)) // Only metro 2033
		{
			// 68 ? ? ? ? BF ? ? ? ? 8D 74 24 ? E8 ? ? ? ? 83 C4 ? 80 7C 24 ? ? 75 - для версии с dlc
			LPVOID VideoMsgAddress = (LPVOID)FindPattern(
				(DWORD)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\x68\x00\x00\x00\x00\xBF\x00\x00\x00\x00\x8D\x74\x24\x00\xE8\x00\x00\x00\x00\x83\xC4\x00\x80\x7C\x24\x00\x00\x75",
				"x????x????xxx?x????xx?xxx??x");
			BYTE VideoMsgJMP[] = { 0xEB, 0x5B };
			if (VideoMsgAddress == NULL)
			{
				// 68 ? ? ? ? BF ? ? ? ? 8D 74 24 ? E8 ? ? ? ? 83 C4 ? 80 7C 24 ? ? 74 ? 8D 7C 24 - для версии без dlc
				VideoMsgAddress = (LPVOID)FindPattern(
					(DWORD)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x68\x00\x00\x00\x00\xBF\x00\x00\x00\x00\x8D\x74\x24\x00\xE8\x00\x00\x00\x00\x83\xC4\x00\x80\x7C\x24\x00\x00\x74\x00\x8D\x7C\x24",
					"x????x????xxx?x????xx?xxx??x?xxx");
				VideoMsgJMP[1] = 0x78;
			}
			ASMWrite(VideoMsgAddress, VideoMsgJMP, sizeof(VideoMsgJMP));
		}
#endif

		if (isNavMapEnabled || getBool("other", "nointro", false))
		{
			LPVOID IntroAddress;
#ifndef _WIN64
			if (!isLL)
			{
				// 51 0F B7 05
				IntroAddress = (LPVOID)FindPattern(
					(DWORD)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x51\x0F\xB7\x05",
					"xxxx");
			}
			else
			{
				// 66 A1 ? ? ? ? 66 83 F8 06 73 15
				IntroAddress = (LPVOID)FindPattern(
					(DWORD)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x66\xA1\x00\x00\x00\x00\x66\x83\xF8\x06\x73\x15",
					"xx????xxxxxx");
			}
			BYTE ret[] = { 0xC3 };
			ASMWrite(IntroAddress, ret, sizeof(ret));
#else
			// 73 1D 33 C9
			IntroAddress = (LPVOID)FindPattern(
				(DWORD64)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\x73\x1D\x33\xC9",
				"xxxx");
			BYTE jmp[] = { 0xEB };
			ASMWrite(IntroAddress, jmp, sizeof(jmp));
#endif
		}

		bool unlock_dev_console = getBool("other", "unlock_dev_console", false);

		if (unlock_dev_console || isNavMapEnabled)
		{
#ifndef _WIN64
			if (!isLL)
			{
				// 55 8B EC 83 E4 ? A1 ? ? ? ? 85 C0 56 57 75 ? E8 ? ? ? ? 85 C0 74 ? 8B F8 E8 ? ? ? ? EB ? 33 C0 8B F0 A3 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 5F
				getConsole = (_getConsole)FindPattern(
					(DWORD)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x55\x8B\xEC\x83\xE4\x00\xA1\x00\x00\x00\x00\x85\xC0\x56\x57\x75\x00\xE8\x00\x00\x00\x00\x85\xC0\x74\x00\x8B\xF8\xE8\x00\x00\x00\x00\xEB\x00\x33\xC0\x8B\xF0\xA3\x00\x00\x00\x00\xE8\x00\x00\x00\x00\xA1\x00\x00\x00\x00\x5F",
					"xxxxx?x????xxxxx?x????xxx?xxx????x?xxxxx????x????x????x");
			}
			else
			{
				// 55 8B EC 83 E4 ? A1 ? ? ? ? 85 C0 75 ? E8 ? ? ? ? 8B C8 A3 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 8B E5
				getConsole = (_getConsole)FindPattern(
					(DWORD)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x55\x8B\xEC\x83\xE4\x00\xA1\x00\x00\x00\x00\x85\xC0\x75\x00\xE8\x00\x00\x00\x00\x8B\xC8\xA3\x00\x00\x00\x00\xE8\x00\x00\x00\x00\xA1\x00\x00\x00\x00\x8B\xE5",
					"xxxxx?x????xxx?x????xxx????x????x????xx");
			}
#else
			// читаем адрес инструкции mov ecx, [g_console]
			// 48 8B 0D ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 48 8B C8 E8 ? ? ? ? 48 8B 0D
			DWORD64 mov = FindPattern(
				(DWORD64)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\x48\x8B\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8B\xC8\xE8\x00\x00\x00\x00\x48\x8B\x0D",
				"xxx????x????x????xxxx????xxx");

			// вычисняем адрес g_console
			g_console = (void**)(mov + 7 + *(DWORD*)(mov + 3));
#endif
		}

		if(unlock_dev_console)
		{
#ifndef _WIN64
			if (!isLL)
			{
				// 56 8B F1 80 7E 48 00
				uconsole_server_impl_show = (_uconsole_server_impl_show)FindPattern(
					(DWORD)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x56\x8B\xF1\x80\x7E\x48\x00",
					"xxxxxxx");
			}
			else
			{
				// 55 8B EC 83 E4 F8 83 EC 0C 53 56 8B F1 33 DB
				uconsole_server_impl_show = (_uconsole_server_impl_show)FindPattern(
					(DWORD)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x55\x8B\xEC\x83\xE4\xF8\x83\xEC\x0C\x53\x56\x8B\xF1\x33\xDB",
					"xxxxxxxxxxxxxxx");
			}

			if (minhook) {
				// 51 ? 8B ? 8B 0D ? ? ? ? 85
				LPVOID clevel_r_on_key_press_Address = (LPVOID)FindPattern(
					(DWORD)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x51\x00\x8B\x00\x8B\x0D\x00\x00\x00\x00\x85",
					"x?x?xx????x");

				MH_STATUS status = MH_CreateHook(clevel_r_on_key_press_Address, 
					(isLL ? (LPVOID)&clevel_r_on_key_press_Hook : (LPVOID)&clevel_r_on_key_press_Hook2033),
					reinterpret_cast<LPVOID*>(&clevel_r_on_key_press_Orig));

				if (status == MH_OK) {
					if (MH_EnableHook(clevel_r_on_key_press_Address) != MH_OK) {
						MessageBox(NULL, "MH_EnableHook() != MH_OK", "clevel_r_on_key_press Hook", MB_OK | MB_ICONERROR);
					}
				} else {
					MessageBox(NULL, "MH_CreateHook() != MH_OK", "clevel_r_on_key_press Hook", MB_OK | MB_ICONERROR);
				}
			}
#else
			// 40 57 48 83 EC ? 80 79 ? ? 48 8B F9 0F 85 ? ? ? ? C6 41
			uconsole_server_impl_show = (_uconsole_server_impl_show)FindPattern(
				(DWORD64)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\x40\x57\x48\x83\xEC\x00\x80\x79\x00\x00\x48\x8B\xF9\x0F\x85\x00\x00\x00\x00\xC6\x41",
				"xxxxx?xx??xxxxx????xx");

			if (minhook) {
				// 40 53 55 56 57 48 83 EC ? 48 8B F1
				LPVOID clevel_r_on_key_press_Address = (LPVOID)FindPattern(
					(DWORD64)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x40\x53\x55\x56\x57\x48\x83\xEC\x00\x48\x8B\xF1",
					"xxxxxxxx?xxx");

				MH_STATUS status = MH_CreateHook(clevel_r_on_key_press_Address, (LPVOID)&clevel_r_on_key_press_Hook,
					reinterpret_cast<LPVOID*>(&clevel_r_on_key_press_Orig));

				if (status == MH_OK) {
					if (MH_EnableHook(clevel_r_on_key_press_Address) != MH_OK) {
						MessageBox(NULL, "MH_EnableHook() != MH_OK", "clevel_r_on_key_press Hook", MB_OK | MB_ICONERROR);
					}
				} else {
					MessageBox(NULL, "MH_CreateHook() != MH_OK", "clevel_r_on_key_press Hook", MB_OK | MB_ICONERROR);
				}
			}
#endif
		}

#ifndef _WIN64
		if (isNavMapEnabled)
		{
			AllocConsole();
			freopen("CONOUT$", "w", stdout);

			getString("nav_map", "format", "raw", navmapFormat, sizeof(logFilename));
			getString("nav_map", "filename", "nav_map.raw", navmapFilename, sizeof(logFilename));
			navmap_bin_mode = getBool("nav_map", "bin_mode", false);
			supply_debug_info_bin = getBool("nav_map", "supply_debug_info_bin", false);
			if (!navmap_bin_mode) {
				getString("nav_map", "result_pe", "nav_map.pe", navmapResult, sizeof(logFilename));
			} else {
				getString("nav_map", "result_bin", "nav_map.bin", navmapResult, sizeof(logFilename));
			}
			navmap_exit = getBool("nav_map", "exitwhendone", false);
		}
#endif
	}
	
	return TRUE;
}