// MyThread.cpp: implementation of the CMyThread class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MyThread.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMyThread::CMyThread(AFX_THREADPROC pfnThreadProc,LPVOID param)
 : CWinThread(pfnThreadProc,NULL)
 , pParents(param)			// Owner Point
 , m_bStop(FALSE)			// Stop Flag
{
	m_bAutoDelete = FALSE;		// Delete CxxxThread Class Than Delete the Thread
	m_pThreadParams = this;		// CxxxThread Point
}

UINT CMyThread::ThreadFunc(LPVOID n)
{
	CMyThread*	pThread = (CMyThread*)n;
	pThread->Go();
	return 0;
}

/***********************************
// virtual function
void CMyThread::Go()
{
}
***********************************/
