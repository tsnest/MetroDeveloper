#include "BugTrap.h"
#include "eh.h"

void BugTrap::bugtrap_attach_process()
{
	HMODULE dll = LoadLibrary("BugTrap.dll");

	if (dll != NULL) {
		_BT_SetAppName	BT_SetAppName				= (_BT_SetAppName)	GetProcAddress(dll, "BT_SetAppName");
		_BT_SetFlags	BT_SetFlags					= (_BT_SetFlags)	GetProcAddress(dll, "BT_SetFlags");
		_BT_InstallSehFilter BT_InstallSehFilter	= (_BT_InstallSehFilter) GetProcAddress(dll, "BT_InstallSehFilter");
		terminate_handler	 BT_CallCppFilter		= (terminate_handler) GetProcAddress(dll, "BT_CallCppFilter");

		BT_SetAppName("MetroDeveloper");
		BT_SetFlags(BTF_DETAILEDMODE | BTF_EDITMAIL | BTF_ATTACHREPORT | BTF_SHOWADVANCEDUI | BTF_SCREENCAPTURE | BTF_INTERCEPTSUEF);
		BT_InstallSehFilter();
		set_terminate(BT_CallCppFilter);
	}
}
