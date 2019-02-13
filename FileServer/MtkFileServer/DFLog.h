
// DFLog.h : 檔案
//

#pragma once

#ifndef __AFXWIN_H__
	#error "對 PCH 包含此檔案前先包含 'stdafx.h'"
#endif

#include "resource.h"		// 主要符號
#include "mythread.h"


// definition
//
#define MAX_LOG_COLUMN 5
#define MAX_LOG_ROW 1000
#define MAX_LOG_MSGLEN 256


// enumeration
//
typedef enum SEVERITY
{
	DF_SEVERITY_INFORMATION	= 1,
	DF_SEVERITY_DEBUG		= 2,
	DF_SEVERITY_WARNING		= 3,
	DF_SEVERITY_HIGH		= 4,
	DF_SEVERITY_DISASTER	= 5,
} SEVERITY;


// struct CDFLog
// - Log
typedef struct CDFLog
{
	SEVERITY		severity;
	UINT			index;
	SYSTEMTIME		time;
	char			msg[MAX_LOG_MSGLEN];
} LOG;


// class CDFLogRecord
// - Log物件
class CDFLogMaster;
class CDFLogRecord : public CXTPReportRecord
{
public:
	CDFLogRecord(CDFLogMaster* master);
	virtual ~CDFLogRecord(void);

	virtual void	CreateItems();
	virtual void	GetItemMetrics(XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics);
//	virtual void	DoPropExchange(CXTPPropExchange* pPX);

	CDFLogMaster*	m_LogMaster;
};


// class CDFLogMaster
// - Log管理員
class CDFLogMaster : public CMyThread
{
public:
	CDFLogMaster(AFX_THREADPROC pfnThreadProc, LPVOID param);
	~CDFLogMaster(void);

protected:
	virtual void	Go(void);

public:
	void			DFLogSend(CString& msg, SEVERITY severity);
	LOG*			m_LogRecord[MAX_LOG_ROW];
	UINT			m_LogIndex;
	SRWLOCK			m_LogLock;
	UINT			m_LogFilter;

	void			ClearAll();
	void			SetFilterOn(UINT i);
	void			SetFilterOff(UINT i);
};








