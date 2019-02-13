
// MtkFileServerDlg.cpp : ��@��
//

#include "stdafx.h"
#include "MtkFileServer.h"
#include "MtkFileServerDlg.h"
#include "afxdialogex.h"
#include "notifydlg.h"
#include "..\include_src\syslog\syslogclient.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// �� App About �ϥ� CAboutDlg ��ܤ��

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// ��ܤ�����
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �䴩

// �{���X��@
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CTabDlg1 implementation 

CTabDlg1::CTabDlg1(CWnd* pParent/* = NULL*/)
	: CDialogEx(CTabDlg1::IDD, pParent)
{

}

BEGIN_MESSAGE_MAP(CTabDlg1, CDialogEx)
	ON_BN_CLICKED(IDC_FILETOOLBARBUTTON1, &CTabDlg1::OnReadonlyFile)
	ON_BN_CLICKED(IDC_FILETOOLBARBUTTON2, &CTabDlg1::OnRefleshFile)
	ON_BN_CLICKED(IDC_FILEFILTERCLEAR, &CTabDlg1::OnClearFilter)
	ON_EN_CHANGE(IDC_FILEFILTEREDIT, &CTabDlg1::OnChangeFilter)
END_MESSAGE_MAP()

void CTabDlg1::OnReadonlyFile()
{
	// �]�w�ɮװ�Ū���s
	CMtkFileServerDlg* parent = (CMtkFileServerDlg*)theApp.m_pMainWnd;
	int selectcount = parent->m_FileGrid.GetSelectedRows()->GetCount();

	int index = 0;
	MTK* mtk = NULL;
	CString strFilename("");
	CString strLog("");
	
	CXTPReportColumnOrder *pSortOrder = parent->m_FileGrid.GetColumns()->GetSortOrder();
	BOOL bDecreasing = pSortOrder->GetCount() > 0 && pSortOrder->GetAt(0)->IsSortedDecreasing();

	AcquireSRWLockExclusive(&parent->m_ptFileThread->m_lockFileList);

	for (int i=0; i<selectcount; i++)
	{
		index = parent->m_FileGrid.GetSelectedRows()->GetAt(i)->GetIndex();
		
		if (bDecreasing)
			index = parent->m_ptFileThread->m_ptSortList[parent->m_FileGrid.GetRows()->GetCount()-index-1].m_index;
		else
			index = parent->m_ptFileThread->m_ptSortList[index].m_index;
		
		mtk = parent->m_ptFileThread->m_arrMtkFile[index];
		
		if (mtk->m_info.m_attribute == FALSE)
		{
			strFilename.Format("%s%s", parent->m_ptFileThread->m_chFilePath, mtk->m_chFilename);
			SetFileAttributes(strFilename, FILE_ATTRIBUTE_READONLY);
			mtk->m_info.m_attribute = TRUE;

			strLog.Format("�j���ɮװ�Ū[%s]", mtk->m_chFilename);
			parent->m_ptDFLog->DFLogSend(strLog, DF_SEVERITY_INFORMATION);
			InterlockedIncrement(&parent->m_ptFileThread->m_uAge);
		}
//		else
//		{
//			strLog.Format("�ɮפ������s �L�k�j���ɮװ�Ū[%s]", mtk->m_chFilename);
//			parent->m_ptDFLog->DFLogSend(strLog, DF_SEVERITY_HIGH);
//		}
	}

	ReleaseSRWLockExclusive(&parent->m_ptFileThread->m_lockFileList);
}

void CTabDlg1::OnRefleshFile()
{
	// ���歫�s��z���s
	// �S�ƻ�n�@��, ���ӴN�@���A���s��z
}

void CTabDlg1::OnClearFilter()
{
	CMtkFileServerDlg* parent = (CMtkFileServerDlg*)theApp.m_pMainWnd;
	parent->m_FileFilterEdit.SetWindowText(_T(""));
	parent->m_FileGrid.SetFilterText(_T(""));
}

void CTabDlg1::OnChangeFilter()
{
	CMtkFileServerDlg* parent = (CMtkFileServerDlg*)theApp.m_pMainWnd;
	CString strFilter("");
	parent->m_FileFilterEdit.GetWindowText(strFilter);
	parent->m_FileGrid.SetFilterText(strFilter);
}

/*afx_msg */void CTabDlg1::OnOK()
{

}

/*afx_msg */void CTabDlg1::OnCancel()
{

}

/*afx_msg */void CTabDlg1::OnDestroy()
{

}


// CTabDlg2 implementation
CTabDlg2::CTabDlg2(CWnd* pParent/* = NULL*/)
	: CDialogEx(CTabDlg2::IDD, pParent)
{

}

BEGIN_MESSAGE_MAP(CTabDlg2, CDialogEx)
	ON_BN_CLICKED(IDC_NETTOOLBARBUTTON1, &CTabDlg2::OnStartServer)
	ON_BN_CLICKED(IDC_NETTOOLBARBUTTON2, &CTabDlg2::OnStopServer)
	ON_BN_CLICKED(IDC_NETTOOLBARBUTTON3, &CTabDlg2::OnDisconnect)
	ON_BN_CLICKED(IDC_NETTOOLBARBUTTON4, &CTabDlg2::OnDebug)
	ON_NOTIFY(XTP_NM_REPORT_SELCHANGED, IDC_NETLISTGRID, &CTabDlg2::OnSelectFileList)
END_MESSAGE_MAP()

/*afx_msg */void CTabDlg2::OnStartServer()
{
	CMtkFileServerDlg* parent = (CMtkFileServerDlg*)theApp.m_pMainWnd;
	CString strLog("");

	// 1.�s�u�޲z��
	if (parent->m_ptDFNet == NULL)
	{
		parent->m_ptDFNet = new CDFNetMaster(CDFNetMaster::ThreadFunc, parent, theApp.m_uListenPort);
		AcquireSRWLockExclusive(&parent->m_lockServer);
		VERIFY(parent->m_ptDFNet->CreateThread());

		strLog.Format("[�ϥΪ̩R�O]���ҳs�u�޲z��(Thread:%d)", parent->m_ptDFNet->m_nThreadID);
		parent->m_ptDFLog->DFLogSend(strLog, DF_SEVERITY_INFORMATION);
	}
}

/*afx_msg */void CTabDlg2::OnStopServer()
{
	CMtkFileServerDlg* parent = (CMtkFileServerDlg*)theApp.m_pMainWnd;
	DWORD nThreadID = 0;
	CString strLog("");

	AcquireSRWLockExclusive(&parent->m_NetSlaveLock);

	// 0.�T�T~�s�u�ʱ��p�ɾ�
	//parent->KillTimer(IDC_TIMER);

	// 1.�T�T~�s�u�޲z��
	if (parent->m_ptDFNet)
	{
		nThreadID = parent->m_ptDFNet->m_nThreadID;
		parent->m_ptDFNet->ForceStopMaster();
		parent->m_ptDFNet->StopThread();
		WaitForSingleObject(parent->m_ptDFNet->m_hThread, INFINITE);
		
		strLog.Format("[�ϥΪ̩R�O]�����s�u�޲z��(Thread:%d)", nThreadID);
		parent->m_ptDFLog->DFLogSend(strLog, DF_SEVERITY_INFORMATION);
		
		delete parent->m_ptDFNet;
		parent->m_ptDFNet = NULL;
	}

	ReleaseSRWLockExclusive(&parent->m_NetSlaveLock);
}

/*afx_msg */void CTabDlg2::OnDisconnect()
{
	CMtkFileServerDlg* parent = (CMtkFileServerDlg*)theApp.m_pMainWnd;
	int index = 0;
	char* key = NULL;
	CString strLog("");

	AcquireSRWLockExclusive(&parent->m_NetSlaveLock);

	int selectcount = parent->m_NetListGrid.GetSelectedRows()->GetCount();
	for (int i=0; i<selectcount; i++)
	{
		index = parent->m_NetListGrid.GetSelectedRows()->GetAt(i)->GetIndex();
		key = parent->m_NetSlaveList[index].m_key;
		
		CDFNetSlave* slave = (CDFNetSlave*)parent->m_ptDFNet->m_mapSlaveList[key];
		
		if (slave)
		{
			slave->ForceStopSlave();

			strLog.Format("[�ϥΪ̩R�O]�j��s�u���_(�D��:%s)", slave->m_host);
			parent->m_ptDFLog->DFLogSend(strLog, DF_SEVERITY_INFORMATION);
		}
//		else
//		{
//			strLog.Format("�ɮפ������s �L�k�j���ɮװ�Ū[%s]", mtk->m_chFilename);
//			parent->m_ptDFLog->DFLogSend(strLog, DF_SEVERITY_HIGH);
//		}
	}

	ReleaseSRWLockExclusive(&parent->m_NetSlaveLock);
}


/*afx_msg */void CTabDlg2::OnDebug()
{
	CMtkFileServerDlg* parent = (CMtkFileServerDlg*)theApp.m_pMainWnd;
	int index = 0;
	char* key = NULL;
	CString strLog("");

	AcquireSRWLockExclusive(&parent->m_NetSlaveLock);

	int selectcount = parent->m_NetListGrid.GetSelectedRows()->GetCount();
	for (int i=0; i<selectcount; i++)
	{
		index = parent->m_NetListGrid.GetSelectedRows()->GetAt(i)->GetIndex();
		key = parent->m_NetSlaveList[index].m_key;

		CDFNetSlave* slave = (CDFNetSlave*)parent->m_ptDFNet->m_mapSlaveList[key];
		
		if (slave)
		{
			slave->m_debug = TRUE;

			strLog.Format("[�ϥΪ̩R�O]�}�Ұ����Ҧ�(�D��:%s)", slave->m_host);
			parent->m_ptDFLog->DFLogSend(strLog, DF_SEVERITY_INFORMATION);
		}
//		else
//		{
//			strLog.Format("�ɮפ������s �L�k�j���ɮװ�Ū[%s]", mtk->m_chFilename);
//			parent->m_ptDFLog->DFLogSend(strLog, DF_SEVERITY_HIGH);
//		}
	}

	ReleaseSRWLockExclusive(&parent->m_NetSlaveLock);
}

/*afx_msg */void CTabDlg2::OnSelectFileList(NMHDR* pNMHDR, LRESULT* pResult)
{
	CMtkFileServerDlg* parent = (CMtkFileServerDlg*)theApp.m_pMainWnd;
	
	if (parent->m_NetListGrid.GetSelectedRows()->GetCount() != 1)
		return;

	int select = parent->m_NetListGrid.GetSelectedRows()->GetAt(0)->GetIndex();
	//CString strHost = parent->m_NetListGrid.GetRecords()->GetAt(select)->GetItem(1)->GetCaption();
	CString strKey = parent->m_NetSlaveList[select].m_key;
	CDFNetSlave* slave = (CDFNetSlave*)parent->m_ptDFNet->m_mapSlaveList[strKey];
	
	if (slave == NULL)
		return;

	for (POSITION pos = slave->m_mapWork.GetStartPosition(); pos != NULL; )
	{
		UINT key;
		WORK* work;
		slave->m_mapWork.GetNextAssoc(pos, key, work);
		parent->m_NetFileGrid.AddRecord(new CDFSetMenuRecord(parent, CString(work->m_mtk->m_chFilename)));
	}
}

/*afx_msg */void CTabDlg2::OnOK()
{

}

/*afx_msg */void CTabDlg2::OnCancel()
{

}

/*afx_msg */void CTabDlg2::OnDestroy()
{

}


// CTabDlg3 implementation 
CTabDlg3::CTabDlg3(CWnd* pParent/* = NULL*/)
	: CDialogEx(CTabDlg3::IDD, pParent)
{

}

BEGIN_MESSAGE_MAP(CTabDlg3, CDialogEx)
	ON_BN_CLICKED(IDC_SETTOOLBARBUTTON1, &CTabDlg3::OnStartServer)
	ON_BN_CLICKED(IDC_SETTOOLBARBUTTON2, &CTabDlg3::OnStopServer)
	ON_BN_CLICKED(IDC_SETTOOLBARBUTTON3, &CTabDlg3::OnSave)
	ON_NOTIFY(XTP_NM_REPORT_SELCHANGED, IDC_SETMENUGRID, &CTabDlg3::OnChangeMenu)
	ON_NOTIFY(XTP_NM_REPORT_VALUECHANGED, IDC_SETLISTGRID, &CTabDlg3::OnChangeSettings)
END_MESSAGE_MAP()


void CTabDlg3::RefreshSettingPage()
{
	OnChangeMenu(NULL, NULL);
}

/*afx_msg */void CTabDlg3::OnStartServer()
{
	CMtkFileServerDlg* parent = (CMtkFileServerDlg*)theApp.m_pMainWnd;
	CString strLog("");

	// 1.�s�u�޲z��
	if (parent->m_ptDFNet == NULL)
	{
		parent->m_ptDFNet = new CDFNetMaster(CDFNetMaster::ThreadFunc, parent, theApp.m_uListenPort);
		AcquireSRWLockExclusive(&parent->m_lockServer);
		VERIFY(parent->m_ptDFNet->CreateThread());

		strLog.Format("[�ϥΪ̩R�O]���ҳs�u�޲z��(Thread:%d)", parent->m_ptDFNet->m_nThreadID);
		parent->m_ptDFLog->DFLogSend(strLog, DF_SEVERITY_INFORMATION);
	}

	OnChangeMenu(NULL, NULL);
}

/*afx_msg */void CTabDlg3::OnStopServer()
{
	CMtkFileServerDlg* parent = (CMtkFileServerDlg*)theApp.m_pMainWnd;
	DWORD nThreadID = 0;
	CString strLog("");

	AcquireSRWLockExclusive(&parent->m_NetSlaveLock);

	// 0.�T�T~�s�u�ʱ��p�ɾ�
	//parent->KillTimer(IDC_TIMER);

	// 1.�T�T~�s�u�޲z��
	if (parent->m_ptDFNet)
	{
		nThreadID = parent->m_ptDFNet->m_nThreadID;
		parent->m_ptDFNet->ForceStopMaster();
		parent->m_ptDFNet->StopThread();
		WaitForSingleObject(parent->m_ptDFNet->m_hThread, INFINITE);
		
		strLog.Format("[�ϥΪ̩R�O]�����s�u�޲z��(Thread:%d)", nThreadID);
		parent->m_ptDFLog->DFLogSend(strLog, DF_SEVERITY_INFORMATION);
		
		delete parent->m_ptDFNet;
		parent->m_ptDFNet = NULL;
	}

	ReleaseSRWLockExclusive(&parent->m_NetSlaveLock);
	OnChangeMenu(NULL, NULL);
}

/*afx_msg */void CTabDlg3::OnSave()
{
	CMtkFileServerDlg* parent = (CMtkFileServerDlg*)theApp.m_pMainWnd;
	
	parent->SaveIni();
}

/*afx_msg */void CTabDlg3::OnChangeMenu(NMHDR* pNMHDR, LRESULT* pResult)
{
	CMtkFileServerDlg* parent = (CMtkFileServerDlg*)theApp.m_pMainWnd;
	
	int select = parent->m_SetMenuGrid.GetSelectedRows()->GetAt(0)->GetIndex();
		
	parent->m_uServerStatus = parent->m_ptDFNet == NULL ? 1 : 0;
	switch (select)
	{
	case 0: // �t�Ϊ��A
		{
			parent->m_SetListGrid.GetRecords()->RemoveAll();
			parent->m_SetListGrid.AddRecord(new CDFSetListRecord(NULL, CString("�t�Ϊ��A"), CString(parent->m_uServerStatus==0 ? "�A�ȥ��`" : "�A�Ȱ���")));
			//parent->m_SetListGrid.AddRecord(new CDFSetListRecord(NULL, CString("�{������"), ));
			parent->m_SetListGrid.GetRecords()->GetAt(0)->GetItem(1)->SetIconIndex(parent->m_uServerStatus==0 ? 0 : 1);
			parent->m_SetListGrid.GetColumns()->GetAt(1)->SetEditable(FALSE);
			parent->m_SetListGrid.Populate();
			break;
		}
	case 1: // �t�γ]�w
		{
			parent->m_SetListGrid.GetRecords()->RemoveAll();
			parent->m_SetListGrid.AddRecord(new CDFSetListRecord(NULL, CString("�ɮץؿ�"), CString(theApp.m_chFileDir)));
			parent->m_SetListGrid.AddRecord(new CDFSetListRecord(NULL, CString("��Ʈw���A��IP"), CString(theApp.m_chServerIP)));
			parent->m_SetListGrid.AddRecord(new CDFSetListRecord(NULL, CString("��Ʈw���A��Port"), theApp.m_uServerPort));
			parent->m_SetListGrid.AddRecord(new CDFSetListRecord(NULL, CString("�ʱ����A��IP"), CString(theApp.m_chZabbixIP)));
			parent->m_SetListGrid.AddRecord(new CDFSetListRecord(NULL, CString("�ʱ����A��Port"), theApp.m_uZabbixPort));
			parent->m_SetListGrid.AddRecord(new CDFSetListRecord(NULL, CString("�ʱ����A��Host"), CString(theApp.m_chZabbixHost)));
			parent->m_SetListGrid.AddRecord(new CDFSetListRecord(NULL, CString("�ʱ����A��Key"), CString(theApp.m_chZabbixKey)));
			parent->m_SetListGrid.AddRecord(new CDFSetListRecord(NULL, CString("�ʱ��W�v"), theApp.m_uZabbixFreq));
			parent->m_SetListGrid.GetColumns()->GetAt(1)->SetEditable(TRUE);
			parent->m_SetListGrid.Populate();
			break;
		}
	case 2: // �A�ȳ]�w
		{
			parent->m_SetListGrid.GetRecords()->RemoveAll();
			parent->m_SetListGrid.AddRecord(new CDFSetListRecord(NULL, CString("�w�]�����ɮ׶ǿ�"), CString(theApp.m_bNetDefaultTransferAll ? "TRUE" : "FALSE")));
			parent->m_SetListGrid.AddRecord(new CDFSetListRecord(NULL, CString("�w�]�����ɮ����Y"), CString(theApp.m_bNetDefaultCompressAll ? "TRUE" : "FALSE")));
			parent->m_SetListGrid.GetColumns()->GetAt(1)->SetEditable(TRUE);
			parent->m_SetListGrid.Populate();
			break;
		}
	case 3: // �����]�w
		{
			parent->m_SetListGrid.GetRecords()->RemoveAll();
			parent->m_SetListGrid.AddRecord(new CDFSetListRecord(NULL, CString("�O��������T"), CString(theApp.m_bLogAllowDebug ? "TRUE" : "FALSE")));
			parent->m_SetListGrid.GetColumns()->GetAt(1)->SetEditable(TRUE);
			parent->m_SetListGrid.Populate();
			break;
		}
	default:
		{
			break;
		}
	}
}

/*afx_msg */void CTabDlg3::OnChangeSettings(NMHDR* pNotifyStruct, LRESULT*)
{
	XTP_NM_REPORTRECORDITEM* pItemNotify = (XTP_NM_REPORTRECORDITEM*)pNotifyStruct;
	if(!pItemNotify) return;
	
	if(pItemNotify->pItem == NULL || pItemNotify->pItem->GetRecord() == NULL) return;

	CMtkFileServerDlg* parent = (CMtkFileServerDlg*)theApp.m_pMainWnd;
	int menu = parent->m_SetMenuGrid.GetSelectedRows()->GetAt(0)->GetIndex();
	int select = parent->m_SetListGrid.GetSelectedRows()->GetAt(0)->GetIndex();

	CString caption = pItemNotify->pItem->GetCaption();
	int count = parent->m_SetListGrid.GetRecords()->GetCount();

	if ((menu==1) && (select==0) && count>0) // �ɮץؿ�
	{
		parent->m_SetListGrid.GetRecords()->GetAt(0)->GetItem(1)->SetCaption(caption);
		memset(theApp.m_chFileDir, NULL, 256);
		memcpy(theApp.m_chFileDir, caption, caption.GetLength());
	}
	else if ((menu==1) && (select==1) && count>1) // ��Ʈw���A��IP
	{
		parent->m_SetListGrid.GetRecords()->GetAt(1)->GetItem(1)->SetCaption(caption);
		memset(theApp.m_chServerIP, NULL, 16);
		memcpy(theApp.m_chServerIP, caption, caption.GetLength());
	}
	else if ((menu==1) && (select==2) && count>2) // ��Ʈw���A��Port
	{
		parent->m_SetListGrid.GetRecords()->GetAt(2)->GetItem(1)->SetCaption(caption);
		theApp.m_uServerPort = atoi(caption);
	}
	else if ((menu==1) && (select==3) && count>3) // �ʱ����A��IP
	{
		parent->m_SetListGrid.GetRecords()->GetAt(3)->GetItem(1)->SetCaption(caption);	
		memset(theApp.m_chZabbixIP, NULL, 16);
		memcpy(theApp.m_chZabbixIP, caption, caption.GetLength());
	}
	else if ((menu==1) && (select==4) && count>4) // �ʱ����A��Port
	{
		parent->m_SetListGrid.GetRecords()->GetAt(4)->GetItem(1)->SetCaption(caption);
		theApp.m_uZabbixPort = atoi(caption);
	}
	else if ((menu==1) && (select==5) && count>5) // �ʱ����A��Host
	{
		parent->m_SetListGrid.GetRecords()->GetAt(5)->GetItem(1)->SetCaption(caption);
		memset(theApp.m_chZabbixHost, NULL, 50);
		memcpy(theApp.m_chZabbixHost, caption, caption.GetLength());
	}
	else if ((menu==1) && (select==6) && count>6) // �ʱ����A��Key
	{
		parent->m_SetListGrid.GetRecords()->GetAt(6)->GetItem(1)->SetCaption(caption);
		memset(theApp.m_chZabbixKey, NULL, 50);
		memcpy(theApp.m_chZabbixKey, caption, caption.GetLength());
	}
	else if ((menu==1) && (select==7) && count>7) // �ʱ��W�v
	{
		parent->m_SetListGrid.GetRecords()->GetAt(7)->GetItem(1)->SetCaption(caption);
		theApp.m_uZabbixFreq = atoi(caption);
	}
	else if ((menu==2) && (select==0) && count>0) // �w�]�����ɮ׶ǿ�
	{
		parent->m_SetListGrid.GetRecords()->GetAt(0)->GetItem(1)->SetCaption(caption);
		theApp.m_bNetDefaultTransferAll = (caption=="TRUE") ? TRUE : FALSE;
	}
	else if ((menu==2) && (select==1) && count>1) // �w�]�����ɮ����Y
	{
		parent->m_SetListGrid.GetRecords()->GetAt(1)->GetItem(1)->SetCaption(caption);
		theApp.m_bNetDefaultCompressAll = (caption=="TRUE") ? TRUE : FALSE;
	}
	else if ((menu==3) && (select==0) && count>0) // �O��������T
	{
		parent->m_SetListGrid.GetRecords()->GetAt(0)->GetItem(1)->SetCaption(caption);
		theApp.m_bLogAllowDebug = (caption=="TRUE") ? TRUE : FALSE;
	}
	else
	{
		CString strLog("");
		strLog.Format("�_�Ǫ��Ʊ� �S���������]�w���[%s]", caption);
		parent->m_ptDFLog->DFLogSend(strLog, DF_SEVERITY_HIGH);
	}

	OnChangeMenu(NULL, NULL);
}

/*afx_msg */void CTabDlg3::OnOK()
{

}

/*afx_msg */void CTabDlg3::OnCancel()
{

}

/*afx_msg */void CTabDlg3::OnDestroy()
{

}


// CTabDlg4 implementation 
CTabDlg4::CTabDlg4(CWnd* pParent/* = NULL*/)
	: CDialogEx(CTabDlg4::IDD, pParent)
, uFilter1(0)
, uFilter2(0)
, uFilter3(0)
, uFilter4(0)
, uFilter5(0)
{

}

BEGIN_MESSAGE_MAP(CTabDlg4, CDialogEx)
	ON_BN_CLICKED(IDC_LOGTOOLBARBUTTON1, &CTabDlg4::OnClear)
	ON_BN_CLICKED(IDC_LOGTOOLBARFILTER1, &CTabDlg4::OnFilter1)
	ON_BN_CLICKED(IDC_LOGTOOLBARFILTER2, &CTabDlg4::OnFilter2)
	ON_BN_CLICKED(IDC_LOGTOOLBARFILTER3, &CTabDlg4::OnFilter3)
	ON_BN_CLICKED(IDC_LOGTOOLBARFILTER4, &CTabDlg4::OnFilter4)
	ON_BN_CLICKED(IDC_LOGTOOLBARFILTER5, &CTabDlg4::OnFilter5)
END_MESSAGE_MAP()

/*afx_msg */void CTabDlg4::OnClear()
{
	CMtkFileServerDlg* parent = (CMtkFileServerDlg*)theApp.m_pMainWnd;
	parent->m_ptDFLog->ClearAll();
}

/*afx_msg */void CTabDlg4::OnFilter1()
{
	CMtkFileServerDlg* parent = (CMtkFileServerDlg*)theApp.m_pMainWnd;
	uFilter1 = (uFilter1+1) % 2;
	
	if (uFilter1)
		parent->m_ptDFLog->SetFilterOn(1);
	else
		parent->m_ptDFLog->SetFilterOff(1);
}

/*afx_msg */void CTabDlg4::OnFilter2()
{
	CMtkFileServerDlg* parent = (CMtkFileServerDlg*)theApp.m_pMainWnd;
	uFilter2 = (uFilter2+1) % 2;

	if (uFilter2)
		parent->m_ptDFLog->SetFilterOn(2);
	else
		parent->m_ptDFLog->SetFilterOff(2);
}

/*afx_msg */void CTabDlg4::OnFilter3()
{
	CMtkFileServerDlg* parent = (CMtkFileServerDlg*)theApp.m_pMainWnd;
	uFilter3 = (uFilter3+1) % 2;

	if (uFilter3)
		parent->m_ptDFLog->SetFilterOn(4);
	else
		parent->m_ptDFLog->SetFilterOff(4);
}

/*afx_msg */void CTabDlg4::OnFilter4()
{
	CMtkFileServerDlg* parent = (CMtkFileServerDlg*)theApp.m_pMainWnd;
	uFilter4 = (uFilter4+1) % 2;

	if (uFilter4)
		parent->m_ptDFLog->SetFilterOn(8);
	else
		parent->m_ptDFLog->SetFilterOff(8);
}

/*afx_msg */void CTabDlg4::OnFilter5()
{
	CMtkFileServerDlg* parent = (CMtkFileServerDlg*)theApp.m_pMainWnd;
	uFilter5 = (uFilter5+1) % 2;

	if (uFilter5)
		parent->m_ptDFLog->SetFilterOn(16);
	else
		parent->m_ptDFLog->SetFilterOff(16);
}

/*afx_msg */void CTabDlg4::OnOK()
{

}

/*afx_msg */void CTabDlg4::OnCancel()
{

}

/*afx_msg */void CTabDlg4::OnDestroy()
{

}


// class CDFNetSlaveRecord implementation
CDFNetSlaveRecord::CDFNetSlaveRecord(CMtkFileServerDlg* master)
	:m_SetMaster(master)
{
	CreateItems();
}
	
/*virtual */CDFNetSlaveRecord::~CDFNetSlaveRecord(void)
{

}

/*virtual */void CDFNetSlaveRecord::CreateItems()
{
	AddItem(new CXTPReportRecordItemText());
	AddItem(new CXTPReportRecordItemText());
	AddItem(new CXTPReportRecordItemText());
	AddItem(new CXTPReportRecordItemText());
	AddItem(new CXTPReportRecordItemText());
	AddItem(new CXTPReportRecordItemText());
	AddItem(new CXTPReportRecordItemText());
	AddItem(new CXTPReportRecordItemText());
	AddItem(new CXTPReportRecordItemText());
	AddItem(new CXTPReportRecordItemText());
}

/*virtual */void CDFNetSlaveRecord::GetItemMetrics(XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics)
{
	CString strColumn = pDrawArgs->pColumn->GetCaption();
	int nIndexCol = pDrawArgs->pColumn->GetItemIndex();
	int nIndexRow = pDrawArgs->pRow->GetIndex();
	int nCount = pDrawArgs->pControl->GetRows()->GetCount();

	AcquireSRWLockExclusive(&m_SetMaster->m_NetSlaveLock);

	//if (m_SetMaster && m_SetMaster->m_ptDFNet)
	if (m_SetMaster->m_NetSlaveList && (m_SetMaster->m_NetSlaveCount>=(UINT)nCount))
	{
		AcquireSRWLockExclusive(&m_SetMaster->m_ptDFNet->m_lockMaster);

		CString key(m_SetMaster->m_NetSlaveList[nIndexRow].m_key);	
		CDFNetSlave* slave = (CDFNetSlave*)m_SetMaster->m_ptDFNet->m_mapSlaveList[key];

		if (slave == NULL)
		{
			pItemMetrics->strText.Format("%s", "---");
			ReleaseSRWLockExclusive(&m_SetMaster->m_ptDFNet->m_lockMaster);
			ReleaseSRWLockExclusive(&m_SetMaster->m_NetSlaveLock);
			return;
		}

		switch (nIndexCol)
		{
		case 0: // �s��
			{
				pItemMetrics->strText.Format("%03d", nIndexRow);
				break;
			}
		case 1: // �D���W��
			{
				pItemMetrics->nItemIcon = ((GetTickCount()-slave->m_lastsec)>=10000) ? 1 : 0;
				if (slave->m_name[0] == 0) 
					pItemMetrics->strText.Format("%s", slave->m_host);
				else
					pItemMetrics->strText.Format("%s(%s)", slave->m_host, slave->m_name);
				break;
			}
		case 2: // ��
			{
				pItemMetrics->strText.Format("%d", slave->m_port);
				break;
			}
		case 3: // �����
			{
				pItemMetrics->strText.Format("%d", slave->m_nThreadID);
				break;
			}
		case 4: // ����
			{
				pItemMetrics->strText.Format("%d", slave->m_age);
				break;
			}
		case 5: // �ɮ�
			{
				// LOCK(?)
				pItemMetrics->strText.Format("%d", slave->m_mapWork.GetCount());
				break;
			}
		case 6: // �̫�ǿ�ɶ�
			{
				pItemMetrics->strText.Format("%04d-%02d-%02d %02d:%02d:%02d", slave->m_currenttime.wYear, slave->m_currenttime.wMonth, slave->m_currenttime.wDay, slave->m_currenttime.wHour, slave->m_currenttime.wMinute, slave->m_currenttime.wSecond);
				break;
			}
		case 7: // ���y�q
			{
				pItemMetrics->strText.Format("%d(�̤j:%d)", slave->m_flow, slave->m_maxflow);
				break;
			}
		case 8: // �y�q
			{
				if (slave->m_debug)
					pItemMetrics->strText.Format("�����Ҧ�");
				else
					pItemMetrics->strText.Format("---");
				break;
			}
		case 9: // ���Y�q
			{
				pItemMetrics->strText.Format("%d/%d(%d%%)", slave->m_comvolumn, slave->m_realvolumn, slave->m_realvolumn<=0 ? 0 : (slave->m_comvolumn*100)/slave->m_realvolumn);
				break;
			}
		}

		ReleaseSRWLockExclusive(&m_SetMaster->m_ptDFNet->m_lockMaster);
	}

	ReleaseSRWLockExclusive(&m_SetMaster->m_NetSlaveLock);
}


// class CDFSetMenuRecord implementation
CDFSetMenuRecord::CDFSetMenuRecord(CMtkFileServerDlg* master, CString& text)
{
	AddItem(new CXTPReportRecordItemText(text));
}

/*virtual */CDFSetMenuRecord::~CDFSetMenuRecord(void)
{

}

/*virtual */void CDFSetMenuRecord::CreateItems()
{

}

/*virtual */void CDFSetMenuRecord::GetItemMetrics(XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics)
{
	CXTPReportRecord::GetItemMetrics(pDrawArgs, pItemMetrics);
}


// class CDFSetListRecord implementation
CDFSetListRecord::CDFSetListRecord()
{
	AddItem(new CXTPReportRecordItemText());
	AddItem(new CXTPReportRecordItemText());
}

CDFSetListRecord::CDFSetListRecord(CMtkFileServerDlg* master, CString& text, CString& data)
{
	AddItem(new CXTPReportRecordItemText(text));
	AddItem(new CXTPReportRecordItemText(data));
}

CDFSetListRecord::CDFSetListRecord(CMtkFileServerDlg* master, CString& text, UINT data)
{
	AddItem(new CXTPReportRecordItemText(text));

	CString str("");
	str.Format("%d", data);
	AddItem(new CXTPReportRecordItemText(str));
}

/*virtual */CDFSetListRecord::~CDFSetListRecord(void)
{

}

/*virtual */void CDFSetListRecord::CreateItems()
{

}

/*virtual */void CDFSetListRecord::GetItemMetrics(XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics)
{
	CXTPReportRecord::GetItemMetrics(pDrawArgs, pItemMetrics);
}

BOOL CDFSetListRecord::UpdateRecord(int idx, CString& data)
{
	if (idx < this->GetItemCount()) 
	{
		((CXTPReportRecordItemText*)GetItem(idx))->SetValue(data);
		return TRUE;
	}

	return FALSE;
}

BOOL CDFSetListRecord::UpdateRecord(int idx, UINT data)
{
	if (idx < this->GetItemCount()) 
	{
		CString strData("");
		strData.Format("%d", data);

		((CXTPReportRecordItemText*)GetItem(idx))->SetValue(strData);
		return TRUE;
	}

	return FALSE;
}


// The Main Dialog class CMtkFileServerDlg implementation
CMtkFileServerDlg::CMtkFileServerDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMtkFileServerDlg::IDD, pParent)
, m_uServerStatus(0)
//, m_uListenPort(0)
//, m_uServerPort(0)
//, m_uZabbixPort(0)
//, m_uZabbixFreq(0)
//, m_bNetDefaultTransferAll(FALSE)
//, m_bNetDefaultCompressAll(FALSE)
//, m_bLogAllowDebug(FALSE)
, m_NetSlaveCount(0)
, m_NetSlaveList(NULL)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON3);
	//  IDC_FILEFILTEREDIT = 0;
//	memset(m_chFileDir, 0, 256);
//	memset(m_chServerIP, 0, 16);
//	memset(m_chZabbixIP, 0, 16);
//	memset(m_chZabbixHost, 0, 50);
//	memset(m_chZabbixKey, 0, 50);

	InitializeSRWLock(&m_NetSlaveLock);
}

void CMtkFileServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB1, m_tabctrlMain);
}

BEGIN_MESSAGE_MAP(CMtkFileServerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_WM_QUERYDRAGICON()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, &CMtkFileServerDlg::OnTcnSelchangeTab1)
END_MESSAGE_MAP()


// CMtkFileServerDlg �T���B�z�`��

BOOL CMtkFileServerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// �N [����...] �\���[�J�t�Υ\���C

	// IDM_ABOUTBOX �����b�t�ΩR�O�d�򤧤��C
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// �]�w����ܤ�����ϥܡC�����ε{�����D�������O��ܤ���ɡA
	// �ج[�|�۰ʱq�Ʀ��@�~
	SetIcon(m_hIcon, TRUE);			// �]�w�j�ϥ�
	SetIcon(m_hIcon, FALSE);		// �]�w�p�ϥ�

	// TODO: �b���[�J�B�~����l�]�w

	// INI
	InitApp();
	SetWindowText(theApp.m_strTitle);

	WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    wVersionRequested = MAKEWORD(2, 2);
	if ((err = WSAStartup(wVersionRequested, &wsaData)) != 0)
		return FALSE;
	

	m_Font.CreateFont(
		16,							// nHeight(Min 8)
		0,							// nWidth(min 4)
		0,							// nEscapement
		0,							// nOrientation
		FW_NORMAL,					// nWeight
		FALSE,						// bItalic
		FALSE,						// bUnderline
		0,							// cStrikeOut
		ANSI_CHARSET,				// nCharSet
		OUT_DEFAULT_PRECIS,			// nOutPrecision
		CLIP_DEFAULT_PRECIS,		// nClipPrecision
		DEFAULT_QUALITY,			// nQuality
		DEFAULT_PITCH | FF_SWISS,	// nPitchAndFamily
		//"Consolas");				// lpszFacename
		//"Courier New");			// lpszFacename
		//"Arial");					// lpszFacename
		"�L�n������");				// lpszFacename

	CRect rect;
	CBitmap bmp;

	// 1. [tab Control]
	// 1.1. ��������
	m_MainTabImageList.Create(16, 16, ILC_COLOR24 | ILC_MASK, 1, 1);
	UINT arTabImg[] = {IDB_BITMAP19, IDB_BITMAP20, IDB_BITMAP21, IDB_BITMAP22};
	for (int i=0; i<4; i++)
	{
		bmp.LoadBitmap(arTabImg[i]);
		m_MainTabImageList.Add(&bmp, RGB(255, 255, 255));
		bmp.DeleteObject();
	}
	
	m_tabctrlMain.SetImageList(&m_MainTabImageList);
	m_tabctrlMain.InsertItem(0, _T("�ɮ׺޲z"), 0);
	m_tabctrlMain.InsertItem(1, _T("�A�Ⱥ޲z"), 1);
	m_tabctrlMain.InsertItem(2, _T("�t�γ]�w"), 2);
	m_tabctrlMain.InsertItem(3, _T("�t�ά���"), 3);

	// 1.2. �إߤ������e
	m_dlgTab1.Create(IDD_DIALOG1, &m_tabctrlMain);
	m_dlgTab2.Create(IDD_DIALOG2, &m_tabctrlMain);
	m_dlgTab3.Create(IDD_DIALOG3, &m_tabctrlMain);
	m_dlgTab4.Create(IDD_DIALOG4, &m_tabctrlMain);
	
	// 2. [tab1] File
	m_dlgTab1.GetClientRect(&rect);
	
	// 2.1. toolbar1
	m_FileToolBarImageList.Create(16, 16, ILC_COLOR16 | ILC_MASK, 1, 1);
	UINT arTool1Img[] = {IDB_BITMAP23, IDB_BITMAP29};
	UINT arTool1Btn[] = {IDC_FILETOOLBARBUTTON1, IDC_FILETOOLBARBUTTON2, IDC_FILETOOLBARBUTTON3, IDC_FILETOOLBARBUTTON4, IDC_FILETOOLBARBUTTON5};
	for (int i=0; i<2; i++)
	{
		bmp.LoadBitmap(arTool1Img[i]);
		m_FileToolBarImageList.Add(&bmp, RGB(255, 255, 255));
		bmp.DeleteObject();
	}
	m_FileToolBar1.CreateEx(&m_dlgTab1, TBSTYLE_FLAT | TBSTYLE_LIST, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, CRect(0, 0, 0, 0));
	m_FileToolBar1.SetButtons(arTool1Btn, 3);
	m_FileToolBar1.SetSizes(CSize(24, 24), CSize(16, 16));
	m_FileToolBar1.GetToolBarCtrl().SetImageList(&m_FileToolBarImageList);
	m_FileToolBar1.MoveWindow(0, 0, rect.Width(), 24, 1);
	
	m_FileToolBar1.SetButtonStyle(0, TBBS_AUTOSIZE);
	m_FileToolBar1.SetButtonText(0, _T("�ɮװ�Ū"));
	
	m_FileToolBar1.SetButtonStyle(1, TBBS_AUTOSIZE);
	m_FileToolBar1.SetButtonText(1, _T("���s��z"));

//	m_FileToolBar1.SetButtonInfo(2, IDC_FILEVERSIONSTATIC, TBBS_SEPARATOR, -1);
	m_FileVersionText.Create(_T("�{������ ")+theApp.m_strVersion, WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE, CRect(rect.Width()-160, 0, rect.Width(), 24), &m_FileToolBar1, IDC_FILEVERSIONSTATIC);
	m_FileVersionText.SetFont(&m_Font);

	//m_FileToolBar1.SetButtonStyle(2, TBBS_SEPARATOR);
	
	// 2.2. toolbar2
	UINT arTool2Btn[] = {IDC_FILEFILTERSTATIC, IDC_FILEFILTEREDIT, IDC_FILEFILTERCLEAR};
	m_FileToolBar2.CreateEx(&m_dlgTab1, TBSTYLE_FLAT | TBSTYLE_LIST, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, CRect(0, 0, 0, 0));
	m_FileToolBar2.SetButtons(arTool2Btn, 3);
	m_FileToolBar2.SetSizes(CSize(24, 24), CSize(16, 16));
	m_FileToolBar2.MoveWindow(0, 25, rect.Width(), 24, 1);	

	m_FileToolBar2.SetButtonInfo(0, IDC_FILEFILTERSTATIC, TBBS_SEPARATOR, -1);
	m_FileFilterText.Create(_T("�ɮ׹L�o"), WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE, CRect(0, 0, 60, 24), &m_FileToolBar2, IDC_FILEFILTERSTATIC);
	m_FileFilterText.SetFont(&m_Font);
	
	bmp.LoadBitmap(IDB_BITMAP25);
	m_FileFilterText.SetBitmap(bmp);
	bmp.DeleteObject();

	m_FileToolBar2.SetButtonInfo(1, IDC_FILEFILTEREDIT, TBBS_SEPARATOR, -1);
	m_FileFilterEdit.Create(WS_CHILD | WS_VISIBLE | WS_BORDER, CRect(62, 0+1, 262, 24-3), &m_FileToolBar2, IDC_FILEFILTEREDIT);
	m_FileFilterEdit.SetFont(&m_Font);

	m_FileToolBar2.SetButtonInfo(2, IDC_FILEFILTERCLEAR, TBBS_SEPARATOR, -1);
	m_FileFilterClearButton.Create(_T("�M��"), WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE, CRect(264, 0+1, 296, 24-3), &m_FileToolBar2, IDC_FILEFILTERCLEAR);
	m_FileFilterClearButton.SetFont(&m_Font);

	// 2.3. grid
	m_FileGridImageList.Create(16, 16, ILC_COLOR24 | ILC_MASK, 1, 1);

	bmp.LoadBitmap(IDB_BITMAP17);
	m_FileGridImageList.Add(&bmp, RGB(0, 0, 0));
	bmp.DeleteObject();

	bmp.LoadBitmap(IDB_BITMAP18);
	m_FileGridImageList.Add(&bmp, RGB(0, 0, 0));
	bmp.DeleteObject();

	bmp.LoadBitmap(IDB_BITMAP38);
	m_FileGridImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	bmp.LoadBitmap(IDB_BITMAP7);
	m_FileGridImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	// style
	// -xtpReportColumnShaded,      // Columns are gray shaded.
	// -xtpReportColumnFlat,        // Flat style for drawing columns.
	// -xtpReportColumnExplorer,    // Explorer column style
	// -xtpReportColumnOffice2003,  // Gradient column style
	// -xtpReportColumnResource

	m_FileGrid.Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_OVERLAPPED | WS_TABSTOP | WS_BORDER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, CRect(0, 50, rect.Width()-5, rect.Height()-50-35), &m_dlgTab1, IDC_FILEGRID, NULL);
	m_FileGrid.GetPaintManager()->SetColumnStyle(xtpReportColumnFlat);
	m_FileGrid.GetPaintManager()->SetGridStyle(FALSE, xtpReportGridNoLines);
	m_FileGrid.GetReportHeader()->AllowColumnRemove(FALSE);
	
	m_FileGrid.SetImageList(&m_FileGridImageList);
	m_FileGrid.SetFont(&m_Font);
	
	// total:550
	m_FileGrid.AddColumn(new CXTPReportColumn(0, _T("�s��"), 40, 0, -1, 0));
	m_FileGrid.AddColumn(new CXTPReportColumn(1, _T("�ɮצW��"), 105));
	m_FileGrid.AddColumn(new CXTPReportColumn(2, _T("MTK"), 70));
	m_FileGrid.AddColumn(new CXTPReportColumn(3, _T("�̫��s�ɶ�"), 135, 0));
	m_FileGrid.AddColumn(new CXTPReportColumn(4, _T("�ɮפj�p"), 60));
	m_FileGrid.AddColumn(new CXTPReportColumn(5, _T("���m�ɶ�"), 30));
	m_FileGrid.AddColumn(new CXTPReportColumn(6, _T("�y�q"), 80, 0, -1, 0));
	m_FileGrid.AddColumn(new CXTPReportColumn(7, _T("�q�\"), 30));

	m_FileGrid.GetColumns()->GetAt(4)->SetAlignment(DT_RIGHT);
	m_FileGrid.GetColumns()->GetAt(5)->SetAlignment(DT_RIGHT);
	m_FileGrid.GetColumns()->GetAt(6)->SetAlignment(DT_RIGHT);
	m_FileGrid.GetColumns()->GetAt(7)->SetAlignment(DT_RIGHT);

	// 3. [tab2] Net
	m_dlgTab2.GetClientRect(&rect);

	// 3.1. toolbar
	UINT arTool3Btn[] = {IDC_NETTOOLBARBUTTON1, IDC_NETTOOLBARBUTTON2, 0, IDC_NETTOOLBARBUTTON3, IDC_NETTOOLBARBUTTON4};
	m_NetToolBarImageList.Create(16, 16, ILC_COLOR16 | ILC_MASK, 1, 1);

	bmp.LoadBitmap(IDB_BITMAP28);
	m_NetToolBarImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	bmp.LoadBitmap(IDB_BITMAP9);
	m_NetToolBarImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	bmp.LoadBitmap(IDB_BITMAP24);
	m_NetToolBarImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	bmp.LoadBitmap(IDB_BITMAP11);
	m_NetToolBarImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	m_NetToolBar.CreateEx(&m_dlgTab2, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, CRect(0, 0, 0, 0));
	m_NetToolBar.SetButtons(arTool3Btn, 5);
	m_NetToolBar.SetSizes(CSize(24, 24), CSize(16, 16));
	m_NetToolBar.GetToolBarCtrl().SetImageList(&m_NetToolBarImageList);
	m_NetToolBar.MoveWindow(0, 0, rect.Width(), 40, 1);
	
	m_NetToolBar.SetButtonStyle(0, TBBS_AUTOSIZE);
	m_NetToolBar.SetButtonText(0, _T("�Ұ�"));
	
	m_NetToolBar.SetButtonStyle(1, TBBS_AUTOSIZE);
	m_NetToolBar.SetButtonText(1, _T("����"));

	m_NetToolBar.SetButtonStyle(2, TBBS_SEPARATOR);

	m_NetToolBar.SetButtonStyle(3, TBBS_AUTOSIZE);
	m_NetToolBar.SetButtonText(3, _T("�_�u"));

	m_NetToolBar.SetButtonStyle(4, TBBS_AUTOSIZE);
	m_NetToolBar.SetButtonText(4, _T("����"));

	// 3.2. grid2
	m_NetListImageList.Create(16, 16, ILC_COLOR16 | ILC_MASK, 1, 1);
	
	bmp.LoadBitmap(IDB_BITMAP30);
	m_NetListImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	bmp.LoadBitmap(IDB_BITMAP32);
	m_NetListImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	m_NetListGrid.Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_OVERLAPPED | WS_TABSTOP | WS_BORDER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, CRect(202, 41, rect.Width()-5, rect.Height()-50-35), &m_dlgTab2, IDC_NETLISTGRID, NULL);
	m_NetListGrid.GetPaintManager()->SetColumnStyle(xtpReportColumnFlat);
	m_NetListGrid.GetPaintManager()->SetHeaderHeight(20);
	m_NetListGrid.GetReportHeader()->AllowColumnRemove(FALSE);

	m_NetListGrid.SetImageList(&m_NetListImageList);
	//m_NetListGrid.SetFont(&m_Font);

	m_NetListGrid.AddColumn(new CXTPReportColumn(0, _T("�s��"), 40, 0, -1, 0));
	m_NetListGrid.AddColumn(new CXTPReportColumn(1, _T("�D���W��"), 80, 1));
	m_NetListGrid.AddColumn(new CXTPReportColumn(2, _T("��"), 55, 0, -1, 0));
	m_NetListGrid.AddColumn(new CXTPReportColumn(3, _T("�����"), 55, 0, -1, 0));
	m_NetListGrid.AddColumn(new CXTPReportColumn(4, _T("����"), 40, 0, -1, 0));
	m_NetListGrid.AddColumn(new CXTPReportColumn(5, _T("�ɮ�"), 40, 0, -1, 0));
	m_NetListGrid.AddColumn(new CXTPReportColumn(6, _T("�̫�ǿ�ɶ�"), 135, 0, -1, 0));
	m_NetListGrid.AddColumn(new CXTPReportColumn(7, _T("���y�q"), 120, 1, -1, 0));
	m_NetListGrid.AddColumn(new CXTPReportColumn(8, _T("�y�q"), 80, 0, -1, 0));
	m_NetListGrid.AddColumn(new CXTPReportColumn(9, _T("���Y�q"), 100, 1, -1, 0));
	
	m_NetListGrid.GetColumns()->GetAt(7)->SetAlignment(DT_RIGHT);
	m_NetListGrid.GetColumns()->GetAt(8)->SetAlignment(DT_RIGHT);
	m_NetListGrid.GetColumns()->GetAt(9)->SetAlignment(DT_RIGHT);

//	m_NetListGrid.AllowEdit(TRUE);
//	m_NetListGrid.GetColumns()->GetAt(0)->SetEditable(FALSE);
//	m_NetListGrid.GetColumns()->GetAt(1)->SetEditable(TRUE);
	m_NetListGrid.Populate();

	// 3.3. grid1
	m_NetMenuImageList.Create(16, 16, ILC_COLOR16 | ILC_MASK, 1, 1);
	
	bmp.LoadBitmap(IDB_BITMAP33);
	m_NetMenuImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	bmp.LoadBitmap(IDB_BITMAP34);
	m_NetMenuImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	m_NetMenuGrid.Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_OVERLAPPED | WS_TABSTOP | WS_BORDER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, CRect(0, 41, /*rect.Width()-5*/ 200, 144/*rect.Height()-14-41*/), &m_dlgTab2, IDC_NETMENUGRID, NULL);
	m_NetMenuGrid.GetPaintManager()->SetColumnStyle(xtpReportColumnFlat);
	m_NetMenuGrid.GetPaintManager()->SetHeaderHeight(20);
	m_NetMenuGrid.GetReportHeader()->AllowColumnRemove(FALSE);
	m_NetMenuGrid.SetImageList(&m_NetMenuImageList);

	m_NetMenuGrid.AddColumn(new CXTPReportColumn(0, _T(""), 80, 0, -1, 0));
	m_NetMenuGrid.AddColumn(new CXTPReportColumn(1, _T(""), 100, 1, -1, 0));
	//m_NetMenuGrid.GetColumns()->GetAt(0)->SetAlignment(DT_CENTER);
	
	m_NetMenuGrid.AddRecord(new CDFSetListRecord(this, CString("�t�Ϊ��A"), CString("")));
	m_NetMenuGrid.AddRecord(new CDFSetListRecord(this, CString("��ť��"), CString("")));
	m_NetMenuGrid.AddRecord(new CDFSetListRecord(this, CString("�ɮ׼ƶq"), CString("")));
	m_NetMenuGrid.AddRecord(new CDFSetListRecord(this, CString("�s�u�ƶq"), CString("")));

	//m_NetMenuGrid.GetRecords()->GetAt(0)->GetItem(1)->SetIconIndex(0);

	m_NetMenuGrid.Populate();

	// 3.4. grid3
	m_NetFileImageList.Create(16, 16, ILC_COLOR16 | ILC_MASK, 1, 1);

	m_NetFileGrid.Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_OVERLAPPED | WS_TABSTOP | WS_BORDER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, CRect(0, 146, /*rect.Width()-5*/ 200, rect.Height()-50-35), &m_dlgTab2, IDC_NETFILEGRID, NULL);
	m_NetFileGrid.GetPaintManager()->SetColumnStyle(xtpReportColumnFlat);
	m_NetFileGrid.GetPaintManager()->SetHeaderHeight(20);
	m_NetFileGrid.GetReportHeader()->AllowColumnRemove(FALSE);

	m_NetFileGrid.AddColumn(new CXTPReportColumn(0, _T(""), 200, 1, -1, 0));
	//m_NetMenuGrid.GetColumns()->GetAt(0)->SetAlignment(DT_CENTER);
	m_NetFileGrid.Populate();

	// 4. [tab3] Set
	m_dlgTab3.GetClientRect(&rect);

	// 4.1. toolbar
	UINT arTool4Btn[] = {IDC_SETTOOLBARBUTTON1, IDC_SETTOOLBARBUTTON2, 0, IDC_SETTOOLBARBUTTON3};
	m_SetToolBarImageList.Create(16, 16, ILC_COLOR16 | ILC_MASK, 1, 1);
	
	bmp.LoadBitmap(IDB_BITMAP28);
	m_SetToolBarImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	bmp.LoadBitmap(IDB_BITMAP9);
	m_SetToolBarImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	bmp.LoadBitmap(IDB_BITMAP6);
	m_SetToolBarImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();
	
	bmp.LoadBitmap(IDB_BITMAP6);
	m_SetToolBarImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	m_SetToolBar.CreateEx(&m_dlgTab3, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, CRect(0, 0, 0, 0));
	m_SetToolBar.SetButtons(arTool4Btn, 4);
	m_SetToolBar.SetSizes(CSize(24, 24), CSize(16, 16));
	m_SetToolBar.MoveWindow(0, 0, rect.Width(), 40, 1);
	m_SetToolBar.GetToolBarCtrl().SetImageList(&m_SetToolBarImageList);

	m_SetToolBar.SetButtonStyle(0, TBBS_AUTOSIZE);
	m_SetToolBar.SetButtonText(0, _T("�Ұ�"));
	
	m_SetToolBar.SetButtonStyle(1, TBBS_AUTOSIZE);
	m_SetToolBar.SetButtonText(1, _T("����"));

	m_SetToolBar.SetButtonStyle(2, TBBS_SEPARATOR);

	m_SetToolBar.SetButtonStyle(3, TBBS_AUTOSIZE);
	m_SetToolBar.SetButtonText(3, _T("�x�s"));

	// 4.2. grid2
	m_SetListImageList.Create(16, 16, ILC_COLOR16 | ILC_MASK, 1, 1);

	bmp.LoadBitmap(IDB_BITMAP33);
	m_SetListImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	bmp.LoadBitmap(IDB_BITMAP34);
	m_SetListImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	m_SetListGrid.Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_OVERLAPPED | WS_TABSTOP | WS_BORDER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, CRect(102, 41, rect.Width()-5, rect.Height()-50-35), &m_dlgTab3, IDC_SETLISTGRID, NULL);
	m_SetListGrid.GetPaintManager()->SetColumnStyle(xtpReportColumnFlat);
	m_SetListGrid.GetPaintManager()->SetHeaderHeight(20);
	m_SetListGrid.GetReportHeader()->AllowColumnRemove(FALSE);
	m_SetListGrid.SetImageList(&m_SetListImageList);

	m_SetListGrid.AddColumn(new CXTPReportColumn(0, _T(""), 200, 1, -1, 0));
	m_SetListGrid.AddColumn(new CXTPReportColumn(1, _T(""), 400, 1, -1, 0));
	
	m_SetListGrid.AllowEdit(TRUE);
	m_SetListGrid.GetColumns()->GetAt(0)->SetEditable(FALSE);
	m_SetListGrid.GetColumns()->GetAt(1)->SetEditable(TRUE);
	m_SetListGrid.Populate();
	
	// 4.3. grid1
	m_SetMenuImageList.Create(16, 16, ILC_COLOR16 | ILC_MASK, 1, 1);
	
	m_SetMenuGrid.Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_OVERLAPPED | WS_TABSTOP | WS_BORDER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, CRect(0, 41, /*rect.Width()-5*/ 100, rect.Height()-50-35), &m_dlgTab3, IDC_SETMENUGRID, NULL);
	m_SetMenuGrid.GetPaintManager()->SetColumnStyle(xtpReportColumnFlat);
	m_SetMenuGrid.GetPaintManager()->SetHeaderHeight(20);
	m_SetMenuGrid.GetReportHeader()->AllowColumnRemove(FALSE);

	m_SetMenuGrid.AddColumn(new CXTPReportColumn(0, _T(""), 100, 1, -1, 0));
	m_SetMenuGrid.GetColumns()->GetAt(0)->SetAlignment(DT_CENTER);
	
	m_SetMenuGrid.AddRecord(new CDFSetMenuRecord(this, CString("�t �� �� �A")));
	m_SetMenuGrid.AddRecord(new CDFSetMenuRecord(this, CString("�t �� �] �w")));
	m_SetMenuGrid.AddRecord(new CDFSetMenuRecord(this, CString("�A �� �] �w")));
	m_SetMenuGrid.AddRecord(new CDFSetMenuRecord(this, CString("�� �� �] �w")));
	m_SetMenuGrid.Populate();
	
	// 5. [tab4] Log
	// 5.1. toolbar
	m_LogToolBarImageList.Create(16, 16, ILC_COLOR16 | ILC_MASK, 1, 1);
	UINT arLogToolbarBtn[] = {IDC_LOGTOOLBARBUTTON1, 0, IDC_LOGTOOLBARFILTER1, IDC_LOGTOOLBARFILTER2, IDC_LOGTOOLBARFILTER3, IDC_LOGTOOLBARFILTER4, IDC_LOGTOOLBARFILTER5};

	bmp.LoadBitmap(IDB_BITMAP12);
	m_LogToolBarImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	//bmp.LoadBitmap(IDB_BITMAP15);
	//m_LogToolBarImageList.Add(&bmp, RGB(255, 255, 255));
	//bmp.DeleteObject();

	bmp.LoadBitmap(IDB_BITMAP35);
	m_LogToolBarImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	bmp.LoadBitmap(IDB_BITMAP36);
	m_LogToolBarImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	bmp.LoadBitmap(IDB_BITMAP16);
	m_LogToolBarImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	bmp.LoadBitmap(IDB_BITMAP37);
	m_LogToolBarImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	bmp.LoadBitmap(IDB_BITMAP37);
	m_LogToolBarImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	m_LogToolBar.CreateEx(&m_dlgTab4, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, CRect(0, 0, 0, 0));
	m_LogToolBar.SetButtons(arLogToolbarBtn, 7);
	m_LogToolBar.SetSizes(CSize(24, 24), CSize(16, 16));
	m_LogToolBar.GetToolBarCtrl().SetImageList(&m_LogToolBarImageList);
	m_LogToolBar.LoadToolBar(IDC_LOGTOOLBAR);
	
	m_LogToolBar.SetButtonStyle(0, TBBS_AUTOSIZE);
	m_LogToolBar.SetButtonText(0, _T("�M��"));

	m_LogToolBar.SetButtonStyle(1, TBBS_SEPARATOR);

	m_LogToolBar.SetButtonStyle(2, TBBS_AUTOSIZE | BTNS_CHECK);
	m_LogToolBar.SetButtonText(2, _T("��T"));

	m_LogToolBar.SetButtonStyle(3, TBBS_AUTOSIZE | BTNS_CHECK);
	m_LogToolBar.SetButtonText(3, _T("����"));

	m_LogToolBar.SetButtonStyle(4, TBBS_AUTOSIZE | BTNS_CHECK);
	m_LogToolBar.SetButtonText(4, _T("ĵ�i"));

	m_LogToolBar.SetButtonStyle(5, TBBS_AUTOSIZE | BTNS_CHECK);
	m_LogToolBar.SetButtonText(5, _T("���~"));

	m_LogToolBar.SetButtonStyle(6, TBBS_AUTOSIZE | BTNS_CHECK);
	m_LogToolBar.SetButtonText(6, _T("�a��"));

	m_dlgTab4.GetClientRect(&rect);
	m_LogToolBar.MoveWindow(0, 0, rect.Width(), 40, 1);

	// 5.2. grid
	m_LogGridImageList.Create(16, 16, ILC_COLOR16 | ILC_MASK, 1, 1);

	bmp.LoadBitmap(IDB_BITMAP15);
	m_LogGridImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	bmp.LoadBitmap(IDB_BITMAP16);
	m_LogGridImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	bmp.LoadBitmap(IDB_BITMAP35);
	m_LogGridImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	bmp.LoadBitmap(IDB_BITMAP36);
	m_LogGridImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	bmp.LoadBitmap(IDB_BITMAP37);
	m_LogGridImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	// style
	// -xtpReportColumnShaded,      // Columns are gray shaded.
	// -xtpReportColumnFlat,        // Flat style for drawing columns.
	// -xtpReportColumnExplorer,    // Explorer column style
	// -xtpReportColumnOffice2003,  // Gradient column style
	// -xtpReportColumnResource

	m_LogGrid.Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_OVERLAPPED | WS_TABSTOP | WS_BORDER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, CRect(0, 41, rect.Width()-5, rect.Height()-50-35), &m_dlgTab4, IDC_LOGGRID, NULL);
	m_LogGrid.GetPaintManager()->SetColumnStyle(xtpReportColumnFlat);
	m_LogGrid.GetPaintManager()->SetGridStyle(FALSE, xtpReportGridNoLines);
	m_LogGrid.GetReportHeader()->AllowColumnRemove(FALSE);
	m_LogGrid.SetImageList(&m_LogGridImageList);

	// total:550
	m_LogGrid.AddColumn(new CXTPReportColumn(0, _T("�s��"), 50));
	m_LogGrid.AddColumn(new CXTPReportColumn(1, _T("����"), 100));
	m_LogGrid.AddColumn(new CXTPReportColumn(2, _T("���"), 80));
	m_LogGrid.AddColumn(new CXTPReportColumn(3, _T("�ɶ�"), 80));
	m_LogGrid.AddColumn(new CXTPReportColumn(4, _T("�T��"), 340));

	// 4. ��l�����]�w
	m_tabctrlMain.SetCurSel(0);
	OnTcnSelchangeTab1(NULL, NULL);

	// 5. 
	BeginApp();
	m_dlgTab3.RefreshSettingPage();
	SetTimer(IDC_TIMER, 1000, NULL);

	return TRUE;  // �Ǧ^ TRUE�A���D�z�ﱱ��]�w�J�I
}

BOOL CMtkFileServerDlg::InitApp()
{
	theApp.GetIni();

	theApp.GetDict();
/*	char	szIniPath[261]={0}, szFolder[260]={0}, szAppName[256]={0}, szDrive[3]={0};
	CString	strbuf;

	// then try current working dir followed by app folder
	GetModuleFileName(NULL, szIniPath, sizeof(szIniPath)-1);
	_splitpath_s(szIniPath, szDrive, 3, szFolder, 260, szAppName, 256, NULL, 0);
	// chNowDirectory
	GetCurrentDirectory(sizeof(szIniPath)-1, szIniPath);

	// SYSTEM
	_makepath_s(szIniPath, NULL, szIniPath, szAppName, ".ini");

	memset(m_chFileDir, NULL, 256);
	
	GetPrivateProfileString("SYSTEM", "PATH", "D:\\Quote\\mtk\\", m_chFileDir, 256, szIniPath);	
	if(m_chFileDir[strlen(m_chFileDir)-1] != '\\')
		strcat_s(m_chFileDir, "\\");

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
*/
	return TRUE;
}

BOOL CMtkFileServerDlg::BeginApp()
{
	LoadSysLogClientDll();

	// 0:
	InitializeSRWLock(&m_lockServer);

	// 1:�t�ά����޲z��
	m_ptDFLog = new CDFLogMaster(CDFLogMaster::ThreadFunc, this);
	AcquireSRWLockExclusive(&m_lockServer); 
	VERIFY(m_ptDFLog->CreateThread());

	// 2:�ɮ׺޲z��
	m_ptFileThread = new CMtkFileMaster(CMtkFileMaster::ThreadFunc, this);
	AcquireSRWLockExclusive(&m_lockServer); 
	VERIFY(m_ptFileThread->CreateThread());

	// 3.�s�u�޲z��
	m_ptDFNet = new CDFNetMaster(CDFNetMaster::ThreadFunc, this, theApp.m_uListenPort);
	AcquireSRWLockExclusive(&m_lockServer);
	VERIFY(m_ptDFNet->CreateThread());

	return TRUE;
}

void CMtkFileServerDlg::SaveIni()
{
	theApp.SetIni();
/*	WriteProfileString("SYSTEM", "PATH", m_chFileDir);
	theApp.WriteProfileInt("SYSTEM", "LISTEN", m_uListenPort);
	WriteProfileString("SYSTEM", "DBIP", m_chServerIP);
	theApp.WriteProfileInt("SYSTEM", "DBPORT", m_uServerPort);
	WriteProfileString("SYSTEM", "ZABBIXIP", m_chZabbixIP);
	theApp.WriteProfileInt("SYSTEM", "ZABBIXPORT", m_uZabbixPort);
	WriteProfileString("SYSTEM", "ZABBIXHOST", m_chZabbixHost);
	WriteProfileString("SYSTEM", "ZABBIXKEY", m_chZabbixKey);
	theApp.WriteProfileInt("SYSTEM", "ZABBIXFREQ", m_uZabbixFreq);

	theApp.WriteProfileInt("SERVICE", "DEFTRANSMITALL", m_bNetDefaultTransferAll ? 1:0);
	theApp.WriteProfileInt("SERVICE", "DEFCOMPRESSALL", m_bNetDefaultCompressAll ? 1:0);

	theApp.WriteProfileInt("LOG", "LOGDEBUG", m_bLogAllowDebug ? 1:0);
*/
}

void CMtkFileServerDlg::EndApp()
{
	DWORD nThreadID;
	CString strLog("");

	// 1.�T�T �t�γ]�w�޲z��
	// SAVAINI

	// 2.�T�T �s�u�ʱ��p�ɾ�
	KillTimer(IDC_TIMER);
	if (m_NetSlaveList)
	{
		delete m_NetSlaveList;
		m_NetSlaveList = NULL;
	}

	// 3.�T�T �s�u�޲z��
	if (m_ptDFNet)
	{
		nThreadID = m_ptDFNet->m_nThreadID;
		m_ptDFNet->ForceStopMaster();
		m_ptDFNet->StopThread();
		WaitForSingleObject(m_ptDFNet->m_hThread, INFINITE);
		strLog.Format("[�{������]�����s�u�޲z��(Thread:%d)", nThreadID);
		m_ptDFLog->DFLogSend(strLog, DF_SEVERITY_INFORMATION);
		delete m_ptDFNet;
		m_ptDFNet = NULL;
	}

	// ���A�@��
	Sleep(1000);

	// 4.�T�T �ɮ׺޲z��
	if (m_ptFileThread)
	{
		nThreadID = m_ptFileThread->m_nThreadID;
		m_ptFileThread->StopThread();
		WaitForSingleObject(m_ptFileThread->m_hThread, INFINITE);
		strLog.Format("[�{������]�����ɮ׺޲z��(Thread:%d)", nThreadID);
		m_ptDFLog->DFLogSend(strLog, DF_SEVERITY_INFORMATION);
		delete m_ptFileThread;
		m_ptFileThread = NULL;
	}

	// 5.�T�T �����޲z��
	if (m_ptDFLog)
	{
		nThreadID = m_ptDFLog->m_nThreadID;
		m_ptDFLog->StopThread();
		WaitForSingleObject(m_ptDFLog->m_hThread, INFINITE);
		delete m_ptDFLog;
		m_ptDFLog = NULL;
	}
}

void CMtkFileServerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		if ((nID & 0xFFF0) == SC_CLOSE)
		{
			CNotifyDlg dlgNotify;

			dlgNotify.m_strMessage.Format("�T�w�n�����{��?");
			if (dlgNotify.DoModal() == IDOK)
			{
				EndApp();
				CDialog::OnCancel();
			}
		}

		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// �p�G�N�̤p�ƫ��s�[�J�z����ܤ���A�z�ݭn�U�C���{���X�A
// �H�Kø�s�ϥܡC���ϥΤ��/�˵��Ҧ��� MFC ���ε{���A
// �ج[�|�۰ʧ������@�~�C

void CMtkFileServerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ø�s���˸m���e

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// �N�ϥܸm����Τ�ݯx��
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// �yø�ϥ�
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

void CMtkFileServerDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	if(m_tabctrlMain.m_hWnd == NULL)
		return;      // Return if window is not created yet.

	//RECT rect;
	CRect rect;
	GetClientRect(&rect);
	m_tabctrlMain.MoveWindow(&rect, TRUE);
	OnTcnSelchangeTab1(NULL, NULL);
	
	//m_dlgTab1
	m_FileToolBar1.MoveWindow(0, 0, rect.right, 24, 1);
	m_FileToolBar2.MoveWindow(0, 25, rect.right, 24, 1);
	m_FileGrid.MoveWindow(0, 50, rect.right-5, rect.bottom-50-26, 1);

	//m_dlgTab2
	m_NetToolBar.MoveWindow(0, 0, rect.right, 40, 1);
	m_NetListGrid.MoveWindow(202, 41, rect.right-202-5, rect.bottom-41-26, 1);
	m_NetFileGrid.MoveWindow(0, 146, 200, rect.bottom-146-26, 1);
	
	//m_dlgTab3
	m_SetToolBar.MoveWindow(0, 0, rect.right, 40, 1);
	m_SetMenuGrid.MoveWindow(0, 41, 100, rect.bottom-41-26, 1);
	m_SetListGrid.MoveWindow(102, 41, rect.right-102-5, rect.bottom-41-26, 1);
	
	//m_dlgTab4
	m_LogToolBar.MoveWindow(0, 0, rect.right, 40, 1);
	m_LogGrid.MoveWindow(0, 41, rect.right-5, rect.bottom-41-26, 1);

	//OnTcnSelchangeTab1(NULL, NULL);
}

/*afx_msg */void CMtkFileServerDlg::OnOK()
{

}

/*afx_msg */void CMtkFileServerDlg::OnCancel()
{

}

/*afx_msg */void CMtkFileServerDlg::OnDestroy()
{

}


// ��ϥΪ̩즲�̤p�Ƶ����ɡA
// �t�ΩI�s�o�ӥ\����o�����ܡC
HCURSOR CMtkFileServerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CMtkFileServerDlg::OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: �b���[�J����i���B�z�`���{���X
	if (pResult)
		*pResult = 0;

	CRect rTab, rItem;
	m_tabctrlMain.GetItemRect(0, &rItem);
	m_tabctrlMain.GetClientRect(&rTab);
	
	int x = rItem.left;
	int y = rItem.bottom+1;
	int cx = rTab.right-rItem.left-3;
	int cy = rTab.bottom-y-2;
	int tab = m_tabctrlMain.GetCurSel();

	m_dlgTab1.SetWindowPos(NULL, x, y, cx, cy, SWP_HIDEWINDOW);
	m_dlgTab2.SetWindowPos(NULL, x, y, cx, cy, SWP_HIDEWINDOW);
	m_dlgTab3.SetWindowPos(NULL, x, y, cx, cy, SWP_HIDEWINDOW);
	m_dlgTab4.SetWindowPos(NULL, x, y, cx, cy, SWP_HIDEWINDOW);

	switch(tab)
	{
	case 0:
		m_dlgTab1.SetWindowPos(NULL, x, y, cx, cy, SWP_SHOWWINDOW);
		break;
	case 1:
		m_dlgTab2.SetWindowPos(NULL, x, y, cx, cy, SWP_SHOWWINDOW);
		break;
	case 2:
		m_dlgTab3.SetWindowPos(NULL, x, y, cx, cy, SWP_SHOWWINDOW);
		break;
	case 3:
		m_dlgTab4.SetWindowPos(NULL, x, y, cx, cy, SWP_SHOWWINDOW);
		break;
	default:
		break;
	}
}

void CMtkFileServerDlg::OnTimer(UINT_PTR nIDEvent)
{
	AcquireSRWLockExclusive(&m_NetSlaveLock);

//	CString str("TEST");
//	this->m_ptDFLog->DFLogSend(str, DF_SEVERITY_INFORMATION);
//	this->m_ptDFLog->DFLogSend(str, DF_SEVERITY_DEBUG);
//	this->m_ptDFLog->DFLogSend(str, DF_SEVERITY_WARNING);
//	this->m_ptDFLog->DFLogSend(str, DF_SEVERITY_HIGH);
//	this->m_ptDFLog->DFLogSend(str, DF_SEVERITY_DISASTER);

	int nSlaveCnt = 0;
	if (this->m_ptDFNet)
	{
		AcquireSRWLockExclusive(&this->m_ptDFNet->m_lockMaster);
		m_uServerStatus = this->m_ptDFNet == NULL ? 1 : 0;
		nSlaveCnt = ((m_uServerStatus == 0) ? this->m_ptDFNet->m_mapSlaveList.GetCount() : 0);
		ReleaseSRWLockExclusive(&this->m_ptDFNet->m_lockMaster);
	}

	int nFileCnt = 0;
	if (this->m_ptFileThread)
	{
		AcquireSRWLockExclusive(&this->m_ptFileThread->m_lockFileList);
		nFileCnt = this->m_ptFileThread->m_arrMtkFile.GetCount();
		ReleaseSRWLockExclusive(&this->m_ptFileThread->m_lockFileList);
	}

	// ��sServerStatus
	if (this->m_NetMenuGrid.GetRecords()->GetCount() == 4)
	{
		((CDFSetListRecord*)this->m_NetMenuGrid.GetRecords()->GetAt(0))->UpdateRecord(1, CString(m_uServerStatus==0 ? "�A�ȥ��`" : "�A�Ȱ���"));
		((CDFSetListRecord*)this->m_NetMenuGrid.GetRecords()->GetAt(0))->GetItem(1)->SetIconIndex(m_uServerStatus==0 ? 0 : 1);
		((CDFSetListRecord*)this->m_NetMenuGrid.GetRecords()->GetAt(1))->UpdateRecord(1, theApp.m_uListenPort);
		((CDFSetListRecord*)this->m_NetMenuGrid.GetRecords()->GetAt(2))->UpdateRecord(1, nFileCnt);
		((CDFSetListRecord*)this->m_NetMenuGrid.GetRecords()->GetAt(3))->UpdateRecord(1, nSlaveCnt);

		this->m_NetMenuGrid.RedrawControl();
	}

	// ��sSlaveList
	//int cnt = this->m_ptDFNet->m_mapSlaveList.GetCount();
	if (nSlaveCnt == 0)
	{
		this->m_NetListGrid.SetVirtualMode(new CDFNetSlaveRecord(this), 0);
		this->m_NetListGrid.Populate();
	}
	else if (this->m_ptDFNet->m_bRefresh/*this->m_NetListGrid.GetRecords()->GetCount() != nSlaveCnt*/)
	{
		this->m_ptDFNet->m_bRefresh = FALSE;

		// Sort
		if (m_NetSlaveList)
		{
			delete[] m_NetSlaveList;
			m_NetSlaveCount = 0;
		}

		m_NetSlaveList = new SORTSLAVE[nSlaveCnt];
		CString key;
		CDFNetSlave* slave = NULL;
		int i = 0;

		AcquireSRWLockExclusive(&this->m_ptDFNet->m_lockMaster);
		for (POSITION pos=this->m_ptDFNet->m_mapSlaveList.GetStartPosition(); pos!=NULL;)
		{
			this->m_ptDFNet->m_mapSlaveList.GetNextAssoc(pos, key, (void*&)slave);

			// �w������slave
			if (slave == NULL)
			{
				this->m_ptDFNet->m_mapSlaveList.RemoveKey(key);
				continue;
			}

			// �w���slave
			if (slave->IsStopSlave())
			{
				this->m_ptDFNet->m_mapSlaveList.RemoveKey(key);
				delete slave;
				slave = NULL;
				continue;
			}

			// OK
			if (i < nSlaveCnt)
			{
				memset(&m_NetSlaveList[i], NULL, sizeof(SORTSLAVE));
				sprintf_s(m_NetSlaveList[i].m_key, key);
				sprintf_s(m_NetSlaveList[i].m_data, slave->m_host);

				m_NetSlaveCount++;
			}

			i++;
		}
		ReleaseSRWLockExclusive(&this->m_ptDFNet->m_lockMaster);
		qsort(m_NetSlaveList, m_NetSlaveCount, sizeof(SORTSLAVE), &CMtkFileServerDlg::fnFileCompare);

		this->m_NetListGrid.SetVirtualMode(new CDFNetSlaveRecord(this), m_NetSlaveCount);
		this->m_NetListGrid.Populate();
	}
	else
	{
		this->m_NetListGrid.RedrawControl();
	}

	// ��sSlaveFileList
	
	ReleaseSRWLockExclusive(&m_NetSlaveLock);
}

/*static */int CMtkFileServerDlg::fnFileCompare(const void* a, const void* b)
{
	char* data_a = ((SORTUNIT*)a)->m_data;
	char* data_b = ((SORTUNIT*)b)->m_data;

	for (int i=0; i<256; i++)
	{
		if ((data_a[i]==0) && (data_b[i]==0))
			break;

		if ((data_a[i]-data_b[i]) == 0)
			continue;

		return data_a[i]-data_b[i];
	}

	return 0;
}

