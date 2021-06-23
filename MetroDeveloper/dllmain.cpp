#define _CRT_SECURE_NO_WARNINGS 1 // MSVS dno

#include <windows.h>
#include <vector>
#include <psapi.h>
#include "i_pathengine.h"
#include "MinHook.h"

#pragma comment (lib, "psapi.lib")

// settings
typedef char string256[256];

string256 logFilename;

string256 navmapFormat;
string256 navmapFilename;
string256 navmapResult;
bool exitWhenDone;

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

DWORD FindPattern(DWORD start_address, DWORD length, BYTE* pattern, char* mask)
{
	for (DWORD i = 0; i < length; i++)
		if (DataCompare((BYTE*)(start_address + i), pattern, mask))
			return (DWORD)(start_address + i);
	return NULL;
}

// nav_map part
struct Vec3f
{
	float x, y, z;
};

struct Face
{
	int v1, v2, v3;
	int userData;
	int surfaceType;
};

class FaceVertexMeshImpl : public iFaceVertexMesh
{
	public:
	std::vector<Vec3f> m_points;
	std::vector<Face> m_faces;

	virtual tSigned32 faces() const { return m_faces.size(); };
	virtual tSigned32 vertices() const { return m_points.size(); };
	virtual tSigned32 vertexIndex(tSigned32 face, tSigned32 vertexInFace) const
	{
		//printf("vertexIndex face=%d vertexInFace=%d\n", face, vertexInFace);
		switch(vertexInFace)
		{
			case 0:		return m_faces[face].v1;
			case 1:		return m_faces[face].v2;
			case 2:		return m_faces[face].v3;
			default:	return -1;
		}
	};
	virtual tSigned32 vertexX(tSigned32 v) const { return tSigned32(m_points[v].x * 1000); };
	virtual tSigned32 vertexY(tSigned32 v) const { return tSigned32(m_points[v].z * 1000); };
	virtual float vertexZ(tSigned32 v) const { return m_points[v].y * 1000; };
	virtual tSigned32 faceAttribute(tSigned32 face, tSigned32 attributeIndex) const
	{
		switch(attributeIndex)
		{
			case PE_FaceAttribute_SurfaceType:		return m_faces[face].surfaceType;
			case PE_FaceAttribute_UserData:			return m_faces[face].userData;
			default:								return -1;
		}
	};

	bool loadFromFile(const char *filename);
};

bool FaceVertexMeshImpl::loadFromFile(const char *filename)
{
	int version, count;
	FILE *f;

	f = fopen(filename, "rb");
	if(!f)
		return false;

	fread(&version, 1, 4, f);
	if(version != 1)
	{
		fclose(f);
		return false;
	}

	fread(&count, 1, 4, f);
	m_points.resize(count);
	fread(&m_points.front(), sizeof(Vec3f), count, f);

	fread(&count, 1, 4, f);
	m_faces.resize(count);
	for(int i = 0; i < count; i++)
	{
		fread(&m_faces[i].v1, 1, 4, f);
		fread(&m_faces[i].v2, 1, 4, f);
		fread(&m_faces[i].v3, 1, 4, f);
		m_faces[i].surfaceType = 1;
		m_faces[i].userData = -1;
	}

	fclose(f);
	return true;
}

class MemoryStreamImpl : public iOutputStream
{
	public:
	std::vector<char> m_data;

	bool save(const char *filename)
	{
		FILE *out = fopen(filename, "wb");
		if(!out)
			return false;

		size_t written = fwrite(&m_data.front(), 1, m_data.size(), out);
		fclose(out);

		return written == m_data.size();
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

iMesh* load_raw(iPathEngine *pathengine, const char *filename)
{
	FaceVertexMeshImpl m;

	if(!m.loadFromFile(filename))
	{
		printf("raw mesh '%s' load failed\n", filename);
		return NULL;
	}

	iFaceVertexMesh const * const pm = &m;
	iMesh *real_mesh = pathengine->buildMeshFromContent(&pm, 1, 0);

	return real_mesh;
}

iMesh* load_xml(iPathEngine *pathengine, const char *filename)
{
  FILE *f = fopen(filename, "rb");
  if(!f)
    return NULL;
    
  fseek(f, 0, SEEK_END);
  size_t length = ftell(f);
  fseek(f, 0, SEEK_SET);
  
  void *buffer = malloc(length);
  fread(buffer, 1, length, f);
  
  fclose(f);
  
  iMesh *real_mesh = pathengine->loadMeshFromBuffer("xml", (char*)buffer, length, 0);
  free(buffer);
  
  return real_mesh;
}

void savemesh(iPathEngine *pathengine)
{
	iMesh *real_mesh;
	
	if(strcmp(navmapFormat, "raw") == 0)
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

	tSigned32 shape1_data[] = {
		-300, 0,
		-150, 259,
		150, 259,
		300, 0,
		150, -259,
		-150, -259
	};
	iShape *shape1 = pathengine->newShape(6, shape1_data);
	printf("shape1 = %08X\n", shape1);
	if(!shape1)
	{
		delete ctx;
		delete real_mesh;
		return;
	}

	tSigned32 shape2_data[] = {
		-600, 0,
		-300, 519,
		300, 519,
		600, 0,
		300, -519,
		-300, -519
	};
	iShape *shape2 = pathengine->newShape(6, shape2_data);
	printf("shape2 = %08X\n", shape2);
	if(!shape2)
	{
		delete shape1;
		delete ctx;
		delete real_mesh;
		return;
	}

	tSigned32 shape3_data[] = {
		-100, 0,
		-50, 86,
		50, 86,
		100, 0,
		50, -86,
		-50, -86
	};
	iShape *shape3 = pathengine->newShape(6, shape3_data);
	printf("shape3 = %08X\n", shape3);
	if(!shape3)
	{
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

	result.putFloat(0.3f); // unknwown1
	result.putInt(ms_cp[0].size());
	result.put(ms_cp[0].ptr(), ms_cp[0].size());
	result.putInt(ms_pfp[0].size());
	result.put(ms_pfp[0].ptr(), ms_pfp[0].size());

	result.putFloat(0.15f); // unknwown2
	result.putInt(ms_cp[1].size());
	result.put(ms_cp[1].ptr(), ms_cp[1].size());
	result.putInt(ms_pfp[1].size());
	result.put(ms_pfp[1].ptr(), ms_pfp[1].size());

	result.putFloat(0.1f); // unknwown3
	result.putInt(ms_cp[2].size());
	result.put(ms_cp[2].ptr(), ms_cp[2].size());
	result.putInt(ms_pfp[2].size());
	result.put(ms_pfp[2].ptr(), ms_pfp[2].size());

	result.save(navmapResult);

	// free resources and exit
	delete shape3;
	delete shape2;
	delete shape1;

	delete ctx;

	delete real_mesh;

	return;
}

void BadQuitReset()
{
	HKEY hKey;
	DWORD disposition;
	if (RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\4A-Games\\Metro2033", 0, NULL, 0, KEY_SET_VALUE, 0, &hKey,
		&disposition) == ERROR_SUCCESS)
	{
		RegDeleteValue(hKey, "BadQuit");
		RegCloseKey(hKey);
	}
}

typedef void* (__stdcall* _getConsole)();
_getConsole getConsole = nullptr;

void uconsole_server_impl_execute(void* console, const char* cmd)
{
	if (console != nullptr)
	{
		(*(void(__cdecl**)(void*, const char*)) (*(DWORD*)console + 28)) (console, cmd);
	}
}

DWORD WINAPI NavMapThread(LPVOID)
{
	// ищем команду mov ecx, pathEngine
	// B9 ? ? ? ? FF D2 C7 46 ? ? ? ? ? 8B C6 C3 CC CC CC CC CC CC CC CC CC CC CC CC CC CC
	LPVOID mem = (LPVOID)FindPattern(
		(DWORD)mi.lpBaseOfDll,
		mi.SizeOfImage,
		(BYTE*)"\xB9\x00\x00\x00\x00\xFF\xD2\xC7\x46\x00\x00\x00\x00\x00\x8B\xC6\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC",
		"x????xxxx?????xxxxxxxxxxxxxxxxx");

	// читаем адрес iPathEngine*
	iPathEngine* pathEngine = (iPathEngine*) (*(DWORD*)(LPBYTE(mem) + 1));

	printf("pPathEngine = %08X\n", pathEngine);

	Sleep(5000);

	//while (*(DWORD*)pathEngine == NULL)
	//{
	//	Sleep(100);
	//}

	printf("InterfaceMajorVersion %d\n", pathEngine->getInterfaceMajorVersion());
	printf("InterfaceMinorVersion %d\n", pathEngine->getInterfaceMinorVersion());

	tSigned32 i, j, k;
	pathEngine->getReleaseNumbers(i, j, k);
	printf("ReleaseNumbers %d %d %d\n", i, j, k);

	savemesh(pathEngine);

	if (exitWhenDone)
	{
		uconsole_server_impl_execute(getConsole(), "quit");
	}

	return 0;
}

// log part
FILE* fLog;
CRITICAL_SECTION logCS;

typedef void(__cdecl* _Log)(char* format, ...);
_Log Log_Orig = nullptr;

void __cdecl Log_Hook(char* format, ...)
{
	EnterCriticalSection(&logCS);

	va_list v;
	va_start(v, format);

	vfprintf(fLog, format, v);

	char toGameLog[0x1000];
	vsprintf(toGameLog, format, v);
	Log_Orig(toGameLog);

	va_end(v);

	fputc('\n', fLog);
	fflush(fLog);

	LeaveCriticalSection(&logCS);
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
	GetPrivateProfileString(section_name, str_name, default_str, result, size, ".\\developer.ini");
}

bool getBool(const char* section_name, const char* bool_name, bool default_bool)
{
	string256 str;
	getString(section_name, bool_name, (default_bool ? "true" : "false"), str, sizeof(str));
	return (strcmp(str, "true") == 0) || (strcmp(str, "yes") == 0) || (strcmp(str, "on") == 0) || (strcmp(str, "1") == 0);
}

typedef void(__thiscall* _clevel_r_on_key_press)(void* _this, int key, int arg2, int arg3);
_clevel_r_on_key_press clevel_r_on_key_press_Orig = nullptr;

typedef void(__thiscall* _uconsole_server_impl_show)(void* console);
_uconsole_server_impl_show uconsole_server_impl_show = nullptr;

void __fastcall clevel_r_on_key_press_Hook(void* _this, void* _unused, int key, int arg2, int arg3)
{
	//printf("key = %d, arg2 = %d, arg3 = %d\n", key, arg2, arg3);

	if (key == 39)
	{
		uconsole_server_impl_show(getConsole());
	}

	clevel_r_on_key_press_Orig(_this, key, arg2, arg3);
}

BOOL APIENTRY DllMain(HINSTANCE hInstDLL, DWORD reason, LPVOID reserved)
{
	if(reason == DLL_PROCESS_ATTACH)
	{
		AllocConsole();
		freopen("CONOUT$", "w", stdout);

		printf("MetroDeveloper is loaded!\n");

		mi = GetModuleData(NULL);

		bool minhook = (MH_Initialize() == MH_OK);
		if (!minhook)
		{
			MessageBox(NULL, "MinHook not initialized!", "MinHook", MB_OK | MB_ICONERROR);
		}

		if(getBool("log", "enabled", false))
		{
			getString("log", "filename", "uengine.log", logFilename, sizeof(logFilename));
			fLog = fopen(logFilename, "w");
			if (fLog != NULL)
			{
				InitializeCriticalSection(&logCS);

				// 81 EC ? ? ? ? 8B 8C 24 ? ? ? ? 53 56
				LPVOID LogAddress = (LPVOID)FindPattern(
					(DWORD)mi.lpBaseOfDll,
					mi.SizeOfImage,
					(BYTE*)"\x81\xEC\x00\x00\x00\x00\x8B\x8C\x24\x00\x00\x00\x00\x53\x56",
					"xx????xxx????xx");

				if (minhook) {
					if (MH_CreateHook(LogAddress, &Log_Hook, reinterpret_cast<LPVOID*>(&Log_Orig)) == MH_OK) {
						if (MH_EnableHook(LogAddress) != MH_OK) {
							MessageBox(NULL, "MH_EnableHook() != MH_OK", "Log Hook", MB_OK | MB_ICONERROR);
						}
					} else {
						MessageBox(NULL, "MH_CreateHook() != MH_OK", "Log Hook", MB_OK | MB_ICONERROR);
					}
				}
			}
		}

		if (getBool("other", "badquit_reset", false))
		{
			BadQuitReset();
		}

		if (getBool("other", "no_videocard_msg", false))
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

		if (getBool("other", "nointro", false))
		{
			// 51 0F B7 05
			LPVOID IntroAddress = (LPVOID)FindPattern(
				(DWORD)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\x51\x0F\xB7\x05",
				"xxxx");

			BYTE ret[] = { 0xC3 };
			ASMWrite(IntroAddress, ret, sizeof(ret));
		}

		bool unlock_dev_console = getBool("other", "unlock_dev_console", false);
		bool navmap = strstr(GetCommandLine(), "-navmap");

		if (unlock_dev_console || navmap)
		{
			// 55 8B EC 83 E4 ? A1 ? ? ? ? 85 C0 56 57 75 ? E8 ? ? ? ? 85 C0 74 ? 8B F8 E8 ? ? ? ? EB ? 33 C0 8B F0 A3 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 5F
			getConsole = (_getConsole)FindPattern(
				(DWORD)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\x55\x8B\xEC\x83\xE4\x00\xA1\x00\x00\x00\x00\x85\xC0\x56\x57\x75\x00\xE8\x00\x00\x00\x00\x85\xC0\x74\x00\x8B\xF8\xE8\x00\x00\x00\x00\xEB\x00\x33\xC0\x8B\xF0\xA3\x00\x00\x00\x00\xE8\x00\x00\x00\x00\xA1\x00\x00\x00\x00\x5F",
				"xxxxx?x????xxxxx?x????xxx?xxx????x?xxxxx????x????x????x");
		}

		if(unlock_dev_console)
		{
			// 56 8B F1 80 7E 48 00
			uconsole_server_impl_show = (_uconsole_server_impl_show)FindPattern(
				(DWORD)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\x56\x8B\xF1\x80\x7E\x48\x00",
				"xxxxxxx");

			// 51 55 8B E9 8B 0D
			LPVOID clevel_r_on_key_press_Address = (LPVOID)FindPattern(
				(DWORD)mi.lpBaseOfDll,
				mi.SizeOfImage,
				(BYTE*)"\x51\x55\x8B\xE9\x8B\x0D",
				"xxxxxx");

			if (minhook) {
				if (MH_CreateHook(clevel_r_on_key_press_Address, &clevel_r_on_key_press_Hook, reinterpret_cast<LPVOID*>(&clevel_r_on_key_press_Orig)) == MH_OK) {
					if (MH_EnableHook(clevel_r_on_key_press_Address) != MH_OK) {
						MessageBox(NULL, "MH_EnableHook() != MH_OK", "clevel_r_on_key_press Hook", MB_OK | MB_ICONERROR);
					}
				} else {
					MessageBox(NULL, "MH_CreateHook() != MH_OK", "clevel_r_on_key_press Hook", MB_OK | MB_ICONERROR);
				}
			}
		}

		if (navmap)
		{
			getString("nav_map", "format", "raw", navmapFormat, sizeof(logFilename));
			getString("nav_map", "filename", "nav_map.raw", navmapFilename, sizeof(logFilename));
			getString("nav_map", "result", "nav_map.pe", navmapResult, sizeof(logFilename));
			exitWhenDone = getBool("nav_map", "exitwhendone", false);

			HANDLE thread = CreateThread(NULL, 0, NavMapThread, NULL, 0, NULL);
			CloseHandle(thread);
		}
	}
	
	return TRUE;
}