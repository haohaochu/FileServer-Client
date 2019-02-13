#pragma once


// CNotifyDlg 對話方塊
#include "resource.h"

#define		ONE_TIMER			WM_APP + 2001

class CNotifyDlg : public CDialog
{
	DECLARE_DYNAMIC(CNotifyDlg)

public:
	CNotifyDlg(CWnd* pParent = NULL);   // 標準建構函式
	virtual ~CNotifyDlg();

// 對話方塊資料
	enum { IDD = IDD_NOTIFYBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支援
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT_PTR nIDEvent);

	DECLARE_MESSAGE_MAP()

public:
	CString m_strImage;
	CString	m_strMessage;
	int		m_nTimer;
};
