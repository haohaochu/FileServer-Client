
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
// - client擁有的檔案
typedef struct CDFFileInfo
{
	HANDLE					m_handle;
	BOOL					m_attribute;
	UINT64					m_size;
	FILETIME				m_lastwrite;
} MTKINFO;


class CDFFileMaster;
class CDFNetMaster;
typedef struct CDFFile
{
	MTKINFO					m_info;
	
	char					m_chFilepath[256];
	char					m_chFilename[256];
	char					m_chMtkname[256];
	UINT					m_uMtkver;
	DWORD					m_dwLastwrite;

	OVERLAPPED				m_overlapped;

	UINT					m_age;
	CDFNetMaster*			m_master;
	BOOL					m_filter;
	BOOL					m_ended;
	SRWLOCK					m_lock;

	DWORD					m_lasttime;
	UINT					m_flow;
	UINT					m_maxflow;
} MTK;


// class CDFFileRecord
// - 檔案紀錄
class CDFFileRecord : public CXTPReportRecord
{
public:
	CDFFileRecord(CDFFileMaster* master);
	virtual ~CDFFileRecord(void);

	virtual void	CreateItems();
	virtual void	GetItemMetrics(XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics);
//	virtual void	DoPropExchange(CXTPPropExchange* pPX);

	CDFFileMaster*	m_FileMaster;
};


// class CDFFileMaster
// - 檔案管理員
class CDFFileMaster : public CMyThread
{
public:
	CDFFileMaster(AFX_THREADPROC pfnThreadProc, LPVOID param);
	~CDFFileMaster(void);

protected:
	virtual void			Go(void);
	
	// 自己用的,不用鎖
	UINT					m_uCount;
	UINT					m_uRefreshAge;		// UI顯示介面的更動檔案版本
	UINT					m_nSortIndex;
	UINT					m_nSortType;
	CString					m_strFilter;

public:
	static int				fnFileCompare(const void* a, const void* b);
	void					fnFileSort(int type, BOOL filter = FALSE);

	void					fnDoRefresh();
	void					fnDoUpdate();
	BOOL					fnCheckFileFilter(char* name);
	int						wildcmp(const char *wild, const char *string);

	// UI用的,要鎖
	char					m_chFilePath[256];	// 靜態資料,不鎖
	UINT					m_uAge;				// 手上的檔案版本,Interlock
	UINT					m_uGrowAge;			// 手上的更動檔案版本	,Interlock

	// 檔案清單,要鎖
	SRWLOCK					m_lockFileList;
	UINT					m_nSortCount;
	SORTUNIT*				m_ptSortList;	
	CMapStringToPtr			m_mapMtkFile;
	CArray<MTK*, MTK*&>		m_arrMtkFile;
	
};

