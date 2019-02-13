
// DDFileClientDlg.cpp : ��@��
//

#include "stdafx.h"
#include "DDFileClient.h"
#include "DDFileClientDlg.h"
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


// class CTabDlg1 implementation 
CTabDlg1::CTabDlg1(CWnd* pParent/* = NULL*/)
	: CDialogEx(CTabDlg1::IDD, pParent)
{

}

BEGIN_MESSAGE_MAP(CTabDlg1, CDialogEx)
	ON_BN_CLICKED(IDC_NETTOOLBARBUTTON1, &CTabDlg1::OnAddServer)
	ON_BN_CLICKED(IDC_NETTOOLBARBUTTON2, &CTabDlg1::OnDelServer)
	ON_BN_CLICKED(IDC_NETTOOLBARBUTTON3, &CTabDlg1::OnStartServer)
	ON_BN_CLICKED(IDC_NETTOOLBARBUTTON4, &CTabDlg1::OnEndServer)
	ON_BN_CLICKED(IDC_NETTOOLBARBUTTON5, &CTabDlg1::OnRestartServer)
	ON_BN_CLICKED(IDC_FILETOOLBARBUTTON1, &CTabDlg1::OnReadonlyFile)
	ON_BN_CLICKED(IDC_FILETOOLBARBUTTON2, &CTabDlg1::OnRefreshFile)
	ON_BN_CLICKED(IDC_FILETOOLBARCLEAR, &CTabDlg1::OnClearFilter)
	ON_EN_CHANGE(IDC_FILETOOLBARFILTER, &CTabDlg1::OnChangeFilter)
END_MESSAGE_MAP()

void CTabDlg1::OnAddServer()
{
}

void CTabDlg1::OnDelServer()
{
}

void CTabDlg1::OnStartServer()
{
	CDDFileClientDlg* parent = (CDDFileClientDlg*)theApp.m_pMainWnd;
	CString strLog("");

	// 1.�s�u�޲z��
	for (UINT i=0; i<10; i++)
	{
		//if (parent->m_ptDFNet == NULL)
		if (parent->m_ptDFNet[i] == NULL && theApp.m_chServerIP[i][0] != 0)
		{
			AcquireSRWLockExclusive(&parent->m_lockServer);
			parent->m_ptDFNet[i] = new CDFNetMaster(CDFNetMaster::ThreadFunc, parent, i, theApp.m_chServerIP[i], theApp.m_uServerPort[i]);
			VERIFY(parent->m_ptDFNet[i]->CreateThread());

			strLog.Format("[�ϥΪ̩R�O]���ҳs�u�޲z��(Thread:%d)", parent->m_ptDFNet[i]->m_nThreadID);
			parent->m_ptDFLog->DFLogSend(strLog, DF_SEVERITY_INFORMATION);

			parent->m_bManualStop = FALSE;
		}
	}
	//OnChangeMenu(NULL, NULL);
}

void CTabDlg1::OnEndServer()
{
	CDDFileClientDlg* parent = (CDDFileClientDlg*)theApp.m_pMainWnd;
	DWORD nThreadID = 0;
	CString strLog("");

	//AcquireSRWLockExclusive(&parent->m_NetSlaveLock);

	// 0.�T�T �s�u�ʱ��p�ɾ�
	//parent->KillTimer(IDC_TIMER);

	// 1.�T�T �s�u�޲z��
	for (UINT i=0; i<10; i++)
	{
		if (parent->m_ptDFNet[i])
		{
			nThreadID = parent->m_ptDFNet[i]->m_nThreadID;
			parent->m_ptDFNet[i]->ForceStopMaster();
			parent->m_ptDFNet[i]->StopThread();
			WaitForSingleObject(parent->m_ptDFNet[i]->m_hThread, INFINITE);
		
			strLog.Format("[�ϥΪ̩R�O]�����s�u�޲z��(Thread:%d)", nThreadID);
			parent->m_ptDFLog->DFLogSend(strLog, DF_SEVERITY_INFORMATION);
		
			delete parent->m_ptDFNet[i];
			parent->m_ptDFNet[i] = NULL;

			parent->m_bManualStop = TRUE;
		}
	}

	//ReleaseSRWLockExclusive(&parent->m_NetSlaveLock);
	//OnChangeMenu(NULL, NULL);
}

void CTabDlg1::OnRestartServer()
{
	OnEndServer();
	Sleep(1000);
	OnStartServer();
}

void CTabDlg1::OnReadonlyFile()
{
	// �]�w�ɮװ�Ū���s
	CDDFileClientDlg* parent = (CDDFileClientDlg*)theApp.m_pMainWnd;
	int selectcount = parent->m_FileGrid.GetSelectedRows()->GetCount();

	int index = 0;
	MTK* mtk = NULL;
	CString strFilename("");
	CString strLog("");
	
	CXTPReportColumnOrder *pSortOrder = parent->m_FileGrid.GetColumns()->GetSortOrder();
	BOOL bDecreasing = pSortOrder->GetCount() > 0 && pSortOrder->GetAt(0)->IsSortedDecreasing();

/////////////////////

	//CArray<int, int> arTest;
	//arTest.Add(0);
	//arTest.Add(1);
	//arTest.Add(2);
	//arTest.Add(3);
	//arTest.Add(4);
	//arTest.Add(5);
	//arTest.Add(6);
	//arTest.Add(7);
	//arTest.Add(8);
	//arTest.Add(9);

	//int cnt = arTest.GetCount();
	//int data = arTest[1];
	//arTest.RemoveAt(1);
	//
	//cnt = arTest.GetCount();
	//data = arTest[1];
	//arTest.RemoveAt(1);

/////////////////////

	AcquireSRWLockExclusive(&parent->m_ptDFFile->m_lockFileList);

	for (int i=0; i<selectcount; i++)
	{
		index = parent->m_FileGrid.GetSelectedRows()->GetAt(i)->GetIndex();
		
		if (bDecreasing)
			index = parent->m_ptDFFile->m_ptSortList[parent->m_FileGrid.GetRows()->GetCount()-index-1].m_index;
		else
			index = parent->m_ptDFFile->m_ptSortList[index].m_index;

		mtk = parent->m_ptDFFile->m_arrMtkFile[index];
		
		if (mtk->m_info.m_attribute == FALSE)
		{
			strFilename.Format("%s%s", parent->m_ptDFFile->m_chFilePath, mtk->m_chFilename);
			SetFileAttributes(strFilename, FILE_ATTRIBUTE_READONLY);
			mtk->m_info.m_attribute = TRUE;
			mtk->m_master = NULL;

			strLog.Format("�j���ɮװ�Ū[%s]", mtk->m_chFilename);
			parent->m_ptDFLog->DFLogSend(strLog, DF_SEVERITY_INFORMATION);
		}
	}

	ReleaseSRWLockExclusive(&parent->m_ptDFFile->m_lockFileList);
}

/*afx_msg */void CTabDlg1::OnRefreshFile()
{
	// ���歫�s��z���s
	// �S�ƻ�n�@��, ���ӴN�@���A���s��z
}

/*afx_msg */void CTabDlg1::OnClearFilter()
{
	CDDFileClientDlg* parent = (CDDFileClientDlg*)theApp.m_pMainWnd;
	parent->m_FileFilterEdit.SetWindowText(_T(""));
	parent->m_FileGrid.SetFilterText(_T(""));
}

/*afx_msg */void CTabDlg1::OnChangeFilter()
{
	CDDFileClientDlg* parent = (CDDFileClientDlg*)theApp.m_pMainWnd;
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


// class CTabDlg2 implementation 
CTabDlg2::CTabDlg2(CWnd* pParent/* = NULL*/)
	: CDialogEx(CTabDlg2::IDD, pParent)
{

}

BEGIN_MESSAGE_MAP(CTabDlg2, CDialogEx)
	ON_BN_CLICKED(IDC_SETTOOLBARBUTTON1, &CTabDlg2::OnStartServer)
	ON_BN_CLICKED(IDC_SETTOOLBARBUTTON2, &CTabDlg2::OnStopServer)
	ON_BN_CLICKED(IDC_SETTOOLBARBUTTON3, &CTabDlg2::OnRestartServer)
	ON_BN_CLICKED(IDC_SETTOOLBARBUTTON4, &CTabDlg2::OnSave)
	ON_NOTIFY(XTP_NM_REPORT_SELCHANGED, IDC_SETMENUGRID, &CTabDlg2::OnChangeMenu)
	ON_NOTIFY(XTP_NM_REPORT_VALUECHANGED, IDC_SETLISTGRID, &CTabDlg2::OnChangeSettings)
END_MESSAGE_MAP()

/*afx_msg */void CTabDlg2::OnStartServer()
{
	CDDFileClientDlg* parent = (CDDFileClientDlg*)theApp.m_pMainWnd;
	CString strLog("");

	// 1.�s�u�޲z��
	for (UINT i=0; i<10; i++)
	{
		if (parent->m_ptDFNet[i] == NULL && theApp.m_chServerIP[i][0] != 0)
		{
			AcquireSRWLockExclusive(&parent->m_lockServer);
			parent->m_ptDFNet[i] = new CDFNetMaster(CDFNetMaster::ThreadFunc, parent, i, theApp.m_chServerIP[i], theApp.m_uServerPort[i]);
			VERIFY(parent->m_ptDFNet[i]->CreateThread());

			strLog.Format("[�ϥΪ̩R�O]���ҳs�u�޲z��(Thread:%d)", parent->m_ptDFNet[i]->m_nThreadID);
			parent->m_ptDFLog->DFLogSend(strLog, DF_SEVERITY_INFORMATION);

			parent->m_bManualStop = FALSE;
		}
	}

	OnChangeMenu(NULL, NULL);
}

/*afx_msg */void CTabDlg2::OnStopServer()
{
	CDDFileClientDlg* parent = (CDDFileClientDlg*)theApp.m_pMainWnd;
	DWORD nThreadID = 0;
	CString strLog("");

	//AcquireSRWLockExclusive(&parent->m_NetSlaveLock);

	// 0.�T�T �s�u�ʱ��p�ɾ�
	//parent->KillTimer(IDC_TIMER);

	// 1.�T�T �s�u�޲z��
	for (UINT i=0; i<10; i++)
	{
		if (parent->m_ptDFNet[i])
		{
			nThreadID = parent->m_ptDFNet[i]->m_nThreadID;
			parent->m_ptDFNet[i]->ForceStopMaster();
			parent->m_ptDFNet[i]->StopThread();
			WaitForSingleObject(parent->m_ptDFNet[i]->m_hThread, INFINITE);
		
			strLog.Format("[�ϥΪ̩R�O]�����s�u�޲z��(Thread:%d)", nThreadID);
			parent->m_ptDFLog->DFLogSend(strLog, DF_SEVERITY_INFORMATION);
		
			delete parent->m_ptDFNet[i];
			parent->m_ptDFNet[i] = NULL;

			parent->m_bManualStop = TRUE;
		}
	}

	//ReleaseSRWLockExclusive(&parent->m_NetSlaveLock);
	OnChangeMenu(NULL, NULL);
}

/*afx_msg */void CTabDlg2::OnRestartServer()
{
	OnStopServer();
	Sleep(1000);
	OnStartServer();
}

/*afx_msg */void CTabDlg2::OnSave()
{
	CDDFileClientDlg* parent = (CDDFileClientDlg*)theApp.m_pMainWnd;
	
	parent->SaveIni();
}

/*afx_msg */void CTabDlg2::OnChangeMenu(NMHDR* pNMHDR, LRESULT* pResult)
{
	CDDFileClientDlg* parent = (CDDFileClientDlg*)theApp.m_pMainWnd;
	
	int select = parent->m_SetMenuGrid.GetSelectedRows()->GetAt(0)->GetIndex();
		
	theApp.m_uServerStatus = parent->m_ptDFNet == NULL ? 1 : 0;
	switch (select)
	{
	case 0: // �t�Ϊ��A
		{
			parent->m_SetListGrid.GetRecords()->RemoveAll();
			parent->m_SetListGrid.AddRecord(new CDFSetListRecord(NULL, CString("�t�Ϊ��A"), CString(theApp.m_uServerStatus==0 ? "�A�ȥ��`" : "�A�Ȱ���")));
			parent->m_SetListGrid.GetRecords()->GetAt(0)->GetItem(1)->SetIconIndex(theApp.m_uServerStatus==0 ? 0 : 1);
			parent->m_SetListGrid.GetColumns()->GetAt(1)->SetEditable(FALSE);
			parent->m_SetListGrid.Populate();
			break;
		}
	case 1: // �t�γ]�w
		{
			parent->m_SetListGrid.GetRecords()->RemoveAll();
			parent->m_SetListGrid.AddRecord(new CDFSetListRecord(NULL, CString("�ɮץؿ�"), CString(theApp.m_chFileDir)));
			parent->m_SetListGrid.AddRecord(new CDFSetListRecord(NULL, CString("�s�u���A��Key"), CString(theApp.m_chKey)));
			parent->m_SetListGrid.AddRecord(new CDFSetListRecord(NULL, CString("�ʱ����A��IP"), CString(theApp.m_chZabbixIP)));
			parent->m_SetListGrid.AddRecord(new CDFSetListRecord(NULL, CString("�ʱ����A��Port"), theApp.m_uZabbixPort));
			parent->m_SetListGrid.AddRecord(new CDFSetListRecord(NULL, CString("�ʱ����A��Host"), CString(theApp.m_chZabbixHost)));
			parent->m_SetListGrid.AddRecord(new CDFSetListRecord(NULL, CString("�ʱ����A��Key"), CString(theApp.m_chZabbixKey)));
			parent->m_SetListGrid.AddRecord(new CDFSetListRecord(NULL, CString("�ʱ��W�v"), theApp.m_uZabbixFreq));
			parent->m_SetListGrid.GetColumns()->GetAt(1)->SetEditable(TRUE);
			parent->m_SetListGrid.Populate();
			break;
		}
	case 2: // �s�u�]�w
		{
			parent->m_SetListGrid.GetRecords()->RemoveAll();
			CString strItem("");
			CString strValue("");
			for (int i=0; i<MAX_SERVER_COUNT; i++)
			{
				if (theApp.m_chServerIP[i][0] != 0)
				{	
					strItem.Format("[%d]���A����}", i);
					strValue.Format("%s:%d", theApp.m_chServerIP[i], theApp.m_uServerPort[i]);
					parent->m_SetListGrid.AddRecord(new CDFSetListRecord(NULL, strItem, strValue));
				}
				else
				{
					strItem.Format("[%d]���A����}", i);
					strValue = "";
					parent->m_SetListGrid.AddRecord(new CDFSetListRecord(NULL, strItem, strValue));
				}
			}
			parent->m_SetListGrid.GetColumns()->GetAt(1)->SetEditable(FALSE);
			parent->m_SetListGrid.Populate();
			break;
		}
	case 3: // �ɮ׳]�w
		{
			CArray<CString, CString&> arrFilter;
			parent->m_SetListGrid.GetRecords()->RemoveAll();
			for (int i=0; i<arrFilter.GetCount(); i++)
			{
				parent->m_SetListGrid.AddRecord(new CDFSetListRecord(NULL, CString("�L�o�ɮ�"), arrFilter[i]));
			}
			parent->m_SetListGrid.GetColumns()->GetAt(1)->SetEditable(TRUE);
			parent->m_SetListGrid.Populate();
			break;
		}
	case 4: // �����]�w
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

/*afx_msg */void CTabDlg2::OnChangeSettings(NMHDR* pNotifyStruct, LRESULT*)
{
	XTP_NM_REPORTRECORDITEM* pItemNotify = (XTP_NM_REPORTRECORDITEM*)pNotifyStruct;
	if(!pItemNotify) return;
	
	if(pItemNotify->pItem == NULL || pItemNotify->pItem->GetRecord() == NULL) return;

	CDDFileClientDlg* parent = (CDDFileClientDlg*)theApp.m_pMainWnd;
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
	else if ((menu==1) && (select==1) && count>1) // �ʱ����A��IP
	{
		parent->m_SetListGrid.GetRecords()->GetAt(1)->GetItem(1)->SetCaption(caption);
		memset(theApp.m_chZabbixIP, NULL, 16);
		memcpy(theApp.m_chZabbixIP, caption, caption.GetLength());
	}
	else if ((menu==1) && (select==2) && count>2) // �ʱ����A��Port
	{
		parent->m_SetListGrid.GetRecords()->GetAt(2)->GetItem(1)->SetCaption(caption);
		theApp.m_uZabbixPort = atoi(caption);
	}
	else if ((menu==1) && (select==3) && count>3) // �ʱ����A��Host
	{
		parent->m_SetListGrid.GetRecords()->GetAt(3)->GetItem(1)->SetCaption(caption);	
		memset(theApp.m_chZabbixHost, NULL, 16);
		memcpy(theApp.m_chZabbixHost, caption, caption.GetLength());
	}
	else if ((menu==1) && (select==4) && count>4) // �ʱ����A��Key
	{
		parent->m_SetListGrid.GetRecords()->GetAt(4)->GetItem(1)->SetCaption(caption);
		memset(theApp.m_chZabbixKey, NULL, 50);
		memcpy(theApp.m_chZabbixKey, caption, caption.GetLength());
	}
	else if ((menu==1) && (select==5) && count>5) // �ʱ��W�v
	{
		parent->m_SetListGrid.GetRecords()->GetAt(5)->GetItem(1)->SetCaption(caption);
		theApp.m_uZabbixFreq = atoi(caption);
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

/*afx_msg */void CTabDlg2::OnOK()
{

}

/*afx_msg */void CTabDlg2::OnCancel()
{

}

/*afx_msg */void CTabDlg2::OnDestroy()
{

}


// class CTabDlg3 implementation 
CTabDlg3::CTabDlg3(CWnd* pParent/* = NULL*/)
	: CDialogEx(CTabDlg3::IDD, pParent)
, uFilter1(0)
, uFilter2(0)
, uFilter3(0)
, uFilter4(0)
, uFilter5(0)
{

}

BEGIN_MESSAGE_MAP(CTabDlg3, CDialogEx)
	ON_BN_CLICKED(IDC_LOGTOOLBARBUTTON1, &CTabDlg3::OnClear)
	ON_BN_CLICKED(IDC_LOGTOOLBARFILTER1, &CTabDlg3::OnFilter1)
	ON_BN_CLICKED(IDC_LOGTOOLBARFILTER2, &CTabDlg3::OnFilter2)
	ON_BN_CLICKED(IDC_LOGTOOLBARFILTER3, &CTabDlg3::OnFilter3)
	ON_BN_CLICKED(IDC_LOGTOOLBARFILTER4, &CTabDlg3::OnFilter4)
	ON_BN_CLICKED(IDC_LOGTOOLBARFILTER5, &CTabDlg3::OnFilter5)
END_MESSAGE_MAP()

/*afx_msg */void CTabDlg3::OnClear()
{
	CDDFileClientDlg* parent = (CDDFileClientDlg*)theApp.m_pMainWnd;
	parent->m_ptDFLog->ClearAll();
}

/*afx_msg */void CTabDlg3::OnFilter1()
{
	CDDFileClientDlg* parent = (CDDFileClientDlg*)theApp.m_pMainWnd;
	uFilter1 = (uFilter1+1) % 2;
	
	if (uFilter1)
		parent->m_ptDFLog->SetFilterOn(1);
	else
		parent->m_ptDFLog->SetFilterOff(1);
}

/*afx_msg */void CTabDlg3::OnFilter2()
{
	CDDFileClientDlg* parent = (CDDFileClientDlg*)theApp.m_pMainWnd;
	uFilter2 = (uFilter2+1) % 2;

	if (uFilter2)
		parent->m_ptDFLog->SetFilterOn(2);
	else
		parent->m_ptDFLog->SetFilterOff(2);
}

/*afx_msg */void CTabDlg3::OnFilter3()
{
	CDDFileClientDlg* parent = (CDDFileClientDlg*)theApp.m_pMainWnd;
	uFilter3 = (uFilter3+1) % 2;

	if (uFilter3)
		parent->m_ptDFLog->SetFilterOn(4);
	else
		parent->m_ptDFLog->SetFilterOff(4);
}

/*afx_msg */void CTabDlg3::OnFilter4()
{
	CDDFileClientDlg* parent = (CDDFileClientDlg*)theApp.m_pMainWnd;
	uFilter4 = (uFilter4+1) % 2;

	if (uFilter4)
		parent->m_ptDFLog->SetFilterOn(8);
	else
		parent->m_ptDFLog->SetFilterOff(8);
}

/*afx_msg */void CTabDlg3::OnFilter5()
{
	CDDFileClientDlg* parent = (CDDFileClientDlg*)theApp.m_pMainWnd;
	uFilter5 = (uFilter5+1) % 2;

	if (uFilter5)
		parent->m_ptDFLog->SetFilterOn(16);
	else
		parent->m_ptDFLog->SetFilterOff(16);
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

// class CDFNetRecord implementation
CDFNetRecord::CDFNetRecord(CDDFileClientDlg* master, UINT index, CString& text)
: m_uServerID(index)
, m_DlgMaster(master)
{
	AddItem(new CXTPReportRecordItemText(text));
	this->GetChilds()->Add(new CDFSetMenuRecord(master, CString("�ɶ�: ")));
	this->GetChilds()->Add(new CDFSetMenuRecord(master, CString("���m: ")));
	this->GetChilds()->Add(new CDFSetMenuRecord(master, CString("�ɮ�: ")));
	this->GetChilds()->Add(new CDFSetMenuRecord(master, CString("��q: ")));
	this->GetChilds()->Add(new CDFSetMenuRecord(master, CString("�`�q: ")));
}
	
/*virtual */CDFNetRecord::~CDFNetRecord(void)
{

}

/*virtual */void CDFNetRecord::CreateItems()
{

}
	
/*virtual */void CDFNetRecord::GetItemMetrics(XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics)
{
	CDFNetMaster* master = m_DlgMaster->m_ptDFNet[m_uServerID];
	CString str("");
	
	if (master == NULL)
		return;

	AcquireSRWLockExclusive(&master->m_NetServerStatusLock);

	str.Format("�ɶ�: %s", master->m_NetServerStartTime);
	this->GetChilds()->GetAt(0)->GetItem(0)->SetCaption(str);

	str.Format("���m: %02d sec", (GetTickCount()-master->m_NetServerLastTick)/1000);
	//master->m_NetServerLastTick = GetTickCount();
	this->GetChilds()->GetAt(1)->GetItem(0)->SetCaption(str);
	
	str.Format("�ɮ�: %d", master->m_NetServerFileCount);
	this->GetChilds()->GetAt(2)->GetItem(0)->SetCaption(str);

	str.Format("��q: %s/%s(%d%%)", m_DlgMaster->ConvertUINT64toString(master->m_NetServerComVolumn), m_DlgMaster->ConvertUINT64toString(master->m_NetServerVolumn), master->m_NetServerVolumn==0 ? 0 : master->m_NetServerComVolumn*100/master->m_NetServerVolumn);
	this->GetChilds()->GetAt(3)->GetItem(0)->SetCaption(str);

	str.Format("�`�q: %s/%s(%d%%)", m_DlgMaster->ConvertUINT64toString(master->m_NetServerTotalComVolumn), m_DlgMaster->ConvertUINT64toString(master->m_NetServerTotalVolumn), master->m_NetServerTotalVolumn==0 ? 0 : master->m_NetServerTotalComVolumn*100/master->m_NetServerTotalVolumn);
	this->GetChilds()->GetAt(4)->GetItem(0)->SetCaption(str);

	//UINT64 val=0;
	//if (master->m_NetServerVolumn > (1024*1024*1024))
	//{
	//	val = master->m_NetServerVolumn / (1024*1024*1024);
	//	str.Format("��q: 000GB/%dGB(%02d%%)", val, 0);
	//}
	//else if (master->m_NetServerVolumn > (1024*1024))
	//{
	//	val = master->m_NetServerVolumn / (1024*1024);
	//	str.Format("��q: 000MB/%dMB(%02d%%)", val, 0);
	//}
	//else if (master->m_NetServerVolumn > 1024)
	//{
	//	val = master->m_NetServerVolumn / 1024;
	//	str.Format("��q: 000KB/%dKB(%02d%%)", val, 0);
	//}
	//else
	//{
	//	val = master->m_NetServerVolumn;
	//	str.Format("��q: 000B/%dB(%02d%%)", val, 0);
	//}
	//this->GetChilds()->GetAt(3)->GetItem(0)->SetCaption(str);
	//
	//if (master->m_NetServerTotalVolumn / (1024*1024*1024))
	//{
	//	val = master->m_NetServerTotalVolumn / (1024*1024*1024);
	//	str.Format("�`�q: 000GB/%dGB(%02d%%)", val, 0);
	//}
	//else if (master->m_NetServerTotalVolumn / (1024*1024))
	//{
	//	val = master->m_NetServerTotalVolumn / (1024*1024);
	//	str.Format("�`�q: 000MB/%dMB(%02d%%)", val, 0);
	//}
	//else if (master->m_NetServerTotalVolumn > 1024)
	//{
	//	val = master->m_NetServerTotalVolumn / 1024;
	//	str.Format("�`�q: 000KB/%dKB(%02d%%)", val, 0);
	//}
	//else
	//{
	//	val = master->m_NetServerTotalVolumn;
	//	str.Format("�`�q: 000B/%dB(%02d%%)", val, 0);
	//}
	//this->GetChilds()->GetAt(4)->GetItem(0)->SetCaption(str);

	ReleaseSRWLockExclusive(&master->m_NetServerStatusLock);

	CXTPReportRecord::GetItemMetrics(pDrawArgs, pItemMetrics);
}


// class CDFSetMenuRecord implementation
CDFSetMenuRecord::CDFSetMenuRecord(CDDFileClientDlg* master, CString& text)
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

CDFSetListRecord::CDFSetListRecord(CDDFileClientDlg* master, CString& text, CString& data)
{
	AddItem(new CXTPReportRecordItemText(text));
	AddItem(new CXTPReportRecordItemText(data));
}

CDFSetListRecord::CDFSetListRecord(CDDFileClientDlg* master, CString& text, UINT data)
{
	AddItem(new CXTPReportRecordItemText(text));

	CString str("");
	str.Format("%d", data);
	AddItem(new CXTPReportRecordItemText(str));
}

CDFSetListRecord::CDFSetListRecord(CDDFileClientDlg* master, CString& text, CArray<CString, CString&>& data)
{
	AddItem(new CXTPReportRecordItemText(text));

//	CXTPReportRecordItemText txt;
//	txt.
//	AddItem(new CXTPReportRecordItemText(""));
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


// CDDFileClientDlg ��ܤ��
CDDFileClientDlg::CDDFileClientDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDDFileClientDlg::IDD, pParent)
, m_ptDFFile(NULL)
, m_ptDFLog(NULL)
//, m_ptDFNet(NULL)
, m_bManualStop(FALSE)
{
	for (UINT i=0; i<10; i++)
		m_ptDFNet[i] = NULL;

	//m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON2);
	InitializeSRWLock(&m_lockServer);
}

void CDDFileClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB1, m_MainTabCtrl);
}

BEGIN_MESSAGE_MAP(CDDFileClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_WM_QUERYDRAGICON()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, &CDDFileClientDlg::OnTcnSelchangeTab1)
END_MESSAGE_MAP()

/*afx_msg */void CDDFileClientDlg::OnOK()
{

}

/*afx_msg */void CDDFileClientDlg::OnCancel()
{

}

/*afx_msg */void CDDFileClientDlg::OnDestroy()
{

}

// CDDFileClientDlg �T���B�z�`��
BOOL CDDFileClientDlg::OnInitDialog()
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

	// INI
	InitApp();
	SetWindowText(theApp.m_strTitle);

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

	// TODO: �b���[�J�B�~����l�]�w
	// 1. [Main] TabControl �D��
	// 1.1 �إߤ�������
	m_MainTabImageList.Create(16, 16, ILC_COLOR24 | ILC_MASK, 1, 1);
	UINT arMainTabImg[] = {IDB_BITMAP2, IDB_BITMAP3, IDB_BITMAP4};
	for (int i=0; i<3; i++)
	{
		bmp.LoadBitmap(arMainTabImg[i]);
		m_MainTabImageList.Add(&bmp, RGB(255, 255, 255));
		bmp.DeleteObject();
	}
	m_MainTabCtrl.SetImageList(&m_MainTabImageList);
	m_MainTabCtrl.InsertItem(0, _T("�s�u�޲z"), 0);
	m_MainTabCtrl.InsertItem(1, _T("�t�γ]�w"), 1);
	m_MainTabCtrl.InsertItem(2, _T("�t�ά���"), 2);
	
	// 1.2 �إߤ������e
	m_MainTab1.Create(IDD_DIALOG1, &m_MainTabCtrl);
	m_MainTab2.Create(IDD_DIALOG2, &m_MainTabCtrl);
	m_MainTab3.Create(IDD_DIALOG3, &m_MainTabCtrl);

	// 2. [Tab1] �s�u�޲z��+�ɮ׺޲z��
	m_MainTab1.GetClientRect(&rect);
	
	// 2.1. ToolBar �s�u�޲z�u��C
	m_NetToolBarImageList.Create(16, 16, ILC_COLOR24 | ILC_MASK, 1, 1);
	UINT arNetToolBarImage[] = {IDB_BITMAP5, IDB_BITMAP6, IDB_BITMAP7, IDB_BITMAP9, IDB_BITMAP8};
	UINT arNetToolBarBtn[] = {IDC_NETTOOLBARBUTTON1, IDC_NETTOOLBARBUTTON2, IDC_NETTOOLBARBUTTON3, IDC_NETTOOLBARBUTTON4, IDC_NETTOOLBARBUTTON5};
	for (int i=0; i<5; i++)
	{
		bmp.LoadBitmap(arNetToolBarImage[i]);
		m_NetToolBarImageList.Add(&bmp, RGB(255, 255, 255));
		bmp.DeleteObject();
	}
	m_NetToolBar.CreateEx(&m_MainTab1, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, CRect(0, 0, 0, 0));
	m_NetToolBar.SetButtons(arNetToolBarBtn, 5);
	m_NetToolBar.SetSizes(CSize(24, 24), CSize(16, 16));
	m_NetToolBar.GetToolBarCtrl().SetImageList(&m_NetToolBarImageList);
	m_NetToolBar.MoveWindow(0, 0, 182, 40, 1);
	
	m_NetToolBar.SetButtonStyle(0, TBBS_AUTOSIZE);
	m_NetToolBar.SetButtonText(0, _T("�s�W"));
	m_NetToolBar.SetButtonStyle(1, TBBS_AUTOSIZE);
	m_NetToolBar.SetButtonText(1, _T("�R��"));
	m_NetToolBar.SetButtonStyle(2, TBBS_AUTOSIZE);
	m_NetToolBar.SetButtonText(2, _T("�}�l"));
	m_NetToolBar.SetButtonStyle(3, TBBS_AUTOSIZE);
	m_NetToolBar.SetButtonText(3, _T("����"));
	m_NetToolBar.SetButtonStyle(4, TBBS_AUTOSIZE);
	m_NetToolBar.SetButtonText(4, _T("����"));

	// 2.2. Grid �s�u��ƲM��
	m_NetGridImageList.Create(16, 16, ILC_COLOR16 | ILC_MASK, 1, 1);
	m_NetGrid.Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_OVERLAPPED | WS_TABSTOP | WS_BORDER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, CRect(0, 41, 182, rect.Height()-42-35), &m_MainTab1, IDC_NETGRID, NULL);
	m_NetGrid.GetPaintManager()->SetColumnStyle(xtpReportColumnFlat);
	m_NetGrid.GetPaintManager()->SetGridStyle(FALSE, xtpReportGridNoLines);
	m_NetGrid.GetPaintManager()->SetHeaderHeight(0);
	m_NetGrid.GetReportHeader()->AllowColumnRemove(FALSE);
	m_NetGrid.SetImageList(&m_NetGridImageList);

	m_NetGrid.AddColumn(new CXTPReportColumn(0, _T(""), 80, 1, -1, 0));
	//m_NetGrid.AddColumn(new CXTPReportColumn(1, _T(""), 100, 1, -1, 0));
	m_NetGrid.GetColumns()->GetAt(0)->SetTreeColumn(TRUE);
	
	m_NetGrid.GetPaintManager()->m_treeStructureStyle = xtpReportTreeStructureDots;
	m_NetGrid.GetPaintManager()->m_nTreeIndent = 10;
	m_NetGrid.Populate();
	
	// 2.3. ToolBar1 �ɮ׺޲z�u��C
	m_FileToolBarImageList.Create(16, 16, ILC_COLOR16 | ILC_MASK, 1, 1);
	UINT arFileToolBar1Image[] = {IDB_BITMAP10, IDB_BITMAP12, IDB_BITMAP11};
	UINT arFileToolBar1Btn[] = {IDC_FILETOOLBARBUTTON1, IDC_FILETOOLBARBUTTON2};
	for (int i=0; i<2; i++)
	{
		bmp.LoadBitmap(arFileToolBar1Image[i]);
		m_FileToolBarImageList.Add(&bmp, RGB(255, 255, 255));
		bmp.DeleteObject();
	}
	m_FileToolBar1.CreateEx(&m_MainTab1, TBSTYLE_FLAT | TBSTYLE_LIST, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, CRect(0, 0, 0, 0));
	m_FileToolBar1.SetButtons(arFileToolBar1Btn, 3);
	m_FileToolBar1.SetSizes(CSize(24, 24), CSize(16, 16));
	m_FileToolBar1.GetToolBarCtrl().SetImageList(&m_FileToolBarImageList);
	m_FileToolBar1.MoveWindow(182, 0, rect.Width()-182, 24, 1);

	m_FileToolBar1.SetButtonStyle(0, TBBS_AUTOSIZE);
	m_FileToolBar1.SetButtonText(0, _T("�ɮװ�Ū"));

	m_FileToolBar1.SetButtonStyle(1, TBBS_AUTOSIZE);
	m_FileToolBar1.SetButtonText(1, _T("���s��z"));

//	m_FileToolBar1.SetButtonInfo(2, IDC_FILEVERSIONSTATIC, TBBS_SEPARATOR, -1);
	m_FileVersionText.Create(_T("�{������ ")+theApp.m_strVersion, WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE, CRect(rect.Width()-342, 0, rect.Width()-182, 24), &m_FileToolBar1, IDC_FILEVERSIONSTATIC);
	m_FileVersionText.SetFont(&m_Font);

	// 2.4. ToolBar2 �ɮ׺޲z�j�M 
	UINT arFileToolBar2Btn[] = {IDC_FILETOOLBARSTATIC, IDC_FILETOOLBARFILTER, IDC_FILETOOLBARCLEAR};
	m_FileToolBar2.CreateEx(&m_MainTab1, TBSTYLE_FLAT | TBSTYLE_LIST, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, CRect(0, 0, 0, 0));
	m_FileToolBar2.SetButtons(arFileToolBar2Btn, 3);
	m_FileToolBar2.SetSizes(CSize(24, 24), CSize(16, 16));
	m_FileToolBar2.MoveWindow(182, 25, rect.Width()-182, 24, 1);	

	m_FileToolBar2.SetButtonInfo(0, IDC_FILETOOLBARSTATIC, TBBS_SEPARATOR, -1);
	m_FileFilterText.Create(_T("�ɮ׹L�o"), WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE, CRect(0, 0, 60, 24), &m_FileToolBar2, IDC_FILETOOLBARSTATIC);
	m_FileFilterText.SetFont(&m_Font);

	m_FileToolBar2.SetButtonInfo(1, IDC_FILETOOLBARFILTER, TBBS_SEPARATOR, -1);
	m_FileFilterEdit.Create(WS_CHILD | WS_VISIBLE | WS_BORDER, CRect(62, 0+1, 262, 24-3), &m_FileToolBar2, IDC_FILETOOLBARFILTER);
	m_FileFilterEdit.SetFont(&m_Font);

	m_FileToolBar2.SetButtonInfo(2, IDC_FILETOOLBARCLEAR, TBBS_SEPARATOR, -1);
	m_FileFilterClearButton.Create(_T("�M��"), WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE, CRect(264, 0+1, 296, 24-3), &m_FileToolBar2, IDC_FILETOOLBARCLEAR);
	m_FileFilterClearButton.SetFont(&m_Font);

	// 2.4. Grid �ɮײM��
	m_FileGridImageList.Create(16, 16, ILC_COLOR24 | ILC_MASK, 1, 1);

	bmp.LoadBitmap(IDB_BITMAP17);
	m_FileGridImageList.Add(&bmp, RGB(0, 0, 0));
	bmp.DeleteObject();

	bmp.LoadBitmap(IDB_BITMAP18);
	m_FileGridImageList.Add(&bmp, RGB(0, 0, 0));
	bmp.DeleteObject();

	bmp.LoadBitmap(IDB_BITMAP19);
	m_FileGridImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	bmp.LoadBitmap(IDB_BITMAP21);
	m_FileGridImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	m_FileGrid.Create(WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_TABSTOP | WS_BORDER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, CRect(184, 50, rect.Width()-5, rect.Height()-42-35), &m_MainTab1, IDC_FILEGRID, NULL);
	m_FileGrid.GetPaintManager()->SetColumnStyle(xtpReportColumnFlat);
	m_FileGrid.GetPaintManager()->SetGridStyle(FALSE, xtpReportGridNoLines);
	m_FileGrid.GetReportHeader()->AllowColumnRemove(FALSE);
	m_FileGrid.SetImageList(&m_FileGridImageList);

	m_FileGrid.AddColumn(new CXTPReportColumn(0, _T("�s��"), 40, 0, -1, 0));
	m_FileGrid.AddColumn(new CXTPReportColumn(1, _T("���A��"), 80));
	m_FileGrid.AddColumn(new CXTPReportColumn(2, _T("�ɮצW��"), 140));
	m_FileGrid.AddColumn(new CXTPReportColumn(3, _T("MTK"), 30));
	m_FileGrid.AddColumn(new CXTPReportColumn(4, _T("�̫��s�ɶ�"), 130, 0));
	m_FileGrid.AddColumn(new CXTPReportColumn(5, _T("�ɮפj�p"), 60));
	m_FileGrid.AddColumn(new CXTPReportColumn(6, _T("���m"), 30));
	m_FileGrid.AddColumn(new CXTPReportColumn(7, _T("���y�q"), 100, 1, -1, 0));
	m_FileGrid.AddColumn(new CXTPReportColumn(8, _T("�y�q"), 80, 0, -1, 0));

	m_FileGrid.GetColumns()->GetAt(5)->SetAlignment(DT_RIGHT);
	m_FileGrid.GetColumns()->GetAt(6)->SetAlignment(DT_RIGHT);
	m_FileGrid.GetColumns()->GetAt(7)->SetAlignment(DT_RIGHT);
	m_FileGrid.GetColumns()->GetAt(8)->SetAlignment(DT_RIGHT);

	m_FileGrid.Populate();

	// 3. [Tab2] Set �t�γ]�w��
	m_MainTab2.GetClientRect(&rect);

	// 3.1. ToolBar �x�s�]�w�u��
	m_SetListToolBarImageList.Create(16, 16, ILC_COLOR16 | ILC_MASK, 1, 1);
	UINT arSetListToolBarBtn[] = {IDC_SETTOOLBARBUTTON4};
	bmp.LoadBitmap(IDB_BITMAP15);
	m_SetListToolBarImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();
	m_SetListToolBar.CreateEx(&m_MainTab2, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, CRect(0, 0, 0, 0));
	m_SetListToolBar.SetButtons(arSetListToolBarBtn, 1);
	m_SetListToolBar.SetSizes(CSize(24, 24), CSize(16, 16));
	m_SetListToolBar.GetToolBarCtrl().SetImageList(&m_SetListToolBarImageList);
	m_SetListToolBar.MoveWindow(112, 0, rect.Width()-112, 40, 1);
	
	m_SetListToolBar.SetButtonStyle(0, TBBS_AUTOSIZE);
	m_SetListToolBar.SetButtonText(0, _T("�x�s"));
	
	// 3.2. Grid �t�γ]�w�ﶵ
	m_SetListGridImageList.Create(16, 16, ILC_COLOR16 | ILC_MASK, 1, 1);
	UINT arSetListGridImage[] = {IDB_BITMAP14, IDB_BITMAP13};
	for (int i=0; i<2; i++)
	{
		bmp.LoadBitmap(arSetListGridImage[i]);
		m_SetListGridImageList.Add(&bmp, RGB(255, 255, 255));
		bmp.DeleteObject();
	}
	m_SetListGrid.Create(WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_TABSTOP | WS_BORDER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, CRect(114, 41, rect.Width()-5, rect.Height()-42-35), &m_MainTab2, IDC_SETLISTGRID, NULL);
	m_SetListGrid.GetPaintManager()->SetColumnStyle(xtpReportColumnFlat);
	m_SetListGrid.GetPaintManager()->SetHeaderHeight(20);
	m_SetListGrid.GetReportHeader()->AllowColumnRemove(FALSE);
	m_SetListGrid.SetImageList(&m_SetListGridImageList);

	m_SetListGrid.AddColumn(new CXTPReportColumn(0, _T(""), 200, 1, -1, 0));
	m_SetListGrid.AddColumn(new CXTPReportColumn(1, _T(""), 400, 1, -1, 0));

	m_SetListGrid.AllowEdit(TRUE);
	m_SetListGrid.GetColumns()->GetAt(0)->SetEditable(FALSE);
	m_SetListGrid.GetColumns()->GetAt(1)->SetEditable(TRUE);
	m_SetListGrid.Populate();

	// 3.3. ToolBar �t�α���u��
	m_SetMenuToolBarImageList.Create(16, 16, ILC_COLOR16 | ILC_MASK, 1, 1);
	UINT arSetMenuToolBarImage[] = {IDB_BITMAP7, IDB_BITMAP9, IDB_BITMAP8};
	UINT arSetMenuToolBarBtn[] = {IDC_SETTOOLBARBUTTON1, IDC_SETTOOLBARBUTTON2, IDC_SETTOOLBARBUTTON3};
	for (int i=0; i<3; i++)
	{
		bmp.LoadBitmap(arSetMenuToolBarImage[i]);
		m_SetMenuToolBarImageList.Add(&bmp, RGB(255, 255, 255));
		bmp.DeleteObject();
	}
	m_SetMenuToolBar.CreateEx(&m_MainTab2, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, CRect(0, 0, 0, 0));
	m_SetMenuToolBar.SetButtons(arSetMenuToolBarBtn, 3);
	m_SetMenuToolBar.SetSizes(CSize(24, 24), CSize(16, 16));
	m_SetMenuToolBar.GetToolBarCtrl().SetImageList(&m_SetMenuToolBarImageList);
	m_SetMenuToolBar.MoveWindow(0, 0, 112, 40, 1);

	m_SetMenuToolBar.SetButtonStyle(0, TBBS_AUTOSIZE);
	m_SetMenuToolBar.SetButtonText(0, _T("�}�l"));
	m_SetMenuToolBar.SetButtonStyle(1, TBBS_AUTOSIZE);
	m_SetMenuToolBar.SetButtonText(1, _T("����"));
	m_SetMenuToolBar.SetButtonStyle(2, TBBS_AUTOSIZE);
	m_SetMenuToolBar.SetButtonText(2, _T("����"));
	
	// 3.4. Grid �t�γ]�w���
	m_SetMenuGridImageList.Create(16, 16, ILC_COLOR16 | ILC_MASK, 1, 1);
	
	m_SetMenuGrid.Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_OVERLAPPED | WS_TABSTOP | WS_BORDER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, CRect(0, 41, 112, rect.Height()-42-35), &m_MainTab2, IDC_SETMENUGRID, NULL);
	m_SetMenuGrid.GetPaintManager()->SetColumnStyle(xtpReportColumnFlat);
	m_SetMenuGrid.GetPaintManager()->SetHeaderHeight(20);
	m_SetMenuGrid.GetReportHeader()->AllowColumnRemove(FALSE);

	m_SetMenuGrid.AddColumn(new CXTPReportColumn(0, _T(""), 100, 1, -1, 0));
	m_SetMenuGrid.GetColumns()->GetAt(0)->SetAlignment(DT_CENTER);
	
	m_SetMenuGrid.AddRecord(new CDFSetMenuRecord(this, CString("�t �� �� �A")));
	m_SetMenuGrid.AddRecord(new CDFSetMenuRecord(this, CString("�t �� �] �w")));
	m_SetMenuGrid.AddRecord(new CDFSetMenuRecord(this, CString("�s �u �] �w")));
	m_SetMenuGrid.AddRecord(new CDFSetMenuRecord(this, CString("�� �� �] �w")));
	m_SetMenuGrid.AddRecord(new CDFSetMenuRecord(this, CString("�� �� �] �w")));
	m_SetMenuGrid.Populate();
	// 3.5.
 	// 3.6.


	// 4. [Tab3] Log �t�ά�����
	m_MainTab3.GetClientRect(&rect);
	
	// 4.1. ToolBar �M���u��
	m_LogToolBarImageList.Create(16, 16, ILC_COLOR16 | ILC_MASK, 1, 1);
	UINT arLogToolbarBtn[] = {IDC_LOGTOOLBARBUTTON1, 0, IDC_LOGTOOLBARFILTER1, IDC_LOGTOOLBARFILTER2, IDC_LOGTOOLBARFILTER3, IDC_LOGTOOLBARFILTER4, IDC_LOGTOOLBARFILTER5};

	bmp.LoadBitmap(IDB_BITMAP16);
	m_LogToolBarImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	bmp.LoadBitmap(IDB_BITMAP22);
	m_LogToolBarImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	bmp.LoadBitmap(IDB_BITMAP23);
	m_LogToolBarImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	bmp.LoadBitmap(IDB_BITMAP25);
	m_LogToolBarImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	bmp.LoadBitmap(IDB_BITMAP26);
	m_LogToolBarImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	bmp.LoadBitmap(IDB_BITMAP26);
	m_LogToolBarImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	m_LogToolBar.CreateEx(&m_MainTab3, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, CRect(0, 0, 0, 0));
	m_LogToolBar.SetButtons(arLogToolbarBtn, 7);
	m_LogToolBar.SetSizes(CSize(24, 24), CSize(16, 16));
	m_LogToolBar.GetToolBarCtrl().SetImageList(&m_LogToolBarImageList);
	m_LogToolBar.MoveWindow(0, 0, rect.Width(), 40, 1);

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

	// 4.2. Grid �����M��
	m_LogGridImageList.Create(16, 16, ILC_COLOR16 | ILC_MASK, 1, 1);

	bmp.LoadBitmap(IDB_BITMAP22);
	m_LogGridImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	bmp.LoadBitmap(IDB_BITMAP23);
	m_LogGridImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	bmp.LoadBitmap(IDB_BITMAP24);
	m_LogGridImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	bmp.LoadBitmap(IDB_BITMAP25);
	m_LogGridImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	bmp.LoadBitmap(IDB_BITMAP26);
	m_LogGridImageList.Add(&bmp, RGB(255, 255, 255));
	bmp.DeleteObject();

	m_LogGrid.Create(WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_TABSTOP | WS_BORDER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, CRect(0, 40, rect.Width()-5, rect.Height()-42-35), &m_MainTab3, IDC_LOGGRID, NULL);
	m_LogGrid.GetPaintManager()->SetColumnStyle(xtpReportColumnFlat);
	m_LogGrid.GetPaintManager()->SetGridStyle(FALSE, xtpReportGridNoLines);
	m_LogGrid.GetReportHeader()->AllowColumnRemove(FALSE);
	m_LogGrid.SetImageList(&m_LogGridImageList);

	m_LogGrid.AddColumn(new CXTPReportColumn(0, _T("�s��"), 50));
	m_LogGrid.AddColumn(new CXTPReportColumn(1, _T("����"), 100));
	m_LogGrid.AddColumn(new CXTPReportColumn(2, _T("���"), 80));
	m_LogGrid.AddColumn(new CXTPReportColumn(3, _T("�ɶ�"), 80));
	m_LogGrid.AddColumn(new CXTPReportColumn(4, _T("�T��"), 340));

	// 5. Initial
	m_MainTabCtrl.SetCurSel(0);
	OnTcnSelchangeTab1(NULL, NULL);

	// 6. Ready
	BeginApp();

	// 7. Go ��s�s�u���AUI+��s�]�w��UI
	SetTimer(IDC_TIMER, 1000, NULL);

	return TRUE;  // �Ǧ^ TRUE�A���D�z�ﱱ��]�w�J�I
}

BOOL CDDFileClientDlg::InitApp()
{
	theApp.GetIni();

	return TRUE;
}

BOOL CDDFileClientDlg::BeginApp()
{
	// 0:Syslog
	LoadSysLogClientDll();

	// 1:�t�ά����޲z��
	AcquireSRWLockExclusive(&m_lockServer);
	m_ptDFLog = new CDFLogMaster(CDFLogMaster::ThreadFunc, this);
	VERIFY(m_ptDFLog->CreateThread());
	
	// 2:�ɮ׺޲z��
	AcquireSRWLockExclusive(&m_lockServer);
	m_ptDFFile = new CDFFileMaster(CDFFileMaster::ThreadFunc, this);
	VERIFY(m_ptDFFile->CreateThread());

	// 3.�s�u�޲z��
	for (UINT i=0; i<10; i++)
	{
		if (theApp.m_chServerIP[i][0]==0 && theApp.m_uServerPort[i]==0)
			continue;

		AcquireSRWLockExclusive(&m_lockServer); 
		m_ptDFNet[i] = new CDFNetMaster(CDFNetMaster::ThreadFunc, this, i, theApp.m_chServerIP[i], theApp.m_uServerPort[i]);
		VERIFY(m_ptDFNet[i]->CreateThread());
	}

	AcquireSRWLockExclusive(&m_lockServer);
	ReleaseSRWLockExclusive(&m_lockServer);
	
	return TRUE;
}

void CDDFileClientDlg::EndApp()
{
	DWORD nThreadID;
	CString strLog("");

	// 1. �T�T UI�w�ɧ�s�p�ɾ�
	KillTimer(IDC_TIMER);
	
	// 2. �T�T �s�u�޲z��
	for (UINT i=0; i<10; i++)
	{
		if (m_ptDFNet[i])
		{
			nThreadID = m_ptDFNet[i]->m_nThreadID;
			m_ptDFNet[i]->ForceStopMaster();
			m_ptDFNet[i]->StopThread();
			WaitForSingleObject(m_ptDFNet[i]->m_hThread, INFINITE);
			strLog.Format("[�{������]�����s�u�޲z��(Thread:%d)", nThreadID);
			m_ptDFLog->DFLogSend(strLog, DF_SEVERITY_INFORMATION);
			delete m_ptDFNet[i];
			m_ptDFNet[i] = NULL;
		}
	}

	// 3. �T�T �ɮ׺޲z��
	if (m_ptDFFile)
	{
		nThreadID = m_ptDFFile->m_nThreadID;
		m_ptDFFile->StopThread();
		WaitForSingleObject(m_ptDFFile->m_hThread, INFINITE);
		strLog.Format("[�{������]�����ɮ׺޲z��(Thread:%d)", nThreadID);
		m_ptDFLog->DFLogSend(strLog, DF_SEVERITY_INFORMATION);
		delete m_ptDFFile;
		m_ptDFFile = NULL;
	}

	// 4. �T�T �����޲z��
	if (m_ptDFLog)
	{
		nThreadID = m_ptDFLog->m_nThreadID;
		m_ptDFLog->StopThread();
		WaitForSingleObject(m_ptDFLog->m_hThread, INFINITE);
		delete m_ptDFLog;
		m_ptDFLog = NULL;
	}
}

void CDDFileClientDlg::SaveIni()
{
	theApp.SetIni();
}

CString	CDDFileClientDlg::ConvertUINT64toString(UINT64 val)
{
	CString strRet("");
	CString strUnit("");
	UINT64 dotL = 0;
	UINT64 dotR = 0;
	UINT64 base = 0;
	if (val >= 1024*1024*1024)	// GB
	{
		strUnit = "GB";
		base = 1024*1024*1024;
		dotL = val / base;
		dotR = ((val % base) * 100) / base;
	}
	else if (val >= 1024*1024)	// MB
	{
		strUnit = "MB";
		base = 1024*1024;
		dotL = val / base;
		dotR = ((val % base) * 100) / base;
	}
	else if (val >= 1024)		// KB
	{
		strUnit = "KB";
		base = 1024;
		dotL = val / base;
		dotR = ((val % base) * 100) / base;
	}
	else						// B
	{
		strUnit = "B";
		dotL = val;
		dotR = 0;
	}

	strRet.Format("%I64d.%I64d%s", dotL, dotR, strUnit);
	return strRet;
}

void CDDFileClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CDDFileClientDlg::OnPaint()
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

void CDDFileClientDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	if(m_MainTabCtrl.m_hWnd == NULL)
		return;      // Return if window is not created yet.

	//RECT rect;
	CRect rect;
	GetClientRect(&rect);
	m_MainTabCtrl.MoveWindow(&rect, TRUE);
	OnTcnSelchangeTab1(NULL, NULL);

	// ����1
	//m_NetToolBar
	m_NetGrid.MoveWindow(0, 41, 182, rect.bottom-41-26, 1);
	m_FileToolBar1.MoveWindow(182, 0, rect.right-182, 24, 1);
	m_FileToolBar2.MoveWindow(182, 25, rect.right-182, 24, 1);
	m_FileGrid.MoveWindow(184, 50, rect.right-184-5, rect.bottom-50-26, 1);
	
	// ����2
	//m_SetMenuToolBar
	m_SetMenuGrid.MoveWindow(0, 41, 112, rect.bottom-41-26, 1);
	m_SetListToolBar.MoveWindow(112, 0, rect.right-112, 40, 1);
	m_SetListGrid.MoveWindow(114, 41, rect.right-114-5, rect.bottom-41-26, 1);
	
	// ����3
	m_LogToolBar.MoveWindow(0, 0, rect.right, 40, 1);
	m_LogGrid.MoveWindow(0, 41, rect.right-5, rect.bottom-41-26, 1);
}

// ��ϥΪ̩즲�̤p�Ƶ����ɡA
// �t�ΩI�s�o�ӥ\����o�����ܡC
HCURSOR CDDFileClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CDDFileClientDlg::OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: �b���[�J����i���B�z�`���{���X
	if (pResult)
		*pResult = 0;

	CRect rTab, rItem;
	m_MainTabCtrl.GetItemRect(0, &rItem);
	m_MainTabCtrl.GetClientRect(&rTab);
	
	int x = rItem.left;
	int y = rItem.bottom+1;
	int cx = rTab.right-rItem.left-3;
	int cy = rTab.bottom-y-2;
	int tab = m_MainTabCtrl.GetCurSel();

	m_MainTab1.SetWindowPos(NULL, x, y, cx, cy, SWP_HIDEWINDOW);
	m_MainTab2.SetWindowPos(NULL, x, y, cx, cy, SWP_HIDEWINDOW);
	m_MainTab3.SetWindowPos(NULL, x, y, cx, cy, SWP_HIDEWINDOW);

	switch(tab)
	{
	case 0:
		m_MainTab1.SetWindowPos(NULL, x, y, cx, cy, SWP_SHOWWINDOW);
		break;
	case 1:
		m_MainTab2.SetWindowPos(NULL, x, y, cx, cy, SWP_SHOWWINDOW);
		break;
	case 2:
		m_MainTab3.SetWindowPos(NULL, x, y, cx, cy, SWP_SHOWWINDOW);
		break;
	default:
		break;
	}
}

void CDDFileClientDlg::OnTimer(UINT_PTR nIDEvent)
{
	// ��s�s�u���A��
	//for (int i=0; i<10; i++)
	//{

	//}

	//m_NetGrid.GetRecords()->RemoveAll();

	//if (m_NetUpdateFlag)
	//{
	int nRows = m_NetGrid.GetRecords()->GetCount()-1;
	int nCols = m_NetGrid.GetColumns()->GetCount()-1;

	for (UINT i=0; i<10; i++)
	{
		if (theApp.m_chServerIP[i][0] != 0)
		{
			CString strHost;
			strHost.Format("%s:%d", theApp.m_chServerIP[i], theApp.m_uServerPort[i]);
			if (m_NetGrid.GetRecords()->FindRecordItem(i, i, 0, 0, 0, 0, strHost, xtpReportTextSearchExactStart) == NULL)
			{
				//m_NetGrid.GetRecords()->RemoveAll();
				m_NetGrid.AddRecord(new CDFNetRecord(this, i, strHost));
				m_NetGrid.Populate();
			}
		}
	}
	m_NetGrid.RedrawControl();
	
	//}

	// ��s�t�γ]�w��
	theApp.m_uServerStatus = (m_ptDFNet != NULL) ? 0 : 1;
	int select = m_SetMenuGrid.GetSelectedRows()->GetAt(0)->GetIndex();
	if (select == 0)
	{
		m_SetListGrid.GetRecords()->GetAt(0)->GetItem(1)->SetCaption(CString(theApp.m_uServerStatus==0 ? "�A�ȥ��`" : "�A�Ȱ���"));
		m_SetListGrid.GetRecords()->GetAt(0)->GetItem(1)->SetIconIndex(theApp.m_uServerStatus==0 ? 0 : 1);
		m_SetListGrid.GetColumns()->GetAt(1)->SetEditable(FALSE);
		m_SetListGrid.RedrawControl();
	}

	// 
	//if (m_ptDFNet && m_ptDFNet->IsStop())
	//{
	//	delete m_ptDFNet;
	//	m_ptDFNet = NULL;
	//}

	//CString strLog("");
	//if (m_ptDFNet == NULL && theApp.m_uServerStatus == 1 && theApp.m_bAutoReconnect && !m_bManualStop)
	//{
	//	strLog.Format("[�۰ʭ��s�s�u]���s�إ߳s�u...");
	//	m_ptDFLog->DFLogSend(strLog, DF_SEVERITY_INFORMATION);

	//	Sleep(5000);

	//	AcquireSRWLockExclusive(&m_lockServer);
	//	m_ptDFNet = new CDFNetMaster(CDFNetMaster::ThreadFunc, this, 0, theApp.m_chServerIP[0], theApp.m_uServerPort[0]);
	//	VERIFY(m_ptDFNet->CreateThread());

	//	strLog.Format("[�۰ʭ��s�s�u]���ҳs�u�޲z��(Thread:%d)", m_ptDFNet->m_nThreadID);
	//	m_ptDFLog->DFLogSend(strLog, DF_SEVERITY_INFORMATION);
	//}
}



