#include "stdafx.h"
#include "SysLogClient.h"

// Load SysLogClient Dll
HINSTANCE			hInstSysLogClientDll;
SYSLOGSEND			SysLogSend;
WORD				wSeverity;
CRITICAL_SECTION	csSysLog;

BOOL LoadSysLogClientDll()
{
	hInstSysLogClientDll = LoadLibrary(SYSLOGCLIENTDLL);

	if ( hInstSysLogClientDll == NULL )
		return FALSE;
	else
	{
		SysLogSend = (SYSLOGSEND) GetProcAddress(hInstSysLogClientDll,"SysLogSend");
		if ( SysLogSend == NULL )
		{
			UnLoadSysLogClientDll();
			return FALSE;
		}
	}
	wSeverity = ~0;

	InitializeCriticalSectionAndSpinCount(&csSysLog,4000);
	//InitializeCriticalSection(&csSysLog);

	return TRUE;
}

void UnLoadSysLogClientDll()
{
	if ( hInstSysLogClientDll )
	{
		FreeLibrary(hInstSysLogClientDll);
		hInstSysLogClientDll = NULL;
	}

	DeleteCriticalSection(&csSysLog);
}

char* LogSend(char* aMsg, WORD ASeverity)
{
	char* pRetCode=NULL;

	if ( hInstSysLogClientDll && GetServerity(ASeverity) )
	{
		EnterCriticalSection(&csSysLog);
		pRetCode = SysLogSend(aMsg,ASeverity);
		LeaveCriticalSection(&csSysLog);
	}
	
	return pRetCode;
}

void SetServerity(WORD ASeverity, BOOL bEnable)
{
	if ( bEnable )
		wSeverity |= (1 << ASeverity);
	else
		wSeverity &= ~(1 << ASeverity);
}

BOOL GetServerity(WORD ASeverity)
{
	return ((wSeverity & (1<<ASeverity)) != 0);
}