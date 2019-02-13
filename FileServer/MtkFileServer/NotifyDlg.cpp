// NotifyDlg.cpp : 實作檔
//

#include "stdafx.h"
#include "NotifyDlg.h"
//#include ".\Pats2mtk.h"


// CNotifyDlg 對話方塊

IMPLEMENT_DYNAMIC(CNotifyDlg, CDialog)

CNotifyDlg::CNotifyDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNotifyDlg::IDD, pParent)
	, m_strMessage(_T(""))
	, m_nTimer(10)
{

}

CNotifyDlg::~CNotifyDlg()
{
}

void CNotifyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_STATIC_IMAGE, m_strImage);
	DDX_Text(pDX, IDC_STATIC_MSG, m_strMessage);
}


BEGIN_MESSAGE_MAP(CNotifyDlg, CDialog)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CNotifyDlg 訊息處理常式

BOOL CNotifyDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此加入額外的初始化
	HICON ico = AfxGetApp()->LoadIcon(IDI_ICON1);
	CStatic *img = (CStatic*)GetDlgItem(IDC_STATIC_IMAGE);
	img->SetIcon(ico);
	
	CString	strMsg;
	strMsg.Format("程式將在 %2d 秒後關閉", m_nTimer);
	SetWindowText(strMsg);

	// 定時更新視窗抬頭
	SetTimer(ONE_TIMER, 1000, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX 屬性頁應傳回 FALSE
}

void CNotifyDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此加入您的訊息處理常式程式碼和 (或) 呼叫預設值
	if ( nIDEvent == ONE_TIMER )
	{
		CString	strMsg;
		
		strMsg.Format("程式將在 %2d 秒後關閉", --m_nTimer);
		SetWindowText(strMsg);

		if ( m_nTimer <= 0 )
			CDialog::OnOK();
	}

	CDialog::OnTimer(nIDEvent);
}