
// MtkFile.cpp : 定義檔案類別。
//

#include "stdafx.h"
#include "MtkFile.h"
#include "MtkFileServer.h"
#include "MtkFileServerDlg.h"

// Static member initialize
//

// CDFFileRecord class implementation
// 
CDFFileRecord::CDFFileRecord(CMtkFileMaster* master)
: m_FileMaster(master) 
//, m_File(NULL)
{
	CreateItems();
}

//CDFFileRecord::CDFFileRecord(CMtkFileMaster* master, MTK* mtk)
//: m_FileMaster(master) 
//, m_File(mtk)
//{
//	CreateItems();
//}


/*virtual */CDFFileRecord::~CDFFileRecord(void)
{

}

/*virtual */void CDFFileRecord::CreateItems()
{
	AddItem(new CXTPReportRecordItemText());
	AddItem(new CXTPReportRecordItemText());
	AddItem(new CXTPReportRecordItemText());
	AddItem(new CXTPReportRecordItemText());
	AddItem(new CXTPReportRecordItemText());
	AddItem(new CXTPReportRecordItemText());
	AddItem(new CXTPReportRecordItemText());
	AddItem(new CXTPReportRecordItemText());

	return;
}

/*virtual */void CDFFileRecord::GetItemMetrics(XTP_REPORTRECORDITEM_DRAWARGS* pDrawArgs, XTP_REPORTRECORDITEM_METRICS* pItemMetrics)
{
	CXTPReportColumnOrder *pSortOrder = pDrawArgs->pControl->GetColumns()->GetSortOrder();

	BOOL bDecreasing = pSortOrder->GetCount() > 0 && pSortOrder->GetAt(0)->IsSortedDecreasing();

	CString strColumn = pDrawArgs->pColumn->GetCaption();
	int nIndexCol = pDrawArgs->pColumn->GetItemIndex();
	int nIndexRow = pDrawArgs->pRow->GetIndex();
	int nCount = pDrawArgs->pControl->GetRows()->GetCount();

	if (m_FileMaster)
	{
		UINT idx = nIndexRow;

		AcquireSRWLockExclusive(&m_FileMaster->m_lockFileList);

		UINT count = m_FileMaster->m_arrMtkFile.GetCount();

		if ((m_FileMaster->m_ptSortList!=NULL) && (m_FileMaster->m_nSortCount<=count))
		{
			idx = bDecreasing ? m_FileMaster->m_ptSortList[m_FileMaster->m_nSortCount-nIndexRow-1].m_index : m_FileMaster->m_ptSortList[nIndexRow].m_index;
		}

		if (idx >= count)
		{
			pItemMetrics->strText.Format("%s", "UNKNOWN");
			ReleaseSRWLockExclusive(&m_FileMaster->m_lockFileList);
			return;
		}
		
		MTK* mtk = m_FileMaster->m_arrMtkFile[idx];

		if (mtk == NULL)
		{
			pItemMetrics->strText.Format("%s", "NULL");
			ReleaseSRWLockExclusive(&m_FileMaster->m_lockFileList);
			return;
		}
		
		switch (nIndexCol)
		{
		case 0: // 編號
			{
				pItemMetrics->strText.Format("%03d", nIndexRow);
				break;
			}
		case 1: // 檔案名稱
			{
				pItemMetrics->nItemIcon = mtk->m_info.m_attribute ? 1 : 0;
			
				DWORD dwDiff = GetTickCount()-mtk->m_dwLastWrite;
				if (dwDiff>=1000 && (dwDiff/1000)>60)
					pItemMetrics->nItemIcon = 3;
				
				pItemMetrics->strText.Format("%s", mtk->m_chFilename);
				break;
			}
		case 2: // MTK
			{
				pItemMetrics->strText.Format("%s", mtk->m_chMtkname);
				break;
			}
		case 3: // 最後更新時間
			{
				FILETIME local;
				FileTimeToLocalFileTime(&mtk->m_info.m_lastwrite, &local);

				SYSTEMTIME time;
				FileTimeToSystemTime(&local, &time);

				pItemMetrics->strText.Format("%04d-%02d-%02d %02d:%02d:%02d", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);
				break;
			}
		case 4: // 檔案大小
			{
				//pItemMetrics->nColumnAlignment = 
				pItemMetrics->strText.Format("%I64d", mtk->m_info.m_size);
				break;
			}
		case 5: // 閒置時間
			{
				DWORD dwDiff = GetTickCount()-mtk->m_dwLastWrite;
				pItemMetrics->strText.Format("%d", dwDiff>=1000 ? dwDiff/1000 : 0);
				//pItemMetrics->strText.Format("---");
				// DEBUG
				//pItemMetrics->strText.Format("%02d%02d%02d:%02d%02d%02d", mtk->m_getevent.wHour, mtk->m_getevent.wMinute, mtk->m_getevent.wSecond, mtk->m_workevent.wHour, mtk->m_workevent.wMinute, mtk->m_workevent.wSecond);
				break;
			}
		case 6: // 流量
			{
				pItemMetrics->strText.Format("%d/%d", mtk->m_uEventCount, mtk->m_master->m_uEventCount);
				//pItemMetrics->strText.Format("---");
				// DEBUG 
				//pItemMetrics->strText.Format("%02d%02d%02d:%02d%02d%02d", mtk->m_getRevent.wHour, mtk->m_getRevent.wMinute, mtk->m_getRevent.wSecond, mtk->m_workRevent.wHour, mtk->m_workRevent.wMinute, mtk->m_workRevent.wSecond);
				break;
			}
		case 7: // 訂閱
			{
				pItemMetrics->strText.Format("%d", mtk->m_uRefCount);
				break;
			}
		default:
			{
				pItemMetrics->strText.Format("%s", "unknown");
				break;
			}
		}

		ReleaseSRWLockExclusive(&m_FileMaster->m_lockFileList);
	}

	return;
}

CString	CDFFileRecord::GetItemValue(int index)
{
	return ((CXTPReportRecordItemText*)(this->GetItem(index)))->GetValue();
}



// class CMtkFileMaster implementation
// 
CMtkFileMaster::CMtkFileMaster(AFX_THREADPROC pfnThreadProc, LPVOID param)
: CMyThread(pfnThreadProc, param)
, m_nSortIndex(1)
, m_nSortType(0)
, m_nSortCount(0)
, m_ptSortList(NULL)
, m_strFilter("")
, m_uAge(0)
, m_uGrowAge(0)
, m_uEventCount(0)
, m_uRefreshAge(0)
, m_hEvent(NULL)
, m_hRefreshEvent(NULL)
{
	memset(m_chFilePath, NULL, 256);
	char* dir = theApp.m_chFileDir;
	memcpy(m_chFilePath, dir, strlen(dir));

	m_mapMtkFile.RemoveAll();
	m_arrMtkFile.RemoveAll();
	InitializeSRWLock(&m_lockFileList);
	InitializeSRWLock(&m_lockRefreshFileList);
}

CMtkFileMaster::~CMtkFileMaster(void)
{
	for (int i=0; i<m_arrMtkFile.GetCount(); i++)
	{		
		if (m_arrMtkFile[i])
		{
			fnUnRefFile(*m_arrMtkFile[i]);
			delete m_arrMtkFile[i];
			m_arrMtkFile[i] = NULL;
		}
	}

	m_mapMtkFile.RemoveAll();

	if (m_ptSortList)
		delete[] m_ptSortList;
}

/*virtual */void CMtkFileMaster::Go(void)
{
	CString strLog("");

	CMtkFileServerDlg* dlg = (CMtkFileServerDlg*)GetParentsHandle();
	CDFLogMaster* log = dlg->m_ptDFLog;

	CString	strRefreshEvent;
	//strRefreshEvent.Format("%srefresh", "d:\\quote\\mtk\\");
	strRefreshEvent.Format("%srefresh", m_chFilePath);
	strRefreshEvent.MakeLower();
	strRefreshEvent.Replace("\\", "_");
	strRefreshEvent.Replace(":", "_");

	// 第一次初始化
	fnDoRefresh(this, TRUE);

	// 第二次以後靠event觸發
	m_hEvent = CreateEvent(NULL, TRUE, FALSE, _T(strRefreshEvent));
	if (!RegisterWaitForSingleObject(&m_hRefreshEvent, m_hEvent, fnDoRefresh, this, 30000/*INFINITE*/, WT_EXECUTEDEFAULT/*WT_EXECUTEINWAITTHREAD*/))
	{
		strLog.Format("[%s]RegisterWaitForSingleObject Failure!!!", strRefreshEvent);
		log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
		return;
	}

	ReleaseSRWLockExclusive(&dlg->m_lockServer);

	while (!IsStop())
	{
		if (m_strFilter != dlg->m_FileGrid.GetFilterText()) // Filter改變(不重要,不鎖,不確定沒問題(?))
		{
			m_strFilter = dlg->m_FileGrid.GetFilterText();

			// 重新排列
			fnFileSort(m_nSortIndex, TRUE);

			dlg->m_FileGrid.SetVirtualMode(new CDFFileRecord(this), m_nSortCount);
			dlg->m_FileGrid.Populate();
		}

		if (m_uRefreshAge < m_uGrowAge) // UI更新(不重要,不鎖沒關係)
		{
			m_uRefreshAge = m_uGrowAge;

			// 重新排列
			fnFileSort(m_nSortIndex);
			
			dlg->m_FileGrid.SetVirtualMode(new CDFFileRecord(this), m_nSortCount);
			dlg->m_FileGrid.Populate();
		}
		
		CXTPReportColumnOrder *pSortOrder = dlg->m_FileGrid.GetColumns()->GetSortOrder();
		
		int sortindex = 1;
		int sorttype = 0;
		
		if (pSortOrder->GetCount() > 0)
		{
			sortindex = pSortOrder->GetAt(0)->GetIndex();
			sorttype = pSortOrder->GetAt(0)->IsSortedDecreasing() ? 1 : 0;
		}
		else
		{
			sortindex = 1;
			sorttype = 0;
		}

		m_nSortType = sorttype;
		if (m_nSortIndex != sortindex)
		{
			// 排序條件變更=要重新排列
			fnFileSort(sortindex);
			m_nSortIndex = sortindex;
		}
		dlg->m_FileGrid.RedrawControl();
		
		Sleep(1000);
	}

	if (!UnregisterWaitEx(m_hRefreshEvent, NULL))
	{
		strLog.Format("[%s]UnregisterWaitEx Failure!!!", strRefreshEvent);
		log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
	}
	
	CloseHandle(m_hEvent);
	EndThread();
}

/*protected */int CMtkFileMaster::fnFileCompare(const void* a, const void* b)
{
	char* data_a = ((SORTUNIT*)a)->m_data;
	char* data_b = ((SORTUNIT*)b)->m_data;

	for (int i=0; i<256; i++)
	{
		if ((data_a[i]==0) && (data_b[i]==0))
			break;

		if ((data_a[i]-data_b[i]) == 0)
			continue;

		return data_a[i]-data_b[i];
	}

	return 0;
}

/*protected */void CMtkFileMaster::fnFileSort(int type, BOOL filter)
{
	if (m_ptSortList)
		delete[] m_ptSortList;

	UINT tick = GetTickCount();

	AcquireSRWLockExclusive(&this->m_lockFileList);

	m_nSortCount = m_arrMtkFile.GetCount();
	m_ptSortList = new SORTUNIT[m_nSortCount];
	memset(m_ptSortList, NULL, sizeof(SORTUNIT)*m_nSortCount);

	CString strFilter("");
	if (filter)
		strFilter = m_strFilter;

	MTK* mtk = NULL; 

	switch (type)
	{
	case 0: case 1: case 6: default: // 檔案名稱
		{
			UINT idx = 0;

			for (UINT i=0; i<m_nSortCount; i++)
			{
				mtk = m_arrMtkFile.GetAt(i);

				if (filter && CString(mtk->m_chFilename).Find(strFilter) < 0)
					continue;

				m_ptSortList[idx].m_index = i;
				memcpy(m_ptSortList[idx].m_data, mtk->m_chFilename, strlen(mtk->m_chFilename));
				idx ++;
			}

			m_nSortCount = idx;

			break;
		}
	case 2: // MTK
		{
			UINT idx = 0;

			for (UINT i=0; i<m_nSortCount; i++)
			{
				mtk = m_arrMtkFile.GetAt(i);

				if (filter && CString(mtk->m_chFilename).Find(strFilter) < 0)
					continue;

				m_ptSortList[idx].m_index = i;
				memcpy(m_ptSortList[idx].m_data, mtk->m_chMtkname, strlen(mtk->m_chMtkname));
				idx ++;
			}

			m_nSortCount = idx;

			break;
		}
	case 3: // 最後更新時間
		{
			UINT idx = 0;

			for (UINT i=0; i<m_nSortCount; i++)
			{
				mtk = m_arrMtkFile.GetAt(i);

				if (filter && CString(mtk->m_chFilename).Find(strFilter) < 0)
					continue;

				m_ptSortList[idx].m_index = i;
				SYSTEMTIME time;
				FileTimeToSystemTime(&mtk->m_info.m_lastwrite, &time);
				sprintf_s(m_ptSortList[idx].m_data, "%04d0%02d%02d%02d%02d%02d", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);
				idx ++;
			}

			m_nSortCount = idx;

			break;
		}
	case 4: // 檔案大小
		{
			UINT idx = 0;

			for (UINT i=0; i<m_nSortCount; i++)
			{
				mtk = m_arrMtkFile.GetAt(i);

				if (filter && CString(mtk->m_chFilename).Find(strFilter) < 0)
					continue;

				m_ptSortList[idx].m_index = i;
				sprintf_s(m_ptSortList[idx].m_data, "%015d", mtk->m_info.m_size);
				idx ++;
			}
			
			m_nSortCount = idx;
			
			break;
		}
	case 5: // 閒置時間
		{
			UINT idx = 0;

			for (UINT i=0; i<m_nSortCount; i++)
			{
				mtk = m_arrMtkFile.GetAt(i);

				if (filter && CString(mtk->m_chFilename).Find(strFilter) < 0)
					continue;

				m_ptSortList[idx].m_index = i;
				memcpy(m_ptSortList[idx].m_data, &mtk->m_info.m_size, sizeof(UINT64));
				idx ++;
			}
			
			m_nSortCount = idx;
			
			break;
		}
	case 7: // 訂閱
		{
			UINT idx = 0;

			for (UINT i=0; i<m_nSortCount; i++)
			{
				mtk = m_arrMtkFile.GetAt(i);

				if (filter && CString(mtk->m_chFilename).Find(strFilter) < 0)
					continue;

				m_ptSortList[idx].m_index = i;
				sprintf_s(m_ptSortList[idx].m_data, "%03d", mtk->m_uRefCount);
				idx ++;
			}

			m_nSortCount = idx;

			break;
		}
	}

	ReleaseSRWLockExclusive(&this->m_lockFileList);

	qsort(m_ptSortList, m_nSortCount, sizeof(SORTUNIT), &this->fnFileCompare);
}

/*public */void CMtkFileMaster::fnRefFile(MTK& file)
{
	CString strLog("");

	CMtkFileServerDlg* dlg = (CMtkFileServerDlg*)GetParentsHandle();
	CDFLogMaster* log = dlg->m_ptDFLog;

	CString	strUpdateEvent;
	//strUpdateEvent.Format("d:\\quote\\mtk\\%s", file.m_chFilename);
	strUpdateEvent.Format("%s%s", m_chFilePath, file.m_chFilename);
	strUpdateEvent.MakeLower();
	strUpdateEvent.Replace("\\", "_");
	strUpdateEvent.Replace(":", "_");
	
	file.m_hEvent = CreateEvent(NULL, TRUE, FALSE, _T(strUpdateEvent)); 
	if (!RegisterWaitForSingleObject(&file.m_hUpdateEvent, file.m_hEvent, fnDoUpdate, &file, INFINITE, WT_EXECUTEDEFAULT/*WT_EXECUTEINWAITTHREAD*/)) // 固定值,不鎖沒關係
	{
		strLog.Format("[%s]RegisterWaitForSingleObject Failure!!!", strUpdateEvent);
		log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
		return;
	}

	file.m_ref = TRUE;
}

/*public */void CMtkFileMaster::fnUnRefFile(MTK& file)
{
	CString strLog("");

	CMtkFileServerDlg* dlg = (CMtkFileServerDlg*)GetParentsHandle();
	CDFLogMaster* log = dlg->m_ptDFLog;

	CString	strUpdateEvent;
	//strUpdateEvent.Format("d:\\quote\\mtk\\%s", file.m_chFilename);
	strUpdateEvent.Format("%s%s", m_chFilePath, file.m_chFilename);
	strUpdateEvent.MakeLower();
	strUpdateEvent.Replace("\\", "_");
	strUpdateEvent.Replace(":", "_");

	//HANDLE hCompleteEvent = CreateEvent(NULL, true, false, NULL); 
	//if (!UnregisterWaitEx(file.m_hUpdateEvent, hCompleteEvent))
	if (!UnregisterWaitEx(file.m_hUpdateEvent, NULL)) // 固定值,不鎖沒關係
	//if (!UnregisterWaitEx(file.m_hUpdateEvent, INVALID_HANDLE_VALUE)) // 固定值,不鎖沒關係
	{
		if (GetLastError() == ERROR_IO_PENDING)
		{
//			DWORD dw = WaitForSingleObject(hCompleteEvent, INFINITE);
//			AcquireSRWLockExclusive(&file.m_lock);
			strLog.Format("UnregisterWaitEx![ERROR_IO_PENDING][%s]", strUpdateEvent);
			log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
			file.m_ref = FALSE;
			return;
//			ReleaseSRWLockExclusive(&file.m_lock);
		}
		else
		{
			strLog.Format("UnregisterWaitEx失敗![%d][%s]", GetLastError(), strUpdateEvent);
			log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
		}	
	}

//	CloseHandle(hCompleteEvent);
	file.m_ref = FALSE;
	return;
}

/*CALLBACK */void CMtkFileMaster::fnDoRefresh(PVOID ldParameter, BOOLEAN bTimer)
{
	CFileFind finder;
	//CString strFind("d:\\quote\\mtk\\*.mtk");
	CString strFind("");
	CString strLog("");
	BOOL bGrowUp = FALSE;

	CMtkFileMaster* ptr = (CMtkFileMaster*)ldParameter;
	if (ptr->IsStop())
		return;

	if (!TryAcquireSRWLockExclusive(&ptr->m_lockRefreshFileList))			// 上鎖(檔案清單)
		return;					

	InterlockedIncrement(&ptr->m_uEventCount);
	InterlockedIncrement(&ptr->m_uAge);
	GetLocalTime(&ptr->m_tmEventTime);

	CMapStringToPtr* map = &ptr->m_mapMtkFile;
	CArray<MTK*, MTK*&>* ary = &ptr->m_arrMtkFile;

	CMtkFileServerDlg* dlg = (CMtkFileServerDlg*)ptr->GetParentsHandle();
	CDFLogMaster* log = dlg->m_ptDFLog;

	//if (!bTimer)
		//log->DFLogSend(CString("DoRefreshEvent!!!"), DF_SEVERITY_INFORMATION);

	strFind.Format("%s*.mtk", ptr->m_chFilePath);
	if (!finder.FindFile(strFind))
	{
		log->DFLogSend(CString("FindFile Failure!!!"), DF_SEVERITY_DISASTER);
		ReleaseSRWLockExclusive(&ptr->m_lockRefreshFileList);
		return;
	}
	
	DWORD dwTick = GetTickCount();

	AcquireSRWLockExclusive(&ptr->m_lockFileList);

	BOOL KeepWork = TRUE;
	while(KeepWork)
	{
		KeepWork = finder.FindNextFile();
		CString strFileName = finder.GetFileName();

		char chMtkVer[9] = {0};
		char* chMtkName = NULL;
		memcpy(chMtkVer, (char*)(LPCTSTR)strFileName, 8);
		chMtkName = ((char*)(LPCTSTR)strFileName)+9;

		CString strFullFileName("");
		//strFullFileName.Format("d:\\quote\\mtk\\%s", strFileName);
		strFullFileName.Format("%s%s", ptr->m_chFilePath, strFileName);

		MTK* mtk;
		BOOL bFindFile = FALSE;
		BOOL bReadOnlyFile = FALSE;

		bFindFile = map->Lookup(strFileName, (void*&)mtk);
		bReadOnlyFile = GetFileAttributes(strFullFileName) & FILE_ATTRIBUTE_READONLY;
		
		if (!bFindFile && bReadOnlyFile) // 檔案唯讀且沒有紀錄=>直接略過
			continue;

		// --- 以下都是需要處理的檔案 ---

		if(!bFindFile) // 沒有紀錄的檔案=>產生紀錄
		{
			// 新的檔案(初始化)
			mtk = new MTK();
			memset(mtk, NULL, sizeof(MTK));
		
			// TODO:檔案路徑
			memcpy(mtk->m_chFilename, strFileName, strlen(strFileName));	// 檔案名稱
			memcpy(mtk->m_chMtkname, chMtkName, strlen(chMtkName));			// MTK名稱
			mtk->m_uMtkver = atoi(chMtkVer);								// MTK日期
			mtk->m_uRefCount = 0;											// 訂閱人數
			mtk->m_uEventCount = 0;
			mtk->m_master = ptr;											// 管理員指標
			mtk->m_dwLastWrite = dwTick;									
			InitializeSRWLock(&mtk->m_lock);								// Lock
		}

		// --- 以下都是有紀錄的檔案 ---

		// DEBUG
		//GetLocalTime(&mtk->m_getRevent);

		if (!bReadOnlyFile && !mtk->m_info.m_handle)
		{
			//CString strFullFileName("");
			//strFullFileName.Format("d:\\quote\\mtk\\%s", strFileName);
			if ((mtk->m_info.m_handle = CreateFile(strFullFileName, GENERIC_READ /*| GENERIC_WRITE*/, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
			{
				if (GetLastError() == ERROR_ACCESS_DENIED)
				{
					// 唯讀檔案跳過
					strLog.Format("[%s][%d]CreateFile Failure Readonly!!!", strFileName, GetLastError());
					log->DFLogSend(strLog, DF_SEVERITY_WARNING);

					delete mtk;
					mtk = NULL;
					continue;		
				}
				else
				{
					// 開檔失敗
					strLog.Format("[%s][%d]CreateFile Failure!!!", strFileName, GetLastError());
					log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
					
					delete mtk;
					mtk = NULL;
					continue;
				}
			}
		}

		AcquireSRWLockExclusive(&mtk->m_lock);								// 上鎖(檔案)
		if (mtk->m_info.m_handle)											// 更新檔案資訊
		{
			// 檔案資訊結構
			BY_HANDLE_FILE_INFORMATION info;
			memset(&info, NULL, sizeof(BY_HANDLE_FILE_INFORMATION));
			if (GetFileInformationByHandle(mtk->m_info.m_handle, &info) == 0)
			{
				// 取得檔案資訊失敗
				strLog.Format("[%s]GetFileInformationByHandle Failure!!!", mtk->m_chFilename);
				log->DFLogSend(strLog, DF_SEVERITY_HIGH);
				ReleaseSRWLockExclusive(&mtk->m_lock);
				continue;
			}
			
			// DEBUG
			//GetLocalTime(&mtk->m_workRevent);

//			if (strcmp(mtk->m_chMtkname, "QAbula.dbo.asx.mtk") == 0)
//			{
//				FILETIME memT, realT;
//				FileTimeToLocalFileTime(&mtk->m_info.m_lastwrite, &memT);
//				FileTimeToLocalFileTime(&info.ftLastWriteTime, &realT);
//
//				SYSTEMTIME memS, realS;
//				FileTimeToSystemTime(&memT, &memS);
//				FileTimeToSystemTime(&realT, &realS);
//			
//				strLog.Format("[%s][MEMTIME:%d-%d-%d %d:%d:%d][FILETIME:%d-%d-%d %d:%d:%d]", mtk->m_chFilename, memS.wYear, memS.wMonth, memS.wDay, memS.wHour, memS.wMinute, memS.wSecond, realS.wYear, realS.wMonth, realS.wDay, realS.wHour, realS.wMinute, realS.wSecond);
//				log->DFLogSend(strLog, DF_SEVERITY_HIGH);
//			}

			// 檔案最後寫入時間
			//FILETIME local;
			//FileTimeToLocalFileTime(&info.ftLastWriteTime, &local);
			//mtk->m_info.m_lastwrite = local;
			if (CompareFileTime(&(mtk->m_info.m_lastwrite), &(info.ftLastWriteTime))!= 0/*(UINT64)*(UINT64*)&(mtk->m_info.m_lastwrite) != (UINT64)*(UINT64*)&(info.ftLastWriteTime)*/) // 轉型態比較FILETIME
			{
				mtk->m_dwLastWrite = dwTick;
				mtk->m_info.m_lastwrite = info.ftLastWriteTime;
			
//				if (strcmp(mtk->m_chMtkname, "QAbula.dbo.asx.mtk") == 0)
//				{
//					FILETIME memT, realT;
//					FileTimeToLocalFileTime(&mtk->m_info.m_lastwrite, &memT);
//					FileTimeToLocalFileTime(&info.ftLastWriteTime, &realT);
//
//					SYSTEMTIME memS, realS;
//					FileTimeToSystemTime(&memT, &memS);
//					FileTimeToSystemTime(&realT, &realS);
//			
//					strLog.Format("[UPDATE!!!][%s][MEMTIME:%d-%d-%d %d:%d:%d][FILETIME:%d-%d-%d %d:%d:%d]", mtk->m_chFilename, memS.wYear, memS.wMonth, memS.wDay, memS.wHour, memS.wMinute, memS.wSecond, realS.wYear, realS.wMonth, realS.wDay, realS.wHour, realS.wMinute, realS.wSecond);
//					log->DFLogSend(strLog, DF_SEVERITY_HIGH);
//				}
			}

			// 檔案大小
			LARGE_INTEGER size;
			size.QuadPart = 0;
			size.HighPart = info.nFileSizeHigh;
			size.LowPart = info.nFileSizeLow;
			mtk->m_info.m_size = size.QuadPart;

			// 檔案屬性(唯讀?)
			mtk->m_info.m_attribute = info.dwFileAttributes & FILE_ATTRIBUTE_READONLY ? TRUE : FALSE;
			
			if (!mtk->m_info.m_attribute) mtk->m_age = ptr->m_uAge;			// 更新生存時間
		}
		ReleaseSRWLockExclusive(&mtk->m_lock);								// 解鎖(檔案)

		if (!bFindFile && !bReadOnlyFile)
		{
//			AcquireSRWLockExclusive(&ptr->m_lockFileList);					// 上鎖(檔案清單)
			ary->Add(mtk);													// 新增檔案(array)
			map->SetAt(strFileName, (void*&)mtk);							// 新增檔案(map)
//			ReleaseSRWLockExclusive(&ptr->m_lockFileList);					// 解鎖(檔案清單)

			bGrowUp = TRUE;													// 成長旗標
			ptr->fnRefFile(*mtk);											// 訂閱檔案事件
		}	
	}

	ReleaseSRWLockExclusive(&ptr->m_lockFileList);
	finder.Close();

	// 檔案退場機制
	AcquireSRWLockExclusive(&ptr->m_lockFileList);
	for (int i=0; i<ary->GetCount(); i++)
	{
		MTK* mtk = ary->GetAt(i);

		if (mtk->m_info.m_attribute && mtk->m_age < ptr->m_uAge && mtk->m_uRefCount == 0)
		{
			if (mtk->m_ref)
			{
				ptr->fnUnRefFile(*mtk);
			}
			else
			{
				strLog.Format("釋放檔案![%s]", mtk->m_chFilename);
				log->DFLogSend(strLog, DF_SEVERITY_INFORMATION);

				ary->RemoveAt(i);
				map->RemoveKey(mtk->m_chFilename);	
				//ptr->fnUnRefFile(*mtk);

				CloseHandle(mtk->m_info.m_handle);
				delete mtk;
				mtk = NULL;

				bGrowUp = TRUE;													// 檔案增減都需要
			}
		}
	}
	ReleaseSRWLockExclusive(&ptr->m_lockFileList);							// 解鎖(檔案清單)

	if (bGrowUp)
		InterlockedIncrement(&ptr->m_uGrowAge);

	ReleaseSRWLockExclusive(&ptr->m_lockRefreshFileList);							// 解鎖(檔案清單)
	//log->DFLogSend(CString("DoRefreshEvent Done!!!"), DF_SEVERITY_HIGH);
}

/*CALLBACK */void CMtkFileMaster::fnDoUpdate(PVOID ldParameter, BOOLEAN bTimer)
{	
	CString strLog("");

	MTK* mtk = (MTK*)ldParameter;
	if (!mtk)
		return;

	InterlockedIncrement(&mtk->m_uEventCount);
	GetLocalTime(&mtk->m_tmEventTime);

	CMtkFileMaster* master = mtk->m_master;
	CDFLogMaster* log = (CDFLogMaster*)((CMtkFileServerDlg*)(master->GetParentsHandle()))->m_ptDFLog;
	if (master->IsStop())
		return;

	//if (strcmp(mtk->m_chFilename, "20171103.QHk.dbo.data.mtk")==0)
	//{
	//	strLog.Format("DoUpdate Event!!!");
	//	log->DFLogSend(strLog, DF_SEVERITY_HIGH);
	//}

	// DEBUG 
	//GetLocalTime(&mtk->m_getevent);

	if (!TryAcquireSRWLockExclusive(&mtk->m_lock))  
		return;

	//if (strcmp(mtk->m_chFilename, "20171103.QHk.dbo.data.mtk")==0)
	//{
	//	strLog.Format("DoUpdate Event!!!");
	//	log->DFLogSend(strLog, DF_SEVERITY_INFORMATION);
	//}

	if (mtk->m_info.m_handle)
	{
		// 檔案資訊結構
		BY_HANDLE_FILE_INFORMATION info;
		memset(&info, NULL, sizeof(BY_HANDLE_FILE_INFORMATION));
		if (GetFileInformationByHandle(mtk->m_info.m_handle, &info) == 0)
		{
			// ERROR
			strLog.Format("[%s][CALLBACK]GetFileInformationByHandle Failure!!!", mtk->m_chFilename);
			log->DFLogSend(strLog, DF_SEVERITY_HIGH);
			return;
		}
			
		// DEBUG
		//GetLocalTime(&mtk->m_workevent);
			
//		if (strcmp(mtk->m_chMtkname, "QAbula.dbo.asx.mtk") == 0)
//		{
//			FILETIME memT, realT;
//			FileTimeToLocalFileTime(&mtk->m_info.m_lastwrite, &memT);
//			FileTimeToLocalFileTime(&info.ftLastWriteTime, &realT);
//
//			SYSTEMTIME memS, realS;
//			FileTimeToSystemTime(&memT, &memS);
//			FileTimeToSystemTime(&realT, &realS);
//			
///			strLog.Format("[%s][MEMTIME:%d-%d-%d %d:%d:%d][FILETIME:%d-%d-%d %d:%d:%d]", mtk->m_chFilename, memS.wYear, memS.wMonth, memS.wDay, memS.wHour, memS.wMinute, memS.wSecond, realS.wYear, realS.wMonth, realS.wDay, realS.wHour, realS.wMinute, realS.wSecond);
//			log->DFLogSend(strLog, DF_SEVERITY_HIGH);
//		}

		// 檔案最後寫入時間
		//FILETIME local;
		//FileTimeToLocalFileTime(&info.ftLastWriteTime, &local);
		//mtk->m_info.m_lastwrite = local;
		//mtk->m_info.m_lastwrite = info.ftLastWriteTime;
		//mtk->m_dwLastWrite = GetTickCount();
		if (CompareFileTime(&(mtk->m_info.m_lastwrite), &(info.ftLastWriteTime))!= 0/*(UINT64)*(UINT64*)&(mtk->m_info.m_lastwrite) != (UINT64)*(UINT64*)&(info.ftLastWriteTime)*/) // 轉型態比較FILETIME
		{
			mtk->m_dwLastWrite = GetTickCount();
			mtk->m_info.m_lastwrite = info.ftLastWriteTime;

//			if (strcmp(mtk->m_chMtkname, "QAbula.dbo.asx.mtk") == 0)
//			{
//				FILETIME memT, realT;
//				FileTimeToLocalFileTime(&mtk->m_info.m_lastwrite, &memT);
//				FileTimeToLocalFileTime(&info.ftLastWriteTime, &realT);
//
//				SYSTEMTIME memS, realS;
//				FileTimeToSystemTime(&memT, &memS);
//				FileTimeToSystemTime(&realT, &realS);
//			
//				strLog.Format("[UPDATE!!!][%s][MEMTIME:%d-%d-%d %d:%d:%d][FILETIME:%d-%d-%d %d:%d:%d]", mtk->m_chFilename, memS.wYear, memS.wMonth, memS.wDay, memS.wHour, memS.wMinute, memS.wSecond, realS.wYear, realS.wMonth, realS.wDay, realS.wHour, realS.wMinute, realS.wSecond);
//				log->DFLogSend(strLog, DF_SEVERITY_HIGH);
//			}
		}

		// 檔案大小
		LARGE_INTEGER size;
		size.QuadPart = 0;
		size.HighPart = info.nFileSizeHigh;
		size.LowPart = info.nFileSizeLow;
		mtk->m_info.m_size = size.QuadPart;

		// 檔案屬性(唯讀?)
		mtk->m_info.m_attribute = info.dwFileAttributes & FILE_ATTRIBUTE_READONLY ? TRUE : FALSE;
	}

	//if (strcmp(mtk->m_chFilename, "20171103.QHk.dbo.data.mtk")==0)
	//{
	//	strLog.Format("DoUpdate Event Done!!!");
	//	log->DFLogSend(strLog, DF_SEVERITY_HIGH);
	//}

	ReleaseSRWLockExclusive(&mtk->m_lock);
}


