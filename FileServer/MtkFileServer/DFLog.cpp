
// DFSLog.cpp : 定義檔案類別。
//

#include "stdafx.h"
#include "DFLog.h"
#include "MtkFileServerDlg.h"
#include "..\include_src\syslog\syslogclient.h"


// CDFLogRecord class implementation
// 
CDFLogRecord::CDFLogRecord(CDFLogMaster* master)
: m_LogMaster(master) 
{
	CreateItems();
}

/*virtual */CDFLogRecord::~CDFLogRecord(void)
{

}

/*virtual */void CDFLogRecord::CreateItems()
{
	AddItem(new CXTPReportRecordItemText());
	AddItem(new CXTPReportRecordItemText());
	AddItem(new CXTPReportRecordItemText());
	AddItem(new CXTPReportRecordItemText());
	AddItem(new CXTPReportRecordItemText());

	return;
}

/*virtual */void CDFLogRecord::GetItemMetrics(XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics)
{
	CXTPReportColumnOrder *pSortOrder = pDrawArgs->pControl->GetColumns()->GetSortOrder();

	BOOL bDecreasing = pSortOrder->GetCount() > 0 && pSortOrder->GetAt(0)->IsSortedDecreasing();

	CString strColumn = pDrawArgs->pColumn->GetCaption();
	int nIndexCol = pDrawArgs->pColumn->GetItemIndex();
	int nIndexRow = pDrawArgs->pRow->GetIndex();
	int nCount = pDrawArgs->pControl->GetRows()->GetCount();

	if (m_LogMaster)
	{
		AcquireSRWLockExclusive(&m_LogMaster->m_LogLock);
		LOG* log = m_LogMaster->m_LogRecord[(m_LogMaster->m_LogIndex-nIndexRow-1) % MAX_LOG_ROW];

		if (log == NULL)
		{
			pItemMetrics->strText.Format("%s", "");
			ReleaseSRWLockExclusive(&m_LogMaster->m_LogLock);
			return;
		}
		
		switch (nIndexCol)
		{
		case 0: // 編號
			{
				pItemMetrics->strText.Format("%d", log->index);
				break;
			}
		case 1: // 類型
			{
				if (log->severity == DF_SEVERITY_INFORMATION)
				{
					pItemMetrics->strText.Format("%s", "系統資訊");
					pItemMetrics->nItemIcon = 2; // 藍色
				}
				else if (log->severity == DF_SEVERITY_WARNING)
				{
					pItemMetrics->strText.Format("%s", "系統警告");
					pItemMetrics->nItemIcon = 1; // 黃色
				}
				else if (log->severity == DF_SEVERITY_DEBUG)
				{
					pItemMetrics->strText.Format("%s", "除錯資訊");
					pItemMetrics->nItemIcon = 3; // 綠色
				}
				else if (log->severity == DF_SEVERITY_HIGH)
				{
					pItemMetrics->strText.Format("%s", "重要警告");
					pItemMetrics->nItemIcon = 4; // 紅色
				}
				else if (log->severity == DF_SEVERITY_DISASTER)
				{
					pItemMetrics->strText.Format("%s", "災難警告");
					pItemMetrics->nItemIcon = 4; // 紅色
				}
				else
				{
					pItemMetrics->strText.Format("%s", "UNKNOWN");
				}
				break;
			}
		case 2: // 日期
			{
				pItemMetrics->strText.Format("%04d-%02d-%02d", log->time.wYear, log->time.wMonth, log->time.wDay);
				break;
			}
		case 3: // 時間
			{
				pItemMetrics->strText.Format("%02d:%02d:%02d", log->time.wHour, log->time.wMinute, log->time.wSecond);
				break;
			}
		case 4: // 訊息
			{
				pItemMetrics->strText.Format("%s", log->msg);
				break;
			}
		default:
			{
				pItemMetrics->strText.Format("%s", "unknown");
				break;
			}
		}

		ReleaseSRWLockExclusive(&m_LogMaster->m_LogLock);
	}

	return;
}


// CDFLogMaster class implementation
//
CDFLogMaster::CDFLogMaster(AFX_THREADPROC pfnThreadProc, LPVOID param)
: CMyThread(pfnThreadProc, param) 
, m_LogIndex(0)
, m_LogFilter(0)
{
	InitializeSRWLock(&m_LogLock);
	memset(m_LogRecord, NULL, sizeof(LOG*)*MAX_LOG_ROW);
}

CDFLogMaster::~CDFLogMaster(void)
{
	for (int i=0; i<MAX_LOG_ROW; i++)
		if (m_LogRecord[i]) delete m_LogRecord[i];
}

/*virtual */void CDFLogMaster::Go(void)
{
	UINT index = 0;
	CMtkFileServerDlg* parant = (CMtkFileServerDlg*)this->GetParentsHandle();

	ReleaseSRWLockExclusive(&parant->m_lockServer);

	parant->m_LogGrid.SetVirtualMode(new CDFLogRecord(this), MAX_LOG_ROW);
	parant->m_LogGrid.Populate();
	
	while (!IsStop())
	{
		AcquireSRWLockExclusive(&m_LogLock);
		BOOL bRedraw = (m_LogIndex != index);
		ReleaseSRWLockExclusive(&m_LogLock);
		
		if (bRedraw)
		{
			parant->m_LogGrid.SetTopRow(0);
			parant->m_LogGrid.RedrawControl();
		}

		index = m_LogIndex;
		Sleep(1000);
	}

	EndThread();
	return;
}

/*public */void CDFLogMaster::DFLogSend(CString& msg, SEVERITY severity)
{
	AcquireSRWLockExclusive(&m_LogLock);
	
	// 過濾機制
	if (m_LogFilter && (m_LogFilter & (1<<(severity-1))) == 0)
	{
		ReleaseSRWLockExclusive(&m_LogLock);
		return;
	}

	// 防爆炸機制
	if (m_LogIndex >= 100000000) m_LogIndex = 0;

	UINT index = m_LogIndex % MAX_LOG_ROW;
	LOG* log = m_LogRecord[index];
	if (log == NULL)
	{
		log = new LOG();
		m_LogRecord[index] = log;
	}

	// 編號
	log->index = m_LogIndex;
	
	// 層級
	log->severity = severity;
	
	// 時間
	GetLocalTime(&log->time);

	// 訊息
	memset(log->msg, NULL, MAX_LOG_MSGLEN);
	memcpy(log->msg, msg, msg.GetLength());

	m_LogIndex++;

	// Syslog
	if (severity >= DF_SEVERITY_HIGH)
		LogSend(log->msg, log->severity);

	ReleaseSRWLockExclusive(&m_LogLock);
}

/*public */void CDFLogMaster::ClearAll()
{
	AcquireSRWLockExclusive(&m_LogLock);

	for (int i=0; i<MAX_LOG_ROW; i++)
		if (m_LogRecord[i]) delete m_LogRecord[i];
	
	m_LogIndex = 0;
	memset(m_LogRecord, NULL, sizeof(LOG*)*MAX_LOG_ROW);
	
	ReleaseSRWLockExclusive(&m_LogLock);
}

/*public */void CDFLogMaster::SetFilterOn(UINT i)
{
	AcquireSRWLockExclusive(&m_LogLock);
	
	m_LogFilter |= i;
	
	ReleaseSRWLockExclusive(&m_LogLock);
}

/*public */void CDFLogMaster::SetFilterOff(UINT i)
{
	AcquireSRWLockExclusive(&m_LogLock);
	
	m_LogFilter &= ~i;
	
	ReleaseSRWLockExclusive(&m_LogLock);
}