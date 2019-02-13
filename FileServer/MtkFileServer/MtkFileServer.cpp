
// MtkFileServer.cpp : 定義應用程式的類別行為。
//

#include "stdafx.h"
#include "MtkFileServer.h"
#include "MtkFileServerDlg.h"

#pragma comment(lib, "Version.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMtkFileServerApp

BEGIN_MESSAGE_MAP(CMtkFileServerApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CMtkFileServerApp 建構

CMtkFileServerApp::CMtkFileServerApp()
{
	// 支援重新啟動管理員
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: 在此加入建構程式碼，
	// 將所有重要的初始設定加入 InitInstance 中
}


// 僅有的一個 CMtkFileServerApp 物件

CMtkFileServerApp theApp;

// CMtkFileServerApp 初始設定

BOOL CMtkFileServerApp::InitInstance()
{
	// 假如應用程式資訊清單指定使用 ComCtl32.dll 6 (含) 以後版本，
	// 來啟動視覺化樣式，在 Windows XP 上，則需要 InitCommonControls()。
	// 否則任何視窗的建立都將失敗。
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// 設定要包含所有您想要用於應用程式中的
	// 通用控制項類別。
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}


	AfxEnableControlContainer();

	// 建立殼層管理員，以防對話方塊包含
	// 任何殼層樹狀檢視或殼層清單檢視控制項。
	CShellManager *pShellManager = new CShellManager;

	// 標準初始設定
	// 如果您不使用這些功能並且想減少
	// 最後完成的可執行檔大小，您可以
	// 從下列程式碼移除不需要的初始化常式，
	// 變更儲存設定值的登錄機碼
	// TODO: 您應該適度修改此字串
	// (例如，公司名稱或組織名稱)
	//SetRegistryKey(_T("本機 AppWizard 所產生的應用程式"));

	CMtkFileServerDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: 在此放置於使用 [確定] 來停止使用對話方塊時
		// 處理的程式碼
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: 在此放置於使用 [取消] 來停止使用對話方塊時
		// 處理的程式碼
	}

	// 刪除上面所建立的殼層管理員。
	if (pShellManager != NULL)
	{
		delete pShellManager;
	}

	// 因為已經關閉對話方塊，傳回 FALSE，所以我們會結束應用程式，
	// 而非提示開始應用程式的訊息。
	return FALSE;
}

// 取得版本別
void CMtkFileServerApp::GetVersion()
{
	char szFileName[128]={0};
	char chLogbuf[128]={0};
	
	// 取程式路徑
	if (GetModuleFileName(NULL, szFileName, sizeof(szFileName)) == 0)
	{
//		sprintf(chLogbuf,"[CPats2mtkApp::GetVersion()]GetModuleFileName() Error(%d)",GetLastError());
//		LogSend(chLogbuf,ID_SYSLOG_SEVERITY_ERROR);
		return;
	}

	DWORD dwInfoSize;
	if ((dwInfoSize=GetFileVersionInfoSize(szFileName,NULL)) == 0)
	{
//		sprintf(chLogbuf,"[CPats2mtkApp::GetVersion()]GetFileVersionInfoSize(%s) Error(%d)",szFileName,GetLastError());
//		LogSend(chLogbuf,ID_SYSLOG_SEVERITY_ERROR);
		return;
	}
    
	char* pVersionInfo = NULL;

	if ((pVersionInfo=new char[dwInfoSize+1]) == NULL)
	{
//		LogSend("[CPats2mtkApp::GetVersion()]New Memory Failure!",ID_SYSLOG_SEVERITY_ERROR);
		return;
	}

	ZeroMemory(pVersionInfo,dwInfoSize+1);

	if (!GetFileVersionInfo(szFileName,NULL,dwInfoSize,pVersionInfo))
	{
		delete [] pVersionInfo;
//		sprintf(chLogbuf,"[CPats2mtkApp::GetVersion()]GetFileVersionInfo(%s) Error(%d)",szFileName,GetLastError());
//		LogSend(chLogbuf,ID_SYSLOG_SEVERITY_ERROR);
		return;
	}

	struct LANGANDCODEPAGE {
		WORD wLanguage;
		WORD wCodePage;
	}*lpTranslate;
	
	UINT cbTranslate;
	// Read the list of languages and code pages.

    if (!VerQueryValue(pVersionInfo,TEXT("\\"),(LPVOID*)&lpTranslate,&cbTranslate) || cbTranslate == 0)
	{
		delete [] pVersionInfo;
//		sprintf(chLogbuf,"[CPats2mtkApp::GetVersion()]VerQueryValue(%s) Error",szFileName);
//		LogSend(chLogbuf,ID_SYSLOG_SEVERITY_ERROR);
		return;
	}

	m_strVersion.Format("Ver. %d.%d.%d.%d", lpTranslate[2].wCodePage, lpTranslate[2].wLanguage, lpTranslate[3].wCodePage, lpTranslate[3].wLanguage);

	delete [] pVersionInfo;
}

void CMtkFileServerApp::GetIni()
{
	char	szIniPath[261]={0}, szFolder[260]={0}, szAppName[256]={0}, szDrive[3]={0};
	CString	strbuf;
	CString strExecutePath;
	// then try current working dir followed by app folder
	GetModuleFileName(NULL, szIniPath, sizeof(szIniPath)-1);
	_splitpath_s(szIniPath, szDrive, 3, szFolder, 260, szAppName, 256, NULL, 0);
	// chNowDirectory
	GetCurrentDirectory(sizeof(szIniPath)-1, szIniPath);
	strExecutePath.Format("%s\\", szIniPath);

	// SYSTEM
	_makepath_s(szIniPath, NULL, szIniPath, szAppName, ".ini");
	free((void*)m_pszProfileName);
	m_pszProfileName = _strdup(szIniPath);

	memset(m_chFileDir, NULL, 256);
	GetPrivateProfileString("SYSTEM", "PATH", "D:\\Quote\\mtk\\", m_chFileDir, 256, szIniPath);	
	if(m_chFileDir[strlen(m_chFileDir)-1] != '\\')
		strcat_s(m_chFileDir, "\\");

//	memset(m_chCaption, NULL, 256);
//	GetPrivateProfileString("SYSTEM", "CAPTION", szFolder, m_chCaption, 256, szIniPath);	
	
//	if (m_chCaption[0] == 0)
//		m_strTitle = CString(szAppName);
//	else
//		m_strTitle.Format("%s-%s", szAppName, m_chCaption);
	m_strTitle.Format("%s-%s%s", szAppName, szDrive, szFolder);

	memset(m_chServerIP, NULL, 16);
	memset(m_chZabbixIP, NULL, 16);
	memset(m_chZabbixHost, NULL, 50);
	memset(m_chZabbixKey, NULL, 50);
	
	m_uListenPort = GetPrivateProfileInt("SYSTEM", "LISTEN", 8877, szIniPath);
	GetPrivateProfileString("SYSTEM", "DBIP", "10.99.0.1", m_chServerIP, 16, szIniPath);
	m_uServerPort = GetPrivateProfileInt("SYSTEM", "DBPORT", 8083, szIniPath);
	GetPrivateProfileString("SYSTEM", "ZABBIXIP", "10.99.0.84", m_chZabbixIP, 16, szIniPath);
	m_uZabbixPort = GetPrivateProfileInt("SYSTEM", "ZABBIXPORT", 10051, szIniPath);
	GetPrivateProfileString("SYSTEM", "ZABBIXHOST", "10.99.0.1_ddfs", m_chZabbixHost, 50, szIniPath);
	GetPrivateProfileString("SYSTEM", "ZABBIXKEY", "ddfs_status", m_chZabbixKey, 50, szIniPath);
	m_uZabbixFreq = GetPrivateProfileInt("SYSTEM", "ZABBIXFREQ", 0, szIniPath);
	
	// SERVICE
	m_bNetDefaultTransferAll = GetPrivateProfileInt("SERVICE", "DEFTRANSMITALL", 0, szIniPath) == 0 ? FALSE : TRUE;
	m_bNetDefaultCompressAll = GetPrivateProfileInt("SERVICE", "DEFCOMPRESSALL", 0, szIniPath) == 0 ? FALSE : TRUE;
	
	GetPrivateProfileString("SERVICE", "DICT", "zstd170125.dic", m_chDict, 256, szIniPath);

	CString strDict("");
	strDict.Format("%sdic\\%s", strExecutePath, m_chDict);
	HANDLE hDict = NULL;
	if (!m_bNetDefaultCompressAll)
	{
		m_ptDict = NULL;
	}
	else if ((hDict = CreateFile(strDict, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
	{
		// Log
		m_ptDict = NULL;
	}
	else
	{
		DWORD len = 256*1024;
		DWORD ret = 0;
		char* buf = new char[len];
		memset(buf, NULL, len);
		if (!ReadFile(hDict, buf, len, &ret, NULL))
		{
			// Log
			m_ptDict = NULL;
		}
		else
		{
			if ((m_ptDict = ZSTD_createCDict(buf, ret, 1)) == NULL)
			{
				// Log
				m_ptDict = NULL;
				m_uDictSize = 0;
			}
			else
				m_uDictSize = ret;
		}

		delete buf;
	}

	// LOG
	m_bLogAllowDebug = GetPrivateProfileInt("LOG", "LOGDEBUG", 0, szIniPath) == 0 ? FALSE : TRUE;

	//SetServerity(ID_SYSLOG_SEVERITY_EMERGENCY,TRUE);		//0
	//SetServerity(ID_SYSLOG_SEVERITY_ALERT,TRUE);			//1
	//SetServerity(ID_SYSLOG_SEVERITY_CRITICAL,TRUE);		//2
	//SetServerity(ID_SYSLOG_SEVERITY_ERROR,TRUE);			//3
	//SetServerity(ID_SYSLOG_SEVERITY_WARNING,TRUE);		//4
	//SetServerity(ID_SYSLOG_SEVERITY_NOTICE,TRUE);			//5
	//SetServerity(ID_SYSLOG_SEVERITY_INFORMATIONAL,TRUE);	//6
	//SetServerity(ID_SYSLOG_SEVERITY_DEBUG,TRUE);			//7

	GetVersion();
}

void CMtkFileServerApp::SetIni()
{
	WriteProfileString("SYSTEM", "PATH", m_chFileDir);
	WriteProfileInt("SYSTEM", "LISTEN", m_uListenPort);
	WriteProfileString("SYSTEM", "DBIP", m_chServerIP);
	WriteProfileInt("SYSTEM", "DBPORT", m_uServerPort);
	WriteProfileString("SYSTEM", "ZABBIXIP", m_chZabbixIP);
	WriteProfileInt("SYSTEM", "ZABBIXPORT", m_uZabbixPort);
	WriteProfileString("SYSTEM", "ZABBIXHOST", m_chZabbixHost);
	WriteProfileString("SYSTEM", "ZABBIXKEY", m_chZabbixKey);
	WriteProfileInt("SYSTEM", "ZABBIXFREQ", m_uZabbixFreq);

	WriteProfileInt("SERVICE", "DEFTRANSMITALL", m_bNetDefaultTransferAll ? 1:0);
	WriteProfileInt("SERVICE", "DEFCOMPRESSALL", m_bNetDefaultCompressAll ? 1:0);

	WriteProfileInt("LOG", "LOGDEBUG", m_bLogAllowDebug ? 1:0);
}

void CMtkFileServerApp::GetDict()
{
	//m_ptDict = NULL;
	//memset(m_chDict, NULL, 256);
}
