#define _CRT_SECURE_NO_WARNINGS 1 // MSVS dno

#include <io.h>
#include <windows.h>
#include <vector>
#include "i_pathengine.h"
#include "MinHook.h"
#include "model.hpp"
#include "uconsole.h"

#define PSAPI_VERSION 1
#include <psapi.h>
#pragma comment (lib, "psapi.lib")

// settings
typedef char string256[256];

string256 logFilename;
bool isLogEnabled;

bool isNavMapEnabled;
bool isLL;

bool g_unlock_3rd_person_camera;
typedef void(__thiscall* _base_npc_cameras_cam_set)(void* _this, int camera_style, float speed, int preserve_attach);
typedef void(__cdecl* _set_camera_2033)(...); // this function has weird calling convention; 'this' passed in EDI, and two parameters passed through stack
_base_npc_cameras_cam_set base_npc_cameras_cam_set = nullptr;
_set_camera_2033 set_camera_2033 = nullptr;


enum enpc_cameras // 2033, Last Light, Redux (changed a bit in Arktika.1)
{
	enc_first_eye = 0,
	enc_ladder    = 1,
	enc_look_at   = 2,
	enc_free_look = 3,
	enc_station   = 4,
	enc_locked    = 5,
	enc_max_cam   = 6
};

bool g_fly;
typedef void(__fastcall*** _track)(void*);
#ifdef _WIN64
typedef _track(__fastcall* _cflycam_cflycam)(void* _this, const char* name);
typedef LPCRITICAL_SECTION(__fastcall* _memory)();
typedef void* (__fastcall* _tlsf_memalign)(DWORD64 tlsf, DWORD align, DWORD size);
typedef void* (__fastcall* _camera_manager_play_track)(DWORD64 _this, void* t, float accrue, float start_pos, void* unused3, void* e, void* unused4, void* unused5, void* owner);
_tlsf_memalign tlsf_memalign = nullptr;
DWORD64 g_game = NULL;
#else
typedef _track(__thiscall* _cflycam_cflycam)(void* _this, const char* name);
typedef LPCRITICAL_SECTION(__cdecl* _memory)();
typedef void* (__thiscall* _camera_manager_play_track)(DWORD _this, void* t, double hz, void* owner);
void* tlsf_memalign = nullptr;
DWORD g_game = NULL;
#endif
_cflycam_cflycam cflycam_cflycam = nullptr;
_memory memory = nullptr;
_camera_manager_play_track camera_manager_play_track = nullptr;

bool g_unlock_dev_console;
bool g_quicksave;

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

bool FileExists(const char *fn)
{
	DWORD attrs = GetFileAttributes(fn);
	return (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

typedef uconsole_server** (__stdcall* _getConsole)();
_getConsole getConsole = nullptr;

#ifndef _WIN64
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

//typedef iwriterObj* (__cdecl* _vfs__wopen_os)(iwriterObj* result, const char* _fname);
//_vfs__wopen_os vfs__wopen_os = nullptr;

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
	if (!navmap_bin_mode) // isLL
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

	// 8B 44 24 08 FF 05
	/*vfs__wopen_os = (_vfs__wopen_os)FindPattern(
		(DWORD)mi.lpBaseOfDll,
		mi.SizeOfImage,
		(BYTE*)"\x8B\x44\x24\x08\xFF\x05",
		"xxxxxx");

	iwriterObj wo;
	vfs__wopen_os(&wo, "nav_map.bin");

	printf("_object = %08X\n", wo._object);
	printf("__vftable = %08X\n", wo._object->__vftable);
	printf("iwriter_dtor_0 = %08X\n", wo._object->__vftable->iwriter_dtor_0);*/

	//real_mesh->saveGround(wo._object);
	//real_mesh->saveCollisionPreprocessFor(shape1, wo._object);
	//real_mesh->savePathfindPreprocessFor(shape1, wo._object);

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
		uconsole_server** console = (uconsole_server**)getConsole();
		(*console)->execute_deferred(console, "quit");
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

void* clevel_r_on_key_press_Orig = nullptr;

#ifndef _WIN64
void __fastcall clevel_r_on_key_press_Hook2033(void* _this, void* _unused, int action, int key, int state)
{
	//printf("action = %d, key = %d, state = %d\n", action, key, state);

	if (g_unlock_dev_console)
	{
		if (key == 41)
		{
			uconsole_server** console = getConsole();
			(*console)->show(console);
		}
	}
	
	// quick save on F5
	if (g_quicksave)
	{
		if (key == 63)
		{
			uconsole_server** console = getConsole();
			(*console)->execute_deferred(console, "gamesave auto_save");
		}
	}

	// fly on F7
	if (g_fly)
	{
		if (key == 65)
		{
			uconsole_server** console = getConsole();
			(*console)->execute_deferred(console, "fly 1");
		}
	}

	if (g_unlock_3rd_person_camera && key <= 61 && key >= 59)
	{
		// _this == g_level + 0x4 (+0x4 due to multiple inheritance)

		// which one is better to use here ??
		void* control_entity = *((void**)((char*)_this + 0x10));
		void* view_entity = *((void**)((char*)_this + 0x14));

		void *base_npc_cameras = *((void**)((char*)view_entity + 0x348));

		if (key == 59) // F1
		{
			__asm
			{
				push 3F800000h              // speed = 1.f
				push enc_first_eye          // camera_style = enc_first_eye
				mov edi, [base_npc_cameras] // 'this' pointer passed in EDI
				call [set_camera_2033]
			}
		}

		if (key == 60) // F2
		{
			__asm
			{
				push 3F800000h              // speed = 1.f
				push enc_look_at            // camera_style = enc_look_at
				mov edi, [base_npc_cameras] // 'this' pointer passed in EDI
				call [set_camera_2033]
			}
		}

		if (key == 61) // F3
		{
			__asm
			{
				push 3F800000h              // speed = 1.f
				push enc_free_look          // camera_style = enc_free_look
				mov edi, [base_npc_cameras] // 'this' pointer passed in EDI
				call [set_camera_2033]
			}
		}
	}

	typedef void(__thiscall* _clevel_r_on_key_press_2033)(void* _this, int action, int key, int state);
	((_clevel_r_on_key_press_2033)clevel_r_on_key_press_Orig)(_this, action, key, state);
}

void __fastcall clevel_r_on_key_press_Hook(void* _this, void* _unused, int action, int key, int state, int resending)
#else
void __fastcall clevel_r_on_key_press_Hook(void* _this, int action, int key, int state, int resending)
#endif
{
	//printf("action = %d, key = %d, state = %d, resending = %d\n", action, key, state, resending);

	if (g_unlock_dev_console)
	{
		if (key == 41)
		{
			uconsole_server** console = getConsole();
			(*console)->show(console);
		}
	}

	// quick save on F5
	if (g_quicksave)
	{
		if (key == 63)
		{
			uconsole_server** console = getConsole();
			(*console)->execute_deferred(console, "gamesave");
		}
	}

#ifdef _WIN64
	// Redux
	if (g_unlock_3rd_person_camera && key <= 61 && key >= 59)
	{
		// _this == g_level + 0x8 (+0x8 due to multiple inheritance)

		// which one is better to use here ??
		void* startup_entity = *((void**)((char*)_this + 0x28));
		void* control_entity = *((void**)((char*)_this + 0x30));
		void* view_entity = *((void**)((char*)_this + 0x38));

		void *base_npc_cameras = *((void**)((char*)view_entity + 0x640));

		if (key == 59) // F1
			base_npc_cameras_cam_set(base_npc_cameras, enc_first_eye, 1.f, 1);
		if (key == 60) // F2
			base_npc_cameras_cam_set(base_npc_cameras, enc_look_at, 1.f, 1);
		if (key == 61) // F3
			base_npc_cameras_cam_set(base_npc_cameras, enc_free_look, 1.f, 1);
	}

	// fly on F7
	if (g_fly)
	{
		if (key == 65)
		{
			LPCRITICAL_SECTION mem = memory();
			++*(DWORD*)((DWORD64)mem + 0x100);
			EnterCriticalSection(mem);
			DWORD64 tlsf = *(DWORD64*)((DWORD64)mem + 0x38);
			// ���� �� �����, � ������� ������������ 0x10. ������ ������ ������ ��, �������� �����-�� ��� � �������
			void* cflycam_this = tlsf_memalign(tlsf, 0x10, 0x120);
			LeaveCriticalSection(mem);

			_track track = cflycam_cflycam(cflycam_this, "1");

			if (g_game == NULL)
			{
				// ������ ����� ���������� mov rax, cs:g_game
				// 48 8B 05 ? ? ? ? 4C 8D 85 ? ? ? ? 48 8D 95 ? ? ? ? 48 8B 58 10 48 8D 4C 24 ? E8 ? ? ? ? 48 8B CB 48 89 7C 24 ? 0F 57 DB 0F 57 D2 48 8B D0 E8 ? ? ? ? 48 8D 8D ? ? ? ? E9 - ON STEAM
				DWORD64 mov = FindPattern(
					(DWORD64)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x48\x8B\x05\x00\x00\x00\x00\x4C\x8D\x85\x00\x00\x00\x00\x48\x8D\x95\x00\x00\x00\x00\x48\x8B\x58\x10\x48\x8D\x4C\x24\x00\xE8\x00\x00\x00\x00\x48\x8B\xCB\x48\x89\x7C\x24\x00\x0F\x57\xDB\x0F\x57\xD2\x48\x8B\xD0\xE8\x00\x00\x00\x00\x48\x8D\x8D\x00\x00\x00\x00\xE9",
					"xxx????xxx????xxx????xxxxxxxx?x????xxxxxxx?xxxxxxxxxx????xxx????x");

				if (mov == NULL) {
					// 48 8B 05 ? ? ? ? 4C 8D 85 ? ? ? ? 48 8D 95 ? ? ? ? 48 8D 4C 24 ? 48 8B 58 10 E8 ? ? ? ? 48 8B D0 48 89 7C 24 ? 0F 57 DB 0F 57 D2 48 8B CB E8 ? ? ? ? B8 ? ? ? ? E9 ? ? ? ? 48 8B 05 ? ? ? ? 48 85 C0 0F 84 ? ? ? ? 48 8B 88 - ON EGS
					mov = FindPattern(
						(DWORD64)mi.lpBaseOfDll,
						mi.SizeOfImage,
						(BYTE*)"\x48\x8B\x05\x00\x00\x00\x00\x4C\x8D\x85\x00\x00\x00\x00\x48\x8D\x95\x00\x00\x00\x00\x48\x8D\x4C\x24\x00\x48\x8B\x58\x10\xE8\x00\x00\x00\x00\x48\x8B\xD0\x48\x89\x7C\x24\x00\x0F\x57\xDB\x0F\x57\xD2\x48\x8B\xCB\xE8\x00\x00\x00\x00\xB8\x00\x00\x00\x00\xE9\x00\x00\x00\x00\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x0F\x84\x00\x00\x00\x00\x48\x8B\x88",
						"xxx????xxx????xxx????xxxx?xxxxx????xxxxxxx?xxxxxxxxxx????x????x????xxx????xxxxx????xxx");
				}

				// ��������� ����� � �������� g_game
				g_game = *(DWORD64*)(mov + 7 + *(DWORD*)(mov + 3));
			}
			DWORD64 _cameras = *(DWORD64*)(g_game + 0x10);

			// � ��� � �� ����� ��� ���� ���� �����������, �.�. �������� � ��� ��� ������, �� ����� �����...
			(**track)(track); // _constructor

			void* owner = nullptr;
			camera_manager_play_track(_cameras, track, 0.0, 0.0, nullptr, nullptr, nullptr, nullptr, &owner);
		}
	}
#else
	// Last Light
	if (g_unlock_3rd_person_camera && key <= 61 && key >= 59)
	{
		// _this == g_level + 0x4 (+0x4 due to multiple inheritance)

		// which one is better to use here ??
		void* startup_entity = *((void**)((char*)_this + 0x18));
		void* control_entity = *((void**)((char*)_this + 0x1C));
		void* view_entity = *((void**)((char*)_this + 0x20));

		void *base_npc_cameras = *((void**)((char*)view_entity + 0x3A4));

		if (key == 59) // F1
			base_npc_cameras_cam_set(base_npc_cameras, enc_first_eye, 1.f, 1);
		if (key == 60) // F2
			base_npc_cameras_cam_set(base_npc_cameras, enc_look_at, 1.f, 1);
		if (key == 61) // F3
			base_npc_cameras_cam_set(base_npc_cameras, enc_free_look, 1.f, 1);
	}

	// fly on F7
	if (g_fly)
	{
		if (key == 65)
		{
			LPCRITICAL_SECTION mem = memory();
			++*(DWORD*)((DWORD)mem + 0x84);
			EnterCriticalSection(mem);
			DWORD tlsf = *(DWORD*)((DWORD)mem + 0x20);
			// ���� �� �����, � ������� ������������ 0x10. ������ ������ ������ ��, �������� �����-�� ��� � �������
			void* cflycam_this = nullptr;

			__asm
			{
				mov esi, tlsf
				push 0x10
				push esi
				mov eax, 0x120
				call tlsf_memalign
				mov cflycam_this, eax
				add esp, 8
			}
			LeaveCriticalSection(mem);

			_track track = cflycam_cflycam(cflycam_this, "1");

			if (g_game == NULL)
			{
				// ������ ����� ���������� mov eax, g_game
				// A1 ? ? ? ? 0F 57 C0 56
				DWORD mov = FindPattern(
					(DWORD)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\xA1\x00\x00\x00\x00\x0F\x57\xC0\x56",
					"x????xxxx");

				// ��������� ����� � �������� g_game
				g_game = *(DWORD*)(*(DWORD*)(mov + 1));
			}
			DWORD _cameras = *(DWORD*)(g_game + 0x8);
			
			// � ��� � �� ����� ��� ���� ���� �����������, �.�. �������� � ��� ��� ������, �� ����� �����...
			(**track)(track); // _constructor
			
			void* owner = nullptr;
			camera_manager_play_track(_cameras, track, 0, &owner);
		}
	}
#endif

	typedef void(__thiscall* _clevel_r_on_key_press)(void* _this, int action, int key, int state, int resending);
	((_clevel_r_on_key_press)clevel_r_on_key_press_Orig)(_this, action, key, state, resending);
}

typedef void (__stdcall* _cmd_register_commands)();
_cmd_register_commands cmd_register_commands_Orig = nullptr;

void* cmd_mask_Address = nullptr;
unsigned int* ps_actor_flags_Address = nullptr;
void* cmd_mask_vftable_Address = nullptr;

cmd_mask_struct g_god;
cmd_mask_struct g_global_god;
cmd_mask_struct g_kill_everyone;
cmd_mask_struct g_notarget;
cmd_mask_struct g_unlimitedammo;
cmd_mask_struct g_unlimitedfilters;
cmd_mask_struct g_autopickup;

#ifndef _WIN64
void cmd_register_commands_Hook()
{
	uconsole cu = uconsole::uconsole(getConsole(), cmd_mask_Address);

	cu.cmd_mask(&g_god, "g_god", ps_actor_flags_Address, 1, false);
	cu.command_add(&g_god);

	cu.cmd_mask(&g_global_god, "g_global_god", ps_actor_flags_Address, 2, false);
	cu.command_add(&g_global_god);

	cu.cmd_mask(&g_kill_everyone, "g_kill_everyone", ps_actor_flags_Address, 16, false);
	cu.command_add(&g_kill_everyone);

	cu.cmd_mask(&g_notarget, "g_notarget", ps_actor_flags_Address, 4, false);
	cu.command_add(&g_notarget);

	cu.cmd_mask(&g_unlimitedammo, "g_unlimitedammo", ps_actor_flags_Address, 8, false);
	cu.command_add(&g_unlimitedammo);

	cu.cmd_mask(&g_unlimitedfilters, "g_unlimitedfilters", ps_actor_flags_Address, 128, false);
	cu.command_add(&g_unlimitedfilters);

	cu.cmd_mask(&g_autopickup, "g_autopickup", ps_actor_flags_Address, 32, false);
	cu.command_add(&g_autopickup);

	cmd_register_commands_Orig();
}
#else
void cmd_register_commands_Hook()
{
	// 0. call original function first to ensure that g_toggle_aim is initialized and we can find it
	cmd_register_commands_Orig();
	
	// 1. find constant string
	const char *str_g_toggle_aim = (const char *)FindPattern(
		(DWORD64)mi.lpBaseOfDll,
		mi.SizeOfImage,
		(BYTE*)"g_toggle_aim\0",
		"xxxxxxxxxxxxx");
		
	// 2. find reference to that string
	const char **xref = (const char **)FindPattern(
		(DWORD64)mi.lpBaseOfDll,
		mi.SizeOfImage,
		(BYTE*)&str_g_toggle_aim,
		"xxxxxxxx");
	
	// 3. find pointer to existing command object based on xref
	cmd_mask_struct * pCmd = (cmd_mask_struct*)(((char*)xref) - offsetof(cmd_mask_struct, _name));
	
	ps_actor_flags_Address = pCmd->value;
	cmd_mask_vftable_Address = pCmd->__vftable;
	
	// 4. register new commands
	uconsole cu = uconsole::uconsole(getConsole(), NULL);
	
	g_god.construct(cmd_mask_vftable_Address, "g_god", ps_actor_flags_Address, 1);
	cu.command_add(&g_god);
	
	g_global_god.construct(cmd_mask_vftable_Address, "g_global_god", ps_actor_flags_Address, 2);
	cu.command_add(&g_global_god);

	g_kill_everyone.construct(cmd_mask_vftable_Address, "g_kill_everyone", ps_actor_flags_Address, 16);
	cu.command_add(&g_kill_everyone);

	g_notarget.construct(cmd_mask_vftable_Address, "g_notarget", ps_actor_flags_Address, 4);
	cu.command_add(&g_notarget);

	g_unlimitedammo.construct(cmd_mask_vftable_Address, "g_unlimitedammo", ps_actor_flags_Address, 8);
	cu.command_add(&g_unlimitedammo);

	g_unlimitedfilters.construct(cmd_mask_vftable_Address, "g_unlimitedfilters", ps_actor_flags_Address, 128);
	cu.command_add(&g_unlimitedfilters);

	g_autopickup.construct(cmd_mask_vftable_Address, "g_autopickup", ps_actor_flags_Address, 32);
	cu.command_add(&g_autopickup);
}
#endif

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
typedef void(__fastcall* _vfs_rbuffered_packageEGS)(const char* fn, void* cb, const int force_raw);
typedef void* (__fastcall* _rblock_init)(const char* fn, unsigned int* f_offset, unsigned int* f_size, unsigned int not_packaged);
typedef bool (__fastcall* _vfs_package_registry_level_downloaded)(void *_this, const char *map_name);

_vfs_ropen_package vfs_ropen_package_Orig = nullptr;
_vfs_ropen_os vfs_ropen_os = nullptr;
_vfs_rbuffered_package vfs_rbuffered_package_Orig = nullptr;
_vfs_rbuffered_packageEGS vfs_rbuffered_package_OrigEGS = nullptr;
_rblock_init rblock_init_Orig = nullptr;
_vfs_package_registry_level_downloaded vfs_package_registry_level_downloaded_Orig = nullptr;

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

// ON STEAM 
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

// ON EGS
void __fastcall vfs_rbuffered_package_HookEGS(const char* fn, fastdelegate* cb, const int force_raw)
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

	vfs_rbuffered_package_OrigEGS(fn, cb, force_raw);
}

void* __fastcall rblock_init_Hook(const char* fn, unsigned int* f_offset, unsigned int* f_size, unsigned int not_packaged)
{
	printf("%s\n", fn);

	if (GetFileAttributes(fn) != 0xFFFFFFFF)
	{
		//return rblock_init_Orig(fn, f_offset, f_size, 1);
		
		// 0F 84 ? ? ? ? 48 8B D1 48 8D 4C 24
		// \x0F\x84\x00\x00\x00\x00\x48\x8B\xD1\x48\x8D\x4C\x24 xx????xxxxxxx
		void* je = (void*)FindPattern(
			(DWORD64)mi.lpBaseOfDll,
			mi.SizeOfImage,
			(BYTE*)"\x0F\x84\x00\x00\x00\x00\x48\x8B\xD1\x48\x8D\x4C\x24",
			"xx????xxxxxxx");
		BYTE nop[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
		ASMWrite(je, nop, sizeof(nop));

		//BYTE nop1[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
		// 0f 84 ? ? ? ? 48 89 bc 24 ? ? ? ? e8
		//ASMWrite((void*)je, nop1, sizeof(nop1));

		void* ttt = rblock_init_Orig(fn, f_offset, f_size, 1);
		if (ttt == 0)
		{
			printf("test\n");
		}
		return ttt;
	}

	return rblock_init_Orig(fn, f_offset, f_size, not_packaged);
}

bool __fastcall vfs_package_registry_level_downloaded_Hook(void *_this, const char *map_name)
{
	bool ret = vfs_package_registry_level_downloaded_Orig(_this, map_name);
	
	if(!ret)
	{
		char map_path[256];
		strcpy(map_path, "content\\maps\\");
		strcat(map_path, map_name);
		strcat(map_path, "\\level");
		if(FileExists(map_path))
			ret = true;
	}
	
	return ret;
}
#endif

BOOL APIENTRY DllMain(HINSTANCE hInstDLL, DWORD reason, LPVOID reserved)
{
	if(reason == DLL_PROCESS_ATTACH)
	{
		AllocConsole();
		freopen("CONOUT$", "w", stdout);

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
			// 75 ? 49 8B 45 ? 48 8D 50 ? 48 85 C0 75 ? 48 8B D6 48 8D 0D - ON STEAM
			LPVOID jne = (LPVOID)FindPattern(
				(DWORD64)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\x75\x00\x49\x8B\x45\x00\x48\x8D\x50\x00\x48\x85\xC0\x75\x00\x48\x8B\xD6\x48\x8D\x0D",
				"x?xxx?xxx?xxxx?xxxxxx");

			if (jne == NULL) {
				// 75 6D 48 8B 06 48 8D 0D ? ? ? ? 33 F6 48 85 C0 48 8D 50 14 48 0F 44 D6 E8 ? ? ? ? 49 8B 1C 24 4C 8D 05 ? ? ? ? 48 8D 95 - ON EGS
				jne = (LPVOID)FindPattern(
					(DWORD64)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x75\x6D\x48\x8B\x06\x48\x8D\x0D\x00\x00\x00\x00\x33\xF6\x48\x85\xC0\x48\x8D\x50\x14\x48\x0F\x44\xD6\xE8\x00\x00\x00\x00\x49\x8B\x1C\x24\x4C\x8D\x05\x00\x00\x00\x00\x48\x8D\x95",
					"xxxxxxxx????xxxxxxxxxxxxxx????xxxxxxx????xxx");
			}
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
			// 48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 54 41 56 41 57 48 83 EC ? 45 33 E4 45 8B F9 - ON STEAM
			LPVOID vfs_ropen_package_Address = (LPVOID)FindPattern(
				(DWORD64)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x56\x57\x41\x54\x41\x56\x41\x57\x48\x83\xEC\x00\x45\x33\xE4\x45\x8B\xF9",
				"xxxx?xxxx?xxxxxxxxxxx?xxxxxx");

			if (vfs_ropen_package_Address == NULL) {
				// 48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 54 41 56 41 57 48 83 EC 30 45 33 E4 41 8B E9 49 8B F0 48 8B DA 4C 8B F1 44 39 62 0C 0F 84 ? ? ? ? 4D 8B C8 4C 89 64 24 ? 4C 8B 02 41 BF ? ? ? ? 33 D2 - ON EGS
				vfs_ropen_package_Address = (LPVOID)FindPattern(
					(DWORD64)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x56\x57\x41\x54\x41\x56\x41\x57\x48\x83\xEC\x30\x45\x33\xE4\x41\x8B\xE9\x49\x8B\xF0\x48\x8B\xDA\x4C\x8B\xF1\x44\x39\x62\x0C\x0F\x84\x00\x00\x00\x00\x4D\x8B\xC8\x4C\x89\x64\x24\x00\x4C\x8B\x02\x41\xBF\x00\x00\x00\x00\x33\xD2",
					"xxxx?xxxx?xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx????xxxxxxx?xxxxx????xx");
			}

			// 48 8B C4 53 55 57 41 56 41 57 48 81 EC - ON STEAM
			vfs_ropen_os = (_vfs_ropen_os)FindPattern(
				(DWORD64)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\x48\x8B\xC4\x53\x55\x57\x41\x56\x41\x57\x48\x81\xEC",
				"xxxxxxxxxxxxx");

			if (vfs_ropen_os == NULL) {
				// 48 8B C4 48 89 58 10 55 56 57 41 54 41 57 48 81 EC ? ? ? ? 33 DB 4C 8B F9 48 8D 4C 24 ? 48 89 58 20 48 89 58 18 8B FB 8B F3 E8 - ON EGS
				vfs_ropen_os = (_vfs_ropen_os)FindPattern(
					(DWORD64)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x48\x8B\xC4\x48\x89\x58\x10\x55\x56\x57\x41\x54\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x33\xDB\x4C\x8B\xF9\x48\x8D\x4C\x24\x00\x48\x89\x58\x20\x48\x89\x58\x18\x8B\xFB\x8B\xF3\xE8",
					"xxxxxxxxxxxxxxxxx????xxxxxxxxx?xxxxxxxxxxxxx");
			}

			// 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 41 56 48 83 EC 40 48 8B 1D ? ? ? ? 48 8B F1 48 8B 6A 08 4C 8B 32 48 85 DB 75 0F E8 ? ? ? ? 48 8B D8 48 89 05 - ON EGS
			LPVOID vfs_rbuffered_package_Address = (LPVOID)FindPattern(
				(DWORD64)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x41\x56\x48\x83\xEC\x40\x48\x8B\x1D\x00\x00\x00\x00\x48\x8B\xF1\x48\x8B\x6A\x08\x4C\x8B\x32\x48\x85\xDB\x75\x0F\xE8\x00\x00\x00\x00\x48\x8B\xD8\x48\x89\x05",
				"xxxx?xxxx?xxxx?xxxxxxxxx");

			bool onEGS = true;

			if (vfs_rbuffered_package_Address == NULL) {
				// 48 89 5C 24 ? 48 89 74 24 ? 41 56 48 83 EC ? 83 79 - ON STEAM
				LPVOID vfs_rbuffered_package_Address = (LPVOID)FindPattern(
					(DWORD64)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x41\x56\x48\x83\xEC\x00\x83\x79",
					"xxxx?xxxx?xxxxx?xx");

				onEGS = false;
			}

			// 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 41 56 48 81 EC ? ? ? ? 33 ED
			/*LPVOID rblock_init_Address = (LPVOID)FindPattern(
				(DWORD64)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x41\x56\x48\x81\xEC\x00\x00\x00\x00\x33\xED",
				"xxxx?xxxx?xxxx?xxxxx????xx");*/
				
			// 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 54 41 56 41 57 48 83 EC 20 44 0F B7 B9 ? ? ? ? 33 DB 4C 8B F2 4C 8B E1 45 85 FF 74 6E - ONLY ON STEAM
			LPVOID vfs_package_registry_level_downloaded_Address = (LPVOID)FindPattern(
				(DWORD64)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x41\x54\x41\x56\x41\x57\x48\x83\xEC\x20\x44\x0F\xB7\xB9\x00\x00\x00\x00\x33\xDB\x4C\x8B\xF2\x4C\x8B\xE1\x45\x85\xFF\x74\x6E",
				"xxxx?xxxx?xxxx?xxxx?xxxxxxxxxxxxxx????xxxxxxxxxxxxx");

			if (MH_CreateHook(vfs_ropen_package_Address, &vfs_ropen_package_Hook, reinterpret_cast<LPVOID*>(&vfs_ropen_package_Orig)) == MH_OK) {
				if (MH_EnableHook(vfs_ropen_package_Address) != MH_OK) {
					MessageBox(NULL, "MH_EnableHook() != MH_OK", "vfs_ropen_package Hook", MB_OK | MB_ICONERROR);
				}
			} else {
				MessageBox(NULL, "MH_CreateHook() != MH_OK", "vfs_ropen_package Hook", MB_OK | MB_ICONERROR);
			}

			if (!onEGS) {
				if (MH_CreateHook(vfs_rbuffered_package_Address, &vfs_rbuffered_package_Hook, reinterpret_cast<LPVOID*>(&vfs_rbuffered_package_Orig)) == MH_OK) {
					if (MH_EnableHook(vfs_rbuffered_package_Address) != MH_OK) {
						MessageBox(NULL, "MH_EnableHook() != MH_OK", "vfs_rbuffered_package Hook", MB_OK | MB_ICONERROR);
					}
				} else {
					MessageBox(NULL, "MH_CreateHook() != MH_OK", "vfs_rbuffered_package Hook", MB_OK | MB_ICONERROR);
				}
			} else {
				if (MH_CreateHook(vfs_rbuffered_package_Address, &vfs_rbuffered_package_HookEGS, reinterpret_cast<LPVOID*>(&vfs_rbuffered_package_OrigEGS)) == MH_OK) {
					if (MH_EnableHook(vfs_rbuffered_package_Address) != MH_OK) {
						MessageBox(NULL, "MH_EnableHook() != MH_OK", "vfs_rbuffered_package Hook", MB_OK | MB_ICONERROR);
					}
				} else {
					MessageBox(NULL, "MH_CreateHook() != MH_OK", "vfs_rbuffered_package Hook", MB_OK | MB_ICONERROR);
				}
			}

			/*if (MH_CreateHook(rblock_init_Address, &rblock_init_Hook, reinterpret_cast<LPVOID*>(&rblock_init_Orig)) == MH_OK) {
				if (MH_EnableHook(rblock_init_Address) != MH_OK) {
					MessageBox(NULL, "MH_EnableHook() != MH_OK", "rblock_init Hook", MB_OK | MB_ICONERROR);
				}
			} else {
				MessageBox(NULL, "MH_CreateHook() != MH_OK", "rblock_init Hook", MB_OK | MB_ICONERROR);
			}*/
			
			if (vfs_package_registry_level_downloaded_Address != NULL) {
				if (MH_CreateHook(vfs_package_registry_level_downloaded_Address, &vfs_package_registry_level_downloaded_Hook, reinterpret_cast<LPVOID*>(&vfs_package_registry_level_downloaded_Orig)) == MH_OK) {
					if (MH_EnableHook(vfs_package_registry_level_downloaded_Address) != MH_OK) {
						MessageBox(NULL, "MH_EnableHook() != MH_OK", "vfs_package_registry_level_downloaded Hook", MB_OK | MB_ICONERROR);
					}
				} else {
					MessageBox(NULL, "MH_CreateHook() != MH_OK", "vfs_package_registry_level_downloaded Hook", MB_OK | MB_ICONERROR);
				}
			}
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
			// 40 53 B8 ? ? ? ? E8 ? ? ? ? 48 2B E0 33 C0 - ON STEAM
			slog_Address = (LPVOID)FindPattern(
				(DWORD64)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\x40\x53\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x2B\xE0\x33\xC0",
				"xxx????x????xxxxx");

			if (slog_Address == NULL) {
				// 48 89 5C 24 ? 48 89 6C 24 ? 56 57 41 56 B8 ? ? ? ? E8 - ON EGS
				slog_Address = (LPVOID)FindPattern(
					(DWORD64)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x56\x57\x41\x56\xB8\x00\x00\x00\x00\xE8",
					"xxxx?xxxx?xxxxx????x");
			}
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
			// 68 ? ? ? ? BF ? ? ? ? 8D 74 24 ? E8 ? ? ? ? 83 C4 ? 80 7C 24 ? ? 75 - ��� ������ � dlc
			LPVOID VideoMsgAddress = (LPVOID)FindPattern(
				(DWORD)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\x68\x00\x00\x00\x00\xBF\x00\x00\x00\x00\x8D\x74\x24\x00\xE8\x00\x00\x00\x00\x83\xC4\x00\x80\x7C\x24\x00\x00\x75",
				"x????x????xxx?x????xx?xxx??x");
			BYTE VideoMsgJMP[] = { 0xEB, 0x5B };
			if (VideoMsgAddress == NULL)
			{
				// 68 ? ? ? ? BF ? ? ? ? 8D 74 24 ? E8 ? ? ? ? 83 C4 ? 80 7C 24 ? ? 74 ? 8D 7C 24 - ��� ������ ��� dlc
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
			// 73 1D 33 C9 - ON STEAM
			IntroAddress = (LPVOID)FindPattern(
				(DWORD64)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\x73\x1D\x33\xC9",
				"xxxx");

			if (IntroAddress == NULL) {
				// 73 15 0F B7 C0 48 8D 8D - ON EGS
				IntroAddress = (LPVOID)FindPattern(
					(DWORD64)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x73\x15\x0F\xB7\xC0\x48\x8D\x8D",
					"xxxxxxxx");
			}

			BYTE jmp[] = { 0xEB };
			ASMWrite(IntroAddress, jmp, sizeof(jmp));
#endif
		}

		g_unlock_3rd_person_camera = getBool("other", "unlock_3rd_person_camera", false);
		if (g_unlock_3rd_person_camera)
		{
#ifdef _WIN64
			// 48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 56 48 83 EC 30 48 63 41 68 48 8B F1 0F 29 74 24 ? 4C 8B 74 C1 ? 89 51 68 48 63 C2 0F 28 F2 48 8B 5C C1 ? 49 8B 06 49 8B CE 41 8B F9 FF 50 28 48 8B 03 44 8B C7 49 8B D6 48 8B CB FF 50 20 8B 05 ? ? ? ? A8 01 75 20 - ON STEAM
			base_npc_cameras_cam_set = (_base_npc_cameras_cam_set)FindPattern(
				(DWORD64)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x41\x56\x48\x83\xEC\x30\x48\x63\x41\x68\x48\x8B\xF1\x0F\x29\x74\x24\x00\x4C\x8B\x74\xC1\x00\x89\x51\x68\x48\x63\xC2\x0F\x28\xF2\x48\x8B\x5C\xC1\x00\x49\x8B\x06\x49\x8B\xCE\x41\x8B\xF9\xFF\x50\x28\x48\x8B\x03\x44\x8B\xC7\x49\x8B\xD6\x48\x8B\xCB\xFF\x50\x20\x8B\x05\x00\x00\x00\x00\xA8\x01\x75\x20",
				"xxxx?xxxx?xxxx?xxxxxxxxxxxxxxxxx?xxxx?xxxxxxxxxxxxx?xxxxxxxxxxxxxxxxxxxxxxxxxxxxx????xxxx");

			if (base_npc_cameras_cam_set == NULL) {
				// 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 30 48 63 41 68 48 8B E9 0F 29 74 24 ? 41 8B F9 0F 28 F2 48 8B 74 C1 ? 89 51 68 48 63 C2 48 8B 5C C1 ? 48 8B CE 48 8B 06 FF 50 28 48 8B 03 44 8B C7 48 8B D6 48 8B CB FF 50 20 E8 ? ? ? ? 48 8B D8 BF ? ? ? ? E8 - ON EGS
				base_npc_cameras_cam_set = (_base_npc_cameras_cam_set)FindPattern(
					(DWORD64)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x30\x48\x63\x41\x68\x48\x8B\xE9\x0F\x29\x74\x24\x00\x41\x8B\xF9\x0F\x28\xF2\x48\x8B\x74\xC1\x00\x89\x51\x68\x48\x63\xC2\x48\x8B\x5C\xC1\x00\x48\x8B\xCE\x48\x8B\x06\xFF\x50\x28\x48\x8B\x03\x44\x8B\xC7\x48\x8B\xD6\x48\x8B\xCB\xFF\x50\x20\xE8\x00\x00\x00\x00\x48\x8B\xD8\xBF\x00\x00\x00\x00\xE8",
					"xxxx?xxxx?xxxx?xxxxxxxxxxxxxxxx?xxxxxxxxxx?xxxxxxxxxx?xxxxxxxxxxxxxxxxxxxxxxxxx????xxxx????x");
			}
#else
			if (isLL)
			{
				base_npc_cameras_cam_set = (_base_npc_cameras_cam_set)FindPattern(
					(DWORD64)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x53\x56\x8B\xF1\x8B\x46\x4C\x57\x8B\x7C\x86\x34\x8B\x44\x24\x10\x89\x46\x4C\x8B\x17\x8B\x5C\x86\x34\x8B\x42\x14\x8B\xCF",
					"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
			}
			else
			{
				set_camera_2033 = (_set_camera_2033)FindPattern(
					(DWORD64)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\xB8\x00\x00\x00\x00\x84\x05\x00\x00\x00\x00\x56\x75\x1D\x09\x05\x00\x00\x00\x00\x68\x00\x00\x00\x00\xC7\x05\x00\x00\x00\x00\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x83\xC4\x04\x8B\x47\x6C",
					"x????xx????xxxxx????x????xx????????x????xxxxxx");
			}
#endif
		}

		g_unlock_dev_console = getBool("other", "unlock_dev_console", false);
		g_quicksave = getBool("other", "quicksave", false);

#ifdef _WIN64
		bool restore_deleted_commands = getBool("other", "restore_deleted_commands", false);
#else
		bool restore_deleted_commands = getBool("other", "restore_deleted_commands", false) && isLL;
#endif

		if (g_unlock_dev_console || restore_deleted_commands || isNavMapEnabled)
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
			// 48 83 ec ? 48 8b 05 ? ? ? ? 48 85 c0 75 ? e8 ? ? ? ? 48 8b 05 - ON STEAM
			getConsole = (_getConsole)FindPattern(
				(DWORD64)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\x48\x83\xec\x00\x48\x8b\x05\x00\x00\x00\x00\x48\x85\xc0\x75\x00\xe8\x00\x00\x00\x00\x48\x8b\x05",
				"xxx?xxx????xxxx?x????xxx");

			if (getConsole == NULL) {
				// 48 83 EC 38 48 8B 05 ? ? ? ? 48 85 C0 0F 85 ? ? ? ? 48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 - ON EGS
				getConsole = (_getConsole)FindPattern(
					(DWORD64)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x48\x83\xEC\x38\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x0F\x85\x00\x00\x00\x00\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24",
					"xxxxxxx????xxxxx????xxxx?xxxx?xxxx");
			}
#endif
		}

		g_fly = getBool("other", "fly", false);

		if(g_unlock_dev_console || g_unlock_3rd_person_camera || g_quicksave || g_fly)
		{
#ifndef _WIN64
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
			if (minhook) {
				// 40 53 55 56 57 48 83 EC ? 48 8B F1 - ON STEAM
				LPVOID clevel_r_on_key_press_Address = (LPVOID)FindPattern(
					(DWORD64)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x40\x53\x55\x56\x57\x48\x83\xEC\x00\x48\x8B\xF1",
					"xxxxxxxx?xxx");

				if (clevel_r_on_key_press_Address == NULL) {
					// 40 55 56 57 41 57 48 83 EC 58 - ON EGS
					clevel_r_on_key_press_Address = (LPVOID)FindPattern(
						(DWORD64)mi.lpBaseOfDll,
						mi.SizeOfImage,
						(BYTE*)"\x40\x55\x56\x57\x41\x57\x48\x83\xEC\x58",
						"xxxxxxxxxx");
				}

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

		if (restore_deleted_commands)
		{
#ifndef _WIN64
			// 8A 54 24 10 8B C1
			cmd_mask_Address = (LPVOID)FindPattern(
				(DWORD)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\x8A\x54\x24\x10\x8B\xC1",
				"xxxxxx");

			// c7 05 ? ? ? ? ? ? ? ? 89 1d ? ? ? ? 89 1d ? ? ? ? 89 1d ? ? ? ? e8 ? ? ? ? 83 c4 ? e8
			ps_actor_flags_Address = (unsigned int*) *(DWORD*)(FindPattern(
				(DWORD)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\xc7\x05\x00\x00\x00\x00\x00\x00\x00\x00\x89\x1d\x00\x00\x00\x00\x89\x1d\x00\x00\x00\x00\x89\x1d\x00\x00\x00\x00\xe8\x00\x00\x00\x00\x83\xc4\x00\xe8",
				"xx????????xx????xx????xx????x????xx?x") + 6);

			if (minhook) {
				// B8 ? ? ? ? 53 BB
				LPVOID cmd_register_commands_Address = (LPVOID)FindPattern(
					(DWORD)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\xB8\x00\x00\x00\x00\x53\xBB",
					"x????xx");

				MH_STATUS status = MH_CreateHook(cmd_register_commands_Address, (LPVOID)&cmd_register_commands_Hook,
					reinterpret_cast<LPVOID*>(&cmd_register_commands_Orig));

				if (status == MH_OK) {
					if (MH_EnableHook(cmd_register_commands_Address) != MH_OK) {
						MessageBox(NULL, "MH_EnableHook() != MH_OK", "cmd_register_commands Hook", MB_OK | MB_ICONERROR);
					}
				}
				else {
					MessageBox(NULL, "MH_CreateHook() != MH_OK", "cmd_register_commands Hook", MB_OK | MB_ICONERROR);
				}
			}
#else
			if (minhook) {
				// 48 89 5C 24 ? 57 48 83 EC 20 8B 05 ? ? ? ? 48 8D 1D ? ? ? ? 48 8D 3D ? ? ? ? A8 01 75 60 - ON STEAM
				LPVOID cmd_register_commands_Address = (LPVOID)FindPattern(
					(DWORD64)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x48\x89\x5C\x24\x00\x57\x48\x83\xEC\x20\x8B\x05\x00\x00\x00\x00\x48\x8D\x1D\x00\x00\x00\x00\x48\x8D\x3D\x00\x00\x00\x00\xA8\x01\x75\x60",
					"xxxx?xxxxxxx????xxx????xxx????xxxx");

				if (cmd_register_commands_Address == NULL) {
					// 40 53 48 83 EC 20 65 48 8B 04 25 ? ? ? ? 8B 0D ? ? ? ? BA ? ? ? ? 48 8B 1C C8 48 03 DA 8B 03 39 05 ? ? ? ? 0F 8F ? ? ? ? E8 ? ? ? ? 48 8D 15 ? ? ? ? 48 8B C8 4C 8B 00 41 FF 50 18 8B 03 39 05 ? ? ? ? 0F 8F - ON EGS
					cmd_register_commands_Address = (LPVOID)FindPattern(
						(DWORD64)mi.lpBaseOfDll,
						mi.SizeOfImage,
						(BYTE*)"\x40\x53\x48\x83\xEC\x20\x65\x48\x8B\x04\x25\x00\x00\x00\x00\x8B\x0D\x00\x00\x00\x00\xBA\x00\x00\x00\x00\x48\x8B\x1C\xC8\x48\x03\xDA\x8B\x03\x39\x05\x00\x00\x00\x00\x0F\x8F\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00\x48\x8B\xC8\x4C\x8B\x00\x41\xFF\x50\x18\x8B\x03\x39\x05\x00\x00\x00\x00\x0F\x8F",
						"xxxxxxxxxxx????xx????x????xxxxxxxxxxx????xx????x????xxx????xxxxxxxxxxxxxx????xx");
				}

				MH_STATUS status = MH_CreateHook(cmd_register_commands_Address, (LPVOID)&cmd_register_commands_Hook,
					reinterpret_cast<LPVOID*>(&cmd_register_commands_Orig));

				if (status == MH_OK) {
					if (MH_EnableHook(cmd_register_commands_Address) != MH_OK) {
						MessageBox(NULL, "MH_EnableHook() != MH_OK", "cmd_register_commands Hook", MB_OK | MB_ICONERROR);
					}
				}
				else {
					MessageBox(NULL, "MH_CreateHook() != MH_OK", "cmd_register_commands Hook", MB_OK | MB_ICONERROR);
				}
			}
#endif
		}

		if (g_fly)
		{
#ifdef _WIN64
			// 48 8B C4 48 89 58 10 48 89 68 18 56 57 41 54 41 56 41 57 48 81 EC ? ? ? ? 0F 29 70 C8 0F 29 78 B8 44 0F 29 40 - ON STEAM
			cflycam_cflycam = (_cflycam_cflycam)FindPattern(
				(DWORD64)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\x48\x8B\xC4\x48\x89\x58\x10\x48\x89\x68\x18\x56\x57\x41\x54\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x0F\x29\x70\xC8\x0F\x29\x78\xB8\x44\x0F\x29\x40",
				"xxxxxxxxxxxxxxxxxxxxxx????xxxxxxxxxxxx");

			if (cflycam_cflycam == NULL) {
				// 48 8B C4 48 89 58 10 48 89 68 18 48 89 70 20 57 41 56 41 57 48 81 EC ? ? ? ? 0F 29 70 D8 48 8D B1 - ON EGS
				cflycam_cflycam = (_cflycam_cflycam)FindPattern(
					(DWORD64)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x48\x8B\xC4\x48\x89\x58\x10\x48\x89\x68\x18\x48\x89\x70\x20\x57\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x0F\x29\x70\xD8\x48\x8D\xB1",
					"xxxxxxxxxxxxxxxxxxxxxxx????xxxxxxx");
			}

			// 48 83 EC 28 48 8B 05 ? ? ? ? 48 85 C0 75 7F 8B 05 ? ? ? ? 48 89 5C 24 ? A8 01 75 1A 83 C8 01 89 05 ? ? ? ? E8 ? ? ? ? 48 8D 0D ? ? ? ? E8 - ON STEAM
			memory = (_memory)FindPattern(
				(DWORD64)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\x48\x83\xEC\x28\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x75\x7F\x8B\x05\x00\x00\x00\x00\x48\x89\x5C\x24\x00\xA8\x01\x75\x1A\x83\xC8\x01\x89\x05\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\xE8",
				"xxxxxxx????xxxxxxx????xxxx?xxxxxxxxx????x????xxx????x");

			if (memory == NULL) {
				// 48 83 EC 28 48 8B 05 ? ? ? ? 48 85 C0 0F 85 ? ? ? ? 65 48 8B 04 25 ? ? ? ? 48 89 5C 24 ? 48 89 6C 24 ? 48 8D 2D ? ? ? ? 48 89 74 24 ? 33 F6 48 8B 08 BA ? ? ? ? 48 89 7C 24 ? 8B 04 0A 39 05 ? ? ? ? 0F 8F - ON EGS
				memory = (_memory)FindPattern(
					(DWORD64)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x48\x83\xEC\x28\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x0F\x85\x00\x00\x00\x00\x65\x48\x8B\x04\x25\x00\x00\x00\x00\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x8D\x2D\x00\x00\x00\x00\x48\x89\x74\x24\x00\x33\xF6\x48\x8B\x08\xBA\x00\x00\x00\x00\x48\x89\x7C\x24\x00\x8B\x04\x0A\x39\x05\x00\x00\x00\x00\x0F\x8F",
					"xxxxxxx????xxxxx????xxxxx????xxxx?xxxx?xxx????xxxx?xxxxxx????xxxx?xxxxx????xx");
			}

			// 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 20 49 8D 40 FF 41 B9 ? ? ? ? 33 F6 48 8B FA 48 8B E9 49 3B C1 77 14 - ON STEAM
			tlsf_memalign = (_tlsf_memalign)FindPattern(
				(DWORD64)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\x49\x8D\x40\xFF\x41\xB9\x00\x00\x00\x00\x33\xF6\x48\x8B\xFA\x48\x8B\xE9\x49\x3B\xC1\x77\x14",
				"xxxx?xxxx?xxxx?xxxxxxxxxxx????xxxxxxxxxxxxx");

			if (tlsf_memalign == NULL) {
				// 48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8B DA 49 8D 40 FF 33 D2 41 B9 ? ? ? ? 48 8B F1 8B FA 49 3B C1 77 14 BF ? ? ? ? 49 8D 40 07 48 83 E0 F8 48 3B C7 48 0F 47 F8 - ON EGS
				tlsf_memalign = (_tlsf_memalign)FindPattern(
					(DWORD64)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\x48\x8B\xDA\x49\x8D\x40\xFF\x33\xD2\x41\xB9\x00\x00\x00\x00\x48\x8B\xF1\x8B\xFA\x49\x3B\xC1\x77\x14\xBF\x00\x00\x00\x00\x49\x8D\x40\x07\x48\x83\xE0\xF8\x48\x3B\xC7\x48\x0F\x47\xF8",
					"xxxx?xxxx?xxxxxxxxxxxxxxxx????xxxxxxxxxxx????xxxxxxxxxxxxxxx");
			}

			// 48 89 5C 24 ? 48 89 54 24 ? 57 48 83 EC 60 48 8B 02 48 8B DA 48 8B 94 24 ? ? ? ? 0F 29 74 24 ? 0F 29 7C 24 ? 0F 28 F3 48 8B F9 48 8B CB 0F 28 FA FF 90 - ON STEAM
			camera_manager_play_track = (_camera_manager_play_track)FindPattern(
				(DWORD64)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x54\x24\x00\x57\x48\x83\xEC\x60\x48\x8B\x02\x48\x8B\xDA\x48\x8B\x94\x24\x00\x00\x00\x00\x0F\x29\x74\x24\x00\x0F\x29\x7C\x24\x00\x0F\x28\xF3\x48\x8B\xF9\x48\x8B\xCB\x0F\x28\xFA\xFF\x90",
				"xxxx?xxxx?xxxxxxxxxxxxxxx????xxxx?xxxx?xxxxxxxxxxxxxx");

			if (camera_manager_play_track == NULL) {
				// 48 89 5C 24 ? 48 89 74 24 ? 48 89 54 24 ? 57 48 83 EC 50 48 8B 02 48 8B DA 48 8B 94 24 ? ? ? ? 48 8B F9 0F 29 74 24 ? 48 8B CB 0F 29 7C 24 ? 0F 28 F2 0F 28 FB FF 90 - ON EGS
				camera_manager_play_track = (_camera_manager_play_track)FindPattern(
					(DWORD64)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x54\x24\x00\x57\x48\x83\xEC\x50\x48\x8B\x02\x48\x8B\xDA\x48\x8B\x94\x24\x00\x00\x00\x00\x48\x8B\xF9\x0F\x29\x74\x24\x00\x48\x8B\xCB\x0F\x29\x7C\x24\x00\x0F\x28\xF2\x0F\x28\xFB\xFF\x90",
					"xxxx?xxxx?xxxx?xxxxxxxxxxxxxxx????xxxxxxx?xxxxxxx?xxxxxxxx");
			}
#else
			if (isLL)
			{
				// 55 8B EC 83 E4 F0 81 EC ? ? ? ? 53 56 8B F1 57 33 FF 89 7E 04 89 7E 08 81 66 ? ? ? ? ? C7 06 ? ? ? ? C7 46
				cflycam_cflycam = (_cflycam_cflycam)FindPattern(
					(DWORD)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x55\x8B\xEC\x83\xE4\xF0\x81\xEC\x00\x00\x00\x00\x53\x56\x8B\xF1\x57\x33\xFF\x89\x7E\x04\x89\x7E\x08\x81\x66\x00\x00\x00\x00\x00\xC7\x06\x00\x00\x00\x00\xC7\x46",
					"xxxxxxxx????xxxxxxxxxxxxxxx?????xx????xx");

				// 55 8B EC 83 E4 F8 A1 ? ? ? ? 85 C0 75 48 B8 ? ? ? ? 84 05 ? ? ? ? 75 18 09 05 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 04
				memory = (_memory)FindPattern(
					(DWORD)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x55\x8B\xEC\x83\xE4\xF8\xA1\x00\x00\x00\x00\x85\xC0\x75\x48\xB8\x00\x00\x00\x00\x84\x05\x00\x00\x00\x00\x75\x18\x09\x05\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x68\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x83\xC4\x04",
					"xxxxxxx????xxxxx????xx????xxxx????x????x????x????xxx");

				// 53 8B 5C 24 0C 55 56 33 ED 57 85 C0 74 19 3D ? ? ? ? 73 12 83 C0 03 83 E0 FC 8B E8 83 F8 0C 77 05 BD
				tlsf_memalign = (void*)FindPattern(
					(DWORD)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x53\x8B\x5C\x24\x0C\x55\x56\x33\xED\x57\x85\xC0\x74\x19\x3D\x00\x00\x00\x00\x73\x12\x83\xC0\x03\x83\xE0\xFC\x8B\xE8\x83\xF8\x0C\x77\x05\xBD",
					"xxxxxxxxxxxxxxx");

				// 83 EC 10 53 56 8B 74 24 1C 8B 06 8B 50 44 57 8B F9 8B 4C 24 2C 51 8B CE FF D2 F3 0F 10 44 24 ? 8B 06 8B 50 6C 83 EC 08 F3 0F 11 44 24
				camera_manager_play_track = (_camera_manager_play_track)FindPattern(
					(DWORD)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x83\xEC\x10\x53\x56\x8B\x74\x24\x1C\x8B\x06\x8B\x50\x44\x57\x8B\xF9\x8B\x4C\x24\x2C\x51\x8B\xCE\xFF\xD2\xF3\x0F\x10\x44\x24\x00\x8B\x06\x8B\x50\x6C\x83\xEC\x08\xF3\x0F\x11\x44\x24",
					"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx?xxxxxxxxxxxxx");
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