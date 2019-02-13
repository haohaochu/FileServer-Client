
// MtkFileServer.h : PROJECT_NAME ���ε{�����D�n���Y��
//


#pragma once
#pragma comment(lib, ".\\lib\\libzstd.lib")
//#pragma warning (disable : 4005)	// It's a bug in VS2010. intsafe.h and stdint.h both define INT8_MIN...
#include "zstd.h"


#ifndef __AFXWIN_H__
	#error "�� PCH �]�t���ɮ׫e���]�t 'stdafx.h'"
#endif

#include "resource.h"		// �D�n�Ÿ�
#include "mtkfile.h"


// CMtkFileServerApp:
// �аѾ\��@�����O�� MtkFileServer.cpp
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

// �мg
public:
	virtual BOOL InitInstance();

	void GetVersion();
	void GetIni();
	void SetIni();

	void GetDict();

// �{���X��@
	DECLARE_MESSAGE_MAP()
};

extern CMtkFileServerApp theApp;

