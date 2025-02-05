#include "LogFile.h"
#include "Utils.h"
#include "stdio.h"

bool isLogEnabled = false;
string256 logFilename;
FILE* fLog;
CRITICAL_SECTION logCS;

LogFile::LogFile()
{
	isLogEnabled = Utils::GetBool("log", "enabled", false);
	if (isLogEnabled)
	{
		Utils::GetString("log", "filename", "uengine.log", logFilename, sizeof(logFilename));
		fLog = fopen(logFilename, "w");
		if (fLog != NULL)
		{
			InitializeCriticalSection(&logCS);
		}
	}
}

void __fastcall LogFile::slog(const char* s)
{
	EnterCriticalSection(&logCS);

	fprintf(fLog, s);
	fputc('\n', fLog);
	fflush(fLog);

	LeaveCriticalSection(&logCS);
}