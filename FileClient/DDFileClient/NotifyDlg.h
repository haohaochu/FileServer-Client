#pragma once


// CNotifyDlg ��ܤ��
#include "resource.h"

#define		ONE_TIMER			WM_APP + 2001

class CNotifyDlg : public CDialog
{
	DECLARE_DYNAMIC(CNotifyDlg)

public:
	CNotifyDlg(CWnd* pParent = NULL);   // �зǫغc�禡
	virtual ~CNotifyDlg();

// ��ܤ�����
	enum { IDD = IDD_NOTIFYBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �䴩
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT_PTR nIDEvent);

	DECLARE_MESSAGE_MAP()

public:
	CString m_strImage;
	CString	m_strMessage;
	int		m_nTimer;
};
