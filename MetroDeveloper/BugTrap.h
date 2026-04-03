#pragma once
#include "Patcher.h"

#ifdef _WIN64
typedef void (__fastcall* _BT_SetAppName)(const char* pszAppName);
typedef void (__fastcall* _BT_SetFlags)(unsigned long dwFlags);
typedef void (__fastcall* _BT_InstallSehFilter)();

#else

typedef void (__stdcall* _BT_SetAppName)(const char* pszAppName);
typedef void (__stdcall* _BT_SetFlags)(unsigned long dwFlags);
typedef void (__stdcall* _BT_InstallSehFilter)();
#endif

typedef enum flags
{
	BTF_NONE = 0x000,
	BTF_DETAILEDMODE = 0x001,
	BTF_EDITMAIL = 0x002,
	BTF_ATTACHREPORT = 0x004,
	BTF_LISTPROCESSES = 0x008,
	BTF_SHOWADVANCEDUI = 0x010,
	BTF_SCREENCAPTURE = 0x020,
	BTF_INTERCEPTSUEF = 0x080,
	BTF_DESCRIBEERROR = 0x100,
	BTF_RESTARTAPP = 0x200
};

class BugTrap : public Patcher
{
public:
	static void bugtrap_attach_process();
};

