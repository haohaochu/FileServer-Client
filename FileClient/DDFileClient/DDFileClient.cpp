
// DDFileClient.cpp : �w�q���ε{�������O�欰�C
//

#include "stdafx.h"
#include "DDFileClient.h"
#include "DDFileClientDlg.h"

#pragma comment(lib, "Version.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CDDFileClientApp

BEGIN_MESSAGE_MAP(CDDFileClientApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CDDFileClientApp �غc

CDDFileClientApp::CDDFileClientApp()
{
	// �䴩���s�Ұʺ޲z��
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: �b���[�J�غc�{���X�A
	// �N�Ҧ����n����l�]�w�[�J InitInstance ��
}


// �Ȧ����@�� CDDFileClientApp ����

CDDFileClientApp theApp;


// CDDFileClientApp ��l�]�w

BOOL CDDFileClientApp::InitInstance()
{
	// ���p���ε{����T�M����w�ϥ� ComCtl32.dll 6 (�t) �H�᪩���A
	// �ӱҰʵ�ı�Ƽ˦��A�b Windows XP �W�A�h�ݭn InitCommonControls()�C
	// �_�h����������إ߳��N���ѡC
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// �]�w�n�]�t�Ҧ��z�Q�n�Ω����ε{������
	// �q�α�����O�C
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}


	AfxEnableControlContainer();

	// �إߴ߼h�޲z���A�H����ܤ���]�t
	// ����߼h���˵��δ߼h�M���˵�����C
	CShellManager *pShellManager = new CShellManager;

	// �зǪ�l�]�w
	// �p�G�z���ϥγo�ǥ\��åB�Q���
	// �̫᧹�����i�����ɤj�p�A�z�i�H
	// �q�U�C�{���X�������ݭn����l�Ʊ`���A
	// �ܧ��x�s�]�w�Ȫ��n�����X
	// TODO: �z���ӾA�׭ק惡�r��
	// (�Ҧp�A���q�W�٩β�´�W��)
	//SetRegistryKey(_T("���� AppWizard �Ҳ��ͪ����ε{��"));

	CDDFileClientDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: �b����m��ϥ� [�T�w] �Ӱ���ϥι�ܤ����
		// �B�z���{���X
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: �b����m��ϥ� [����] �Ӱ���ϥι�ܤ����
		// �B�z���{���X
	}

	// �R���W���ҫإߪ��߼h�޲z���C
	if (pShellManager != NULL)
	{
		delete pShellManager;
	}

	// �]���w�g������ܤ���A�Ǧ^ FALSE�A�ҥH�ڭ̷|�������ε{���A
	// �ӫD���ܶ}�l���ε{�����T���C
	return FALSE;
}

// ���o�����O
void CDDFileClientApp::GetVersion()
{
	char szFileName[128]={0};
	char chLogbuf[128]={0};
	
	// ���{�����|
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

void CDDFileClientApp::GetIni()
{
	char szIniPath[261]={0}, szFolder[260]={0}, szAppName[256]={0}, szDrive[3]={0};
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

	//memset(m_chCaption, NULL, 256);
	//GetPrivateProfileString("SYSTEM", "CAPTION", m_chFileDir, m_chCaption, 256, szIniPath);	
	//
	//if (m_chCaption[0] == 0)
	//	m_strTitle = CString(szAppName);
	//else
	//	m_strTitle.Format("%s-%s", szAppName, m_chCaption);
	m_strTitle.Format("%s-%s%s", szAppName, szDrive, szFolder);

	memset(m_chName, NULL, 32);
	GetPrivateProfileString("SYSTEM", "NAME", "", m_chName, 32, szIniPath);	

	char* chDef = NULL;
	for (UINT i=0; (i+1)<strlen(m_chFileDir); i++)
	{
		if (m_chFileDir[i] == '\\')
			chDef = &m_chFileDir[i];
	}
	CString strDef(chDef);
	strDef.Replace("\\", "");

	memset(m_chKey, NULL, 32);
	GetPrivateProfileString("SYSTEM", "KEY", strDef, m_chKey, 32, szIniPath);	

	memset(m_chServerIP, NULL, 16);
	memset(m_chZabbixIP, NULL, 16);
	memset(m_chZabbixHost, NULL, 50);
	memset(m_chZabbixKey, NULL, 50);

	// SYSTEM
	GetPrivateProfileString("SYSTEM", "ZABBIXIP", "10.99.0.84", m_chZabbixIP, 16, szIniPath);
	m_uZabbixPort = GetPrivateProfileInt("SYSTEM", "ZABBIXPORT", 10051, szIniPath);
	GetPrivateProfileString("SYSTEM", "ZABBIXHOST", "10.99.0.1_ddfs", m_chZabbixHost, 50, szIniPath);
	GetPrivateProfileString("SYSTEM", "ZABBIXKEY", "ddfs_status", m_chZabbixKey, 50, szIniPath);
	m_uZabbixFreq = GetPrivateProfileInt("SYSTEM", "ZABBIXFREQ", 0, szIniPath);
	
	// SERVICE
	//CString strLocal("");
	CString strIP("");
	CString strPort("");
	for (int i=0; i<MAX_SERVER_COUNT; i++)
	{
		//strLocal.Format("LOCALPORT%d", i);
		strIP.Format("SERVERIP%d", i);
		strPort.Format("SERVERPORT%d", i);
		//m_uLocalPort[i] = GetPrivateProfileInt("SERVICE", strLocal, 0, szIniPath);
		GetPrivateProfileString("SERVICE", strIP, 0, m_chServerIP[i], 16, szIniPath);
		m_uServerPort[i] = GetPrivateProfileInt("SERVICE", strPort, 0, szIniPath);
	}
	m_bAutoReconnect = GetPrivateProfileInt("SERVICE", "AUTOCONNECT", 1, szIniPath) == 0 ? FALSE : TRUE;


	// FILE 
	GetPrivateProfileString("SERVICE", "DICT", "zstd170125.dic", m_chDict, 256, szIniPath);

	CString strDict("");
	strDict.Format("%sdic\\%s", strExecutePath, m_chDict);
	HANDLE hDict = NULL;
	if ((hDict = CreateFile(strDict, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
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
			if ((m_ptDict = ZSTD_createDDict(buf, ret)) == NULL)
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

	// FILTER
	m_bFilter2Tick = GetPrivateProfileInt("FILTER", "FILTERFILE", 0, szIniPath) == 0 ? FALSE : TRUE;
	char chFilter[1024]={0};
	GetPrivateProfileString("FILTER", "FILTER", "", chFilter, 1024, szIniPath);
	
	if (chFilter[0] != 0)
	{
		char* start = chFilter;
		char* find = strstr(start, ";");
		while (1)
		{
			m_arrFilter.Add(CString(start, find-start));
			start = find+1;
			find = strstr(start, ";");

			if (!find) // �̫�@��
			{
				m_arrFilter.Add(CString(start));
				break;
			}
		}
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

void CDDFileClientApp::SetIni()
{
	WriteProfileString("SYSTEM", "PATH", m_chFileDir);
	WriteProfileString("SYSTEM", "ZABBIXIP", m_chZabbixIP);
	WriteProfileInt("SYSTEM", "ZABBIXPORT", m_uZabbixPort);
	WriteProfileString("SYSTEM", "ZABBIXHOST", m_chZabbixHost);
	WriteProfileString("SYSTEM", "ZABBIXKEY", m_chZabbixKey);
	WriteProfileInt("SYSTEM", "ZABBIXFREQ", m_uZabbixFreq);

	CString strIP("");
	CString strPort("");
	for (int i=0; i<MAX_SERVER_COUNT; i++)
	{
		if (m_chServerIP[i][0] == 0)
			continue;

		strIP.Format("SERVERIP%d", i);
		strPort.Format("SERVERPORT%d", i);
		WriteProfileString("SERVICE", strIP, m_chServerIP[i]);
		WriteProfileInt("SERVICE", strPort, m_uServerPort[i]);
	}

	WriteProfileInt("LOG", "LOGDEBUG", m_bLogAllowDebug ? 1:0);
}
