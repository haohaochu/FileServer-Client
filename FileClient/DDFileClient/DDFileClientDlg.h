
// DDFileClientDlg.h : 標頭檔
//

#pragma once
#include "DFFile.h"
#include "DFNet.h"
#include "DFLog.h"


class CTabDlg1 : public CDialogEx
{
public:
	CTabDlg1(CWnd* pParent = NULL);	// 標準建構函式

	enum { IDD = IDD_DIALOG1 };

protected:
	afx_msg void OnAddServer();
	afx_msg void OnDelServer();
	afx_msg void OnStartServer();
	afx_msg void OnEndServer();
	afx_msg void OnRestartServer();
	afx_msg void OnReadonlyFile();
	afx_msg void OnRefreshFile();
	afx_msg void OnChangeFilter();	
	afx_msg void OnClearFilter();

	afx_msg void OnOK();
	afx_msg void OnCancel();
	afx_msg void OnDestroy();

	DECLARE_MESSAGE_MAP()
};


class CTabDlg2 : public CDialogEx
{
public:
	CTabDlg2(CWnd* pParent = NULL);	// 標準建構函式

	enum { IDD = IDD_DIALOG2 };

protected:
	afx_msg void OnStartServer();
	afx_msg void OnStopServer();
	afx_msg void OnRestartServer();
	afx_msg void OnSave();
	afx_msg void OnChangeMenu(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnChangeSettings(NMHDR* pNMHDR, LRESULT* pResult);

	afx_msg void OnOK();
	afx_msg void OnCancel();
	afx_msg void OnDestroy();

	DECLARE_MESSAGE_MAP()
};


class CTabDlg3 : public CDialogEx
{
public:
	CTabDlg3(CWnd* pParent = NULL);	// 標準建構函式

	enum { IDD = IDD_DIALOG3 };

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


class CDDFileClientDlg;

// class CDFNetRecord
class CDFNetRecord : public CXTPReportRecord
{
public:
	CDFNetRecord(CDDFileClientDlg* master, UINT index, CString& text);
	virtual ~CDFNetRecord(void);

	virtual void	CreateItems();
	virtual void	GetItemMetrics(XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics);

	CDDFileClientDlg*	m_DlgMaster;
	UINT				m_uServerID;
};


// class CDFSetMenuRecord
// - 設定選單
class CDFSetMenuRecord : public CXTPReportRecord
{
public:
	CDFSetMenuRecord(CDDFileClientDlg* master, CString& text);
	virtual ~CDFSetMenuRecord(void);

	virtual void	CreateItems();
	virtual void	GetItemMetrics(XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics);

	CDDFileClientDlg*	m_SetMaster;
};


// class CDFSetListRecord
// - 設定物件
class CDFSetListRecord : public CXTPReportRecord
{
public:
	CDFSetListRecord();
	CDFSetListRecord(CDDFileClientDlg* master, CString& text, CString& data);
	CDFSetListRecord(CDDFileClientDlg* master, CString& text, UINT data);
	CDFSetListRecord(CDDFileClientDlg* master, CString& text, CArray<CString, CString&>& data);
	virtual ~CDFSetListRecord(void);

	virtual void	CreateItems();
	virtual void	GetItemMetrics(XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics);
	BOOL			UpdateRecord(int idx, CString& data);
	BOOL			UpdateRecord(int idx, UINT data);

	CDDFileClientDlg*	m_SetMaster;
};


// CDDFileClientDlg 對話方塊
class CDDFileClientDlg : public CDialogEx
{
// 建構
public:
	CDDFileClientDlg(CWnd* pParent = NULL);	// 標準建構函式

// 對話方塊資料
	enum { IDD = IDD_DDFILECLIENT_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支援


// 程式碼實作
protected:
	HICON m_hIcon;

	// 產生的訊息對應函式
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
	CDFFileMaster*		m_ptDFFile;
	CDFLogMaster*		m_ptDFLog;
	CDFNetMaster*		m_ptDFNet[10];
	SRWLOCK				m_lockServer;
	BOOL				m_bManualStop;

public:
	// 主介面元件
	CImageList			m_MainTabImageList;
	CXTPTabCtrl			m_MainTabCtrl;
	CTabDlg1			m_MainTab1;
	CTabDlg2			m_MainTab2;
	CTabDlg3			m_MainTab3;
	CFont				m_Font;

	// 分頁1
	BOOL				m_NetUpdateFlag;
	CImageList			m_NetToolBarImageList;
	CToolBar			m_NetToolBar;
	CImageList			m_NetGridImageList;
	CXTPReportControl	m_NetGrid;
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

	// 分頁2
	CImageList			m_SetMenuToolBarImageList;
	CToolBar			m_SetMenuToolBar;
	CImageList			m_SetMenuGridImageList;
	CXTPReportControl	m_SetMenuGrid;
	CImageList			m_SetListToolBarImageList;
	CToolBar			m_SetListToolBar;
	CImageList			m_SetListGridImageList;
	CXTPReportControl	m_SetListGrid;

	// 分頁3
	CImageList			m_LogToolBarImageList;
	CToolBar			m_LogToolBar;
	CImageList			m_LogGridImageList;
	CXTPReportControl	m_LogGrid;

	//
	void				SaveIni();

	//
	CString				ConvertUINT64toString(UINT64 val);

	afx_msg void OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	
	afx_msg void OnOK();
	afx_msg void OnCancel();
	afx_msg void OnDestroy();
};


