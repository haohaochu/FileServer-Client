
// MtkFileServer.h : PROJECT_NAME 應用程式的主要標頭檔
//


#pragma once
#pragma comment(lib, ".\\lib\\libzstd.lib")
//#pragma warning (disable : 4005)	// It's a bug in VS2010. intsafe.h and stdint.h both define INT8_MIN...
#include "zstd.h"


#ifndef __AFXWIN_H__
	#error "對 PCH 包含此檔案前先包含 'stdafx.h'"
#endif

#include "resource.h"		// 主要符號
#include "mtkfile.h"


// CMtkFileServerApp:
// 請參閱實作此類別的 MtkFileServer.cpp
//

class CMtkFileServerApp : public CWinApp
{
public:
	CMtkFileServerApp();

	//
	CString				m_strTitle;
	CString				m_strVersion;

	// SYSTEM
	char				m_chFileDir[256];
	char				m_chCaption[256];
	UINT				m_uListenPort;
	char				m_chServerIP[16];
	UINT				m_uServerPort;
	char				m_chZabbixIP[16];
	UINT				m_uZabbixPort;
	char				m_chZabbixHost[50];
	char				m_chZabbixKey[50];
	UINT				m_uZabbixFreq;

	// SERVICE
	BOOL				m_bNetDefaultTransferAll;
	BOOL				m_bNetDefaultCompressAll;
	char				m_chDict[256];
	ZSTD_CDict*			m_ptDict;
	UINT				m_uDictSize;
	// LOG
	BOOL				m_bLogAllowDebug;

// 覆寫
public:
	virtual BOOL InitInstance();

	void GetVersion();
	void GetIni();
	void SetIni();

	void GetDict();

// 程式碼實作
	DECLARE_MESSAGE_MAP()
};

extern CMtkFileServerApp theApp;

