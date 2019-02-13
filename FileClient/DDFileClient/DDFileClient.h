
// DDFileClient.h : PROJECT_NAME 應用程式的主要標頭檔
//

#pragma once
#pragma comment(lib, ".\\lib\\libzstd.lib")
//#pragma warning (disable : 4005)	// It's a bug in VS2010. intsafe.h and stdint.h both define INT8_MIN...
#include "zstd.h"


#ifndef __AFXWIN_H__
	#error "對 PCH 包含此檔案前先包含 'stdafx.h'"
#endif

#include "resource.h"		// 主要符號


// CDDFileClientApp:
// 請參閱實作此類別的 DDFileClient.cpp
//
#define MAX_SERVER_COUNT 10

class CDDFileClientApp : public CWinApp
{
public:
	CDDFileClientApp();

	// 
	CString				m_strTitle;
	CString				m_strVersion;

	// SYSTEM
	UINT				m_uServerStatus;
	char				m_chFileDir[256];
	char				m_chCaption[256];
	char				m_chName[32];
	char				m_chKey[32];
	char				m_chZabbixIP[16];
	UINT				m_uZabbixPort;
	char				m_chZabbixHost[50];
	char				m_chZabbixKey[50];
	UINT				m_uZabbixFreq;

	// SERVICE
	//UINT				m_uLocalPort[MAX_SERVER_COUNT];
	char				m_chServerIP[MAX_SERVER_COUNT][16];
	UINT				m_uServerPort[MAX_SERVER_COUNT];
	//BOOL				m_bServerStatus[MAX_SERVER_COUNT];
	BOOL				m_bAutoReconnect;

	// FILE
	char				m_chDict[256];
	ZSTD_DDict*			m_ptDict;
	UINT				m_uDictSize;

	// FILTER
	BOOL				m_bFilter2Tick;
	CStringArray		m_arrFilter;

	// LOG
	BOOL				m_bLogAllowDebug;

// 覆寫
public:
	virtual BOOL		InitInstance();

	void				GetVersion();
	void				GetIni();
	void				SetIni();

// 程式碼實作
	DECLARE_MESSAGE_MAP()
};

extern CDDFileClientApp theApp;