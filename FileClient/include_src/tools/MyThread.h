// MyThread.h: interface for the CMyThread class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MYTHREAD_H__CF40E77D_B3FC_4D65_928D_5FE588B5E0F4__INCLUDED_)
#define AFX_MYTHREAD_H__CF40E77D_B3FC_4D65_928D_5FE588B5E0F4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CMyThread : public CWinThread
{
public:
	CMyThread(AFX_THREADPROC pfnThreadProc,LPVOID param);
	static UINT		ThreadFunc(LPVOID param);
	void			StopThread(void) {m_bStop = TRUE;};

protected:
	LPVOID			GetParentsHandle(void) {return pParents;};
	BOOL			IsStop(void)  { return m_bStop;};
	void			EndThread(void)	{m_bStop = TRUE;};

private:
	virtual void	Go() = 0;

	LPVOID			pParents;
	BOOL			m_bStop;
};

#endif // !defined(AFX_MYTHREAD_H__CF40E77D_B3FC_4D65_928D_5FE588B5E0F4__INCLUDED_)
