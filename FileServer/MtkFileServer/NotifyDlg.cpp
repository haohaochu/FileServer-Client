// NotifyDlg.cpp : ��@��
//

#include "stdafx.h"
#include "NotifyDlg.h"
//#include ".\Pats2mtk.h"


// CNotifyDlg ��ܤ��

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


// CNotifyDlg �T���B�z�`��

BOOL CNotifyDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  �b���[�J�B�~����l��
	HICON ico = AfxGetApp()->LoadIcon(IDI_ICON1);
	CStatic *img = (CStatic*)GetDlgItem(IDC_STATIC_IMAGE);
	img->SetIcon(ico);
	
	CString	strMsg;
	strMsg.Format("�{���N�b %2d �������", m_nTimer);
	SetWindowText(strMsg);

	// �w�ɧ�s�������Y
	SetTimer(ONE_TIMER, 1000, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX �ݩʭ����Ǧ^ FALSE
}

void CNotifyDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: �b���[�J�z���T���B�z�`���{���X�M (��) �I�s�w�]��
	if ( nIDEvent == ONE_TIMER )
	{
		CString	strMsg;
		
		strMsg.Format("�{���N�b %2d �������", --m_nTimer);
		SetWindowText(strMsg);

		if ( m_nTimer <= 0 )
			CDialog::OnOK();
	}

	CDialog::OnTimer(nIDEvent);
}