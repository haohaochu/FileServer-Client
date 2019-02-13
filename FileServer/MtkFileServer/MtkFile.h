
// MtkFile.h : 檔案
//

#pragma once

#ifndef __AFXWIN_H__
	#error "對 PCH 包含此檔案前先包含 'stdafx.h'"
#endif

#include "resource.h"		// 主要符號
#include "mythread.h"
#include "DFLog.h"


// Sort unit
typedef struct CSortUnit
{
	int						m_index;
	char					m_data[256];
} SORTUNIT;


// MTK
// - server提供的檔案
typedef struct CMtkFileInfo
{
	HANDLE					m_handle;
	BOOL					m_attribute;
	UINT64					m_size;
	FILETIME				m_lastwrite;
} MTKINFO;


class CMtkFileMaster;
typedef struct CMtkFile
{
	MTKINFO					m_info;
	
	char					m_chFilepath[256];
	char					m_chFilename[256];
	char					m_chMtkname[256];
	UINT					m_uMtkver;
	
	HANDLE					m_hEvent;
	HANDLE					m_hUpdateEvent;
	UINT					m_uRefCount;
	UINT					m_uEventCount;
	SYSTEMTIME				m_tmEventTime;
//	SYSTEMTIME				m_getevent;
//	SYSTEMTIME				m_workevent;

//	SYSTEMTIME				m_getRevent;
	DWORD					m_dwLastWrite;

	BOOL					m_ref;
	UINT					m_age;
	CMtkFileMaster*			m_master;
	SRWLOCK					m_lock;
} MTK;


// class CDFFileRecord
// - File物件
class CDFFileRecord : public CXTPReportRecord
{
public:
	CDFFileRecord(CMtkFileMaster* master);
//	CDFFileRecord(CMtkFileMaster* master, MTK* mtk);
	virtual ~CDFFileRecord(void);

	virtual void	CreateItems();
	virtual void	GetItemMetrics(XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics);
	
	CString			GetItemValue(int index);

//	virtual void	DoPropExchange(CXTPPropExchange* pPX);

	CMtkFileMaster*	m_FileMaster;
//	MTK*			m_File;
};


// class CMtkFileMaster
// - server檔案管理員
class CMtkFileMaster : public CMyThread
{
public:
	CMtkFileMaster(AFX_THREADPROC pfnThreadProc, LPVOID param);
	~CMtkFileMaster(void);

protected:
	virtual void			Go(void);
	HANDLE					m_hEvent;
	HANDLE					m_hRefreshEvent;
	UINT					m_uRefreshAge;

public:
	static int				fnFileCompare(const void* a, const void* b);
	void					fnFileSort(int type, BOOL filter = FALSE);

	static void CALLBACK	fnDoRefresh(PVOID lpParameter, BOOLEAN bTimer);
	static void CALLBACK	fnDoUpdate(PVOID lpParameter, BOOLEAN bTimer);
	void					fnRefFile(MTK& file);
	void					fnUnRefFile(MTK& file);

	char					m_chFilePath[256];
	UINT					m_nSortIndex;
	UINT					m_nSortType;
	UINT					m_nSortCount;
	SORTUNIT*				m_ptSortList;
	CString					m_strFilter;

	CArray<MTK*,MTK*&>		m_arrMtkFile;
	CMapStringToPtr			m_mapMtkFile;
	SRWLOCK					m_lockFileList;
	SRWLOCK					m_lockRefreshFileList;
	UINT					m_uAge;
	UINT					m_uGrowAge;
	UINT					m_uEventCount;
	SYSTEMTIME				m_tmEventTime;
};

