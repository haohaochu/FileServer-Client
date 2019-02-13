
// MtkFileServerDlg.h : ���Y��
//

#pragma once
#include "afxcmn.h"
#include "gridctrl.h"
#include "MtkFile.h"
#include "DFLog.h"
#include "DFNet.h"


// SortSlave
typedef struct CSortSlave
{ 	
	char					m_key[24];
	char					m_data[256];
} SORTSLAVE;


class CTabDlg1 : public CDialogEx
{
public:
	CTabDlg1(CWnd* pParent = NULL);	// �зǫغc�禡

	enum { IDD = IDD_DIALOG1 };

protected:
	afx_msg void OnReadonlyFile();
	afx_msg void OnRefleshFile();	
	afx_msg void OnClearFilter();
	afx_msg void OnChangeFilter();

	afx_msg void OnOK();
	afx_msg void OnCancel();
	afx_msg void OnDestroy();

	DECLARE_MESSAGE_MAP()
};


class CTabDlg2 : public CDialogEx
{
public:
	CTabDlg2(CWnd* pParent = NULL);	// �зǫغc�禡

	enum { IDD = IDD_DIALOG2 };

protected:
	afx_msg void OnStartServer();
	afx_msg void OnStopServer();	
	afx_msg void OnDisconnect();
	afx_msg void OnDebug();
	afx_msg void OnSelectFileList(NMHDR* pNMHDR, LRESULT* pResult);

	afx_msg void OnOK();
	afx_msg void OnCancel();
	afx_msg void OnDestroy();

	DECLARE_MESSAGE_MAP()
};


class CTabDlg3 : public CDialogEx
{
public:
	CTabDlg3(CWnd* pParent = NULL);	// �зǫغc�禡

	enum { IDD = IDD_DIALOG3 };
	
	void RefreshSettingPage();

protected:
	afx_msg void OnStartServer();
	afx_msg void OnStopServer();	
	afx_msg void OnSave();
	afx_msg void OnChangeMenu(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnChangeSettings(NMHDR* pNMHDR, LRESULT* pResult);
	
	afx_msg void OnOK();
	afx_msg void OnCancel();
	afx_msg void OnDestroy();	
	
	DECLARE_MESSAGE_MAP()
};


class CTabDlg4 : public CDialogEx
{
public:
	CTabDlg4(CWnd* pParent = NULL);	// �зǫغc�禡

	enum { IDD = IDD_DIALOG4 };

protected:
	afx_msg void OnClear();
	afx_msg void OnFilter1();
	afx_msg void OnFilter2();
	afx_msg void OnFilter3();
	afx_msg void OnFilter4();
	afx_msg void OnFilter5();

	afx_msg void OnOK();
	afx_msg void OnCancel();
	afx_msg void OnDestroy();	

	UINT	uFilter1;
	UINT	uFilter2;
	UINT	uFilter3;
	UINT	uFilter4;
	UINT	uFilter5;

	DECLARE_MESSAGE_MAP()
};


// class CDFSetMenuRecord
// - �]�w��檫��
class CMtkFileServerDlg;
class CDFNetSlaveRecord : public CXTPReportRecord
{
public:
	CDFNetSlaveRecord(CMtkFileServerDlg* master);
	virtual ~CDFNetSlaveRecord(void);

	virtual void	CreateItems();
	virtual void	GetItemMetrics(XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics);
//	virtual void	DoPropExchange(CXTPPropExchange* pPX);

	CMtkFileServerDlg*	m_SetMaster;
};

class CDFSetMenuRecord : public CXTPReportRecord
{
public:
	CDFSetMenuRecord(CMtkFileServerDlg* master, CString& text);
	virtual ~CDFSetMenuRecord(void);

	virtual void	CreateItems();
	virtual void	GetItemMetrics(XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics);
//	virtual void	DoPropExchange(CXTPPropExchange* pPX);

	CMtkFileServerDlg*	m_SetMaster;
};

// class CDFSetListRecord
// - �]�w����
class CDFSetListRecord : public CXTPReportRecord
{
public:
	CDFSetListRecord();
	CDFSetListRecord(CMtkFileServerDlg* master, CString& text, CString& data);
	CDFSetListRecord(CMtkFileServerDlg* master, CString& text, UINT data);
	virtual ~CDFSetListRecord(void);

	virtual void	CreateItems();
	virtual void	GetItemMetrics(XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics);
	BOOL			UpdateRecord(int idx, CString& data);
	BOOL			UpdateRecord(int idx, UINT data);
//	virtual void	DoPropExchange(CXTPPropExchange* pPX);

	CMtkFileServerDlg*	m_SetMaster;
};

// CMtkFileServerDlg ��ܤ��
class CMtkFileServerDlg : public CDialogEx
{
// �غc
public:
	CMtkFileServerDlg(CWnd* pParent = NULL);	// �зǫغc�禡

// ��ܤ�����
	enum { IDD = IDD_MTKFILESERVER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV �䴩


// �{���X��@
protected:
	HICON m_hIcon;

	// ���ͪ��T�������禡
	virtual BOOL OnInitDialog();
	
	BOOL InitApp();
	BOOL BeginApp();
	void EndApp();

	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

public:
	//
	CMtkFileMaster*		m_ptFileThread;
	CDFLogMaster*		m_ptDFLog;
	CDFNetMaster*		m_ptDFNet;
	CDFNetSlave*		m_ptDFDebug;
	SRWLOCK				m_lockServer;

	// �R�A�]�w
	UINT				m_uServerStatus;
/*	
	char				m_chFileDir[256];
	UINT				m_uListenPort;
	char				m_chServerIP[16];
	UINT				m_uServerPort;
	char				m_chZabbixIP[16];
	UINT				m_uZabbixPort;
	char				m_chZabbixHost[50];
	char				m_chZabbixKey[50];
	UINT				m_uZabbixFreq;

	BOOL				m_bNetDefaultTransferAll;
	BOOL				m_bNetDefaultCompressAll;

	BOOL				m_bLogAllowDebug;
*/
	// �D����
	CImageList			m_MainTabImageList;
	CXTPTabCtrl			m_tabctrlMain;
	CTabDlg1			m_dlgTab1;
	CTabDlg2			m_dlgTab2;
	CTabDlg3			m_dlgTab3;
	CTabDlg4			m_dlgTab4;
	CFont				m_Font;

	// ����1
	CImageList			m_FileToolBarImageList;
	CToolBar			m_FileToolBar1;
	CToolBar			m_FileToolBar2;
	CString				m_FileFilter;
	CStatic				m_FileFilterText;
	CEdit				m_FileFilterEdit;
	CButton				m_FileFilterClearButton;
	CStatic				m_FileVersionText;
	CImageList			m_FileGridImageList;
	CXTPReportControl	m_FileGrid;

	// ����2
	CImageList			m_NetToolBarImageList;
	CToolBar			m_NetToolBar;
	CImageList			m_NetMenuImageList;
	CXTPReportControl	m_NetMenuGrid;
	CImageList			m_NetListImageList;
	CXTPReportControl	m_NetListGrid;
	CImageList			m_NetFileImageList;
	CXTPReportControl	m_NetFileGrid;

	UINT				m_NetSlaveCount;
	SORTSLAVE*			m_NetSlaveList;
	SRWLOCK				m_NetSlaveLock;

	// ����3
	CImageList			m_SetToolBarImageList;
	CToolBar			m_SetToolBar;
	CImageList			m_SetMenuImageList;
	CXTPReportControl	m_SetMenuGrid;
	CImageList			m_SetListImageList;
	CXTPReportControl	m_SetListGrid;

	// ����4
	CImageList			m_LogToolBarImageList;
	CToolBar			m_LogToolBar;
	CImageList			m_LogGridImageList;
	CXTPReportControl	m_LogGrid;
	
	// 
	void SaveIni();

	// 
	static int fnFileCompare(const void* a, const void* b);
	afx_msg void OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	
	afx_msg void OnOK();
	afx_msg void OnCancel();
	afx_msg void OnDestroy();
	// CALLBACK
};
