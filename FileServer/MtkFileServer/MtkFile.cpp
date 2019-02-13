
// MtkFile.cpp : �w�q�ɮ����O�C
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
		case 0: // �s��
			{
				pItemMetrics->strText.Format("%03d", nIndexRow);
				break;
			}
		case 1: // �ɮצW��
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
		case 3: // �̫��s�ɶ�
			{
				FILETIME local;
				FileTimeToLocalFileTime(&mtk->m_info.m_lastwrite, &local);

				SYSTEMTIME time;
				FileTimeToSystemTime(&local, &time);

				pItemMetrics->strText.Format("%04d-%02d-%02d %02d:%02d:%02d", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);
				break;
			}
		case 4: // �ɮפj�p
			{
				//pItemMetrics->nColumnAlignment = 
				pItemMetrics->strText.Format("%I64d", mtk->m_info.m_size);
				break;
			}
		case 5: // ���m�ɶ�
			{
				DWORD dwDiff = GetTickCount()-mtk->m_dwLastWrite;
				pItemMetrics->strText.Format("%d", dwDiff>=1000 ? dwDiff/1000 : 0);
				//pItemMetrics->strText.Format("---");
				// DEBUG
				//pItemMetrics->strText.Format("%02d%02d%02d:%02d%02d%02d", mtk->m_getevent.wHour, mtk->m_getevent.wMinute, mtk->m_getevent.wSecond, mtk->m_workevent.wHour, mtk->m_workevent.wMinute, mtk->m_workevent.wSecond);
				break;
			}
		case 6: // �y�q
			{
				pItemMetrics->strText.Format("%d/%d", mtk->m_uEventCount, mtk->m_master->m_uEventCount);
				//pItemMetrics->strText.Format("---");
				// DEBUG 
				//pItemMetrics->strText.Format("%02d%02d%02d:%02d%02d%02d", mtk->m_getRevent.wHour, mtk->m_getRevent.wMinute, mtk->m_getRevent.wSecond, mtk->m_workRevent.wHour, mtk->m_workRevent.wMinute, mtk->m_workRevent.wSecond);
				break;
			}
		case 7: // �q�\
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

	// �Ĥ@����l��
	fnDoRefresh(this, TRUE);

	// �ĤG���H��aeventĲ�o
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
		if (m_strFilter != dlg->m_FileGrid.GetFilterText()) // Filter����(�����n,����,���T�w�S���D(?))
		{
			m_strFilter = dlg->m_FileGrid.GetFilterText();

			// ���s�ƦC
			fnFileSort(m_nSortIndex, TRUE);

			dlg->m_FileGrid.SetVirtualMode(new CDFFileRecord(this), m_nSortCount);
			dlg->m_FileGrid.Populate();
		}

		if (m_uRefreshAge < m_uGrowAge) // UI��s(�����n,����S���Y)
		{
			m_uRefreshAge = m_uGrowAge;

			// ���s�ƦC
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
			// �ƧǱ����ܧ�=�n���s�ƦC
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
	case 0: case 1: case 6: default: // �ɮצW��
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
	case 3: // �̫��s�ɶ�
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
	case 4: // �ɮפj�p
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
	case 5: // ���m�ɶ�
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
	case 7: // �q�\
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
	if (!RegisterWaitForSingleObject(&file.m_hUpdateEvent, file.m_hEvent, fnDoUpdate, &file, INFINITE, WT_EXECUTEDEFAULT/*WT_EXECUTEINWAITTHREAD*/)) // �T�w��,����S���Y
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
	if (!UnregisterWaitEx(file.m_hUpdateEvent, NULL)) // �T�w��,����S���Y
	//if (!UnregisterWaitEx(file.m_hUpdateEvent, INVALID_HANDLE_VALUE)) // �T�w��,����S���Y
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
			strLog.Format("UnregisterWaitEx����![%d][%s]", GetLastError(), strUpdateEvent);
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

	if (!TryAcquireSRWLockExclusive(&ptr->m_lockRefreshFileList))			// �W��(�ɮײM��)
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
		
		if (!bFindFile && bReadOnlyFile) // �ɮװ�Ū�B�S������=>�������L
			continue;

		// --- �H�U���O�ݭn�B�z���ɮ� ---

		if(!bFindFile) // �S���������ɮ�=>���ͬ���
		{
			// �s���ɮ�(��l��)
			mtk = new MTK();
			memset(mtk, NULL, sizeof(MTK));
		
			// TODO:�ɮ׸��|
			memcpy(mtk->m_chFilename, strFileName, strlen(strFileName));	// �ɮצW��
			memcpy(mtk->m_chMtkname, chMtkName, strlen(chMtkName));			// MTK�W��
			mtk->m_uMtkver = atoi(chMtkVer);								// MTK���
			mtk->m_uRefCount = 0;											// �q�\�H��
			mtk->m_uEventCount = 0;
			mtk->m_master = ptr;											// �޲z������
			mtk->m_dwLastWrite = dwTick;									
			InitializeSRWLock(&mtk->m_lock);								// Lock
		}

		// --- �H�U���O���������ɮ� ---

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
					// ��Ū�ɮ׸��L
					strLog.Format("[%s][%d]CreateFile Failure Readonly!!!", strFileName, GetLastError());
					log->DFLogSend(strLog, DF_SEVERITY_WARNING);

					delete mtk;
					mtk = NULL;
					continue;		
				}
				else
				{
					// �}�ɥ���
					strLog.Format("[%s][%d]CreateFile Failure!!!", strFileName, GetLastError());
					log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
					
					delete mtk;
					mtk = NULL;
					continue;
				}
			}
		}

		AcquireSRWLockExclusive(&mtk->m_lock);								// �W��(�ɮ�)
		if (mtk->m_info.m_handle)											// ��s�ɮ׸�T
		{
			// �ɮ׸�T���c
			BY_HANDLE_FILE_INFORMATION info;
			memset(&info, NULL, sizeof(BY_HANDLE_FILE_INFORMATION));
			if (GetFileInformationByHandle(mtk->m_info.m_handle, &info) == 0)
			{
				// ���o�ɮ׸�T����
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

			// �ɮ׳̫�g�J�ɶ�
			//FILETIME local;
			//FileTimeToLocalFileTime(&info.ftLastWriteTime, &local);
			//mtk->m_info.m_lastwrite = local;
			if (CompareFileTime(&(mtk->m_info.m_lastwrite), &(info.ftLastWriteTime))!= 0/*(UINT64)*(UINT64*)&(mtk->m_info.m_lastwrite) != (UINT64)*(UINT64*)&(info.ftLastWriteTime)*/) // �૬�A���FILETIME
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

			// �ɮפj�p
			LARGE_INTEGER size;
			size.QuadPart = 0;
			size.HighPart = info.nFileSizeHigh;
			size.LowPart = info.nFileSizeLow;
			mtk->m_info.m_size = size.QuadPart;

			// �ɮ��ݩ�(��Ū?)
			mtk->m_info.m_attribute = info.dwFileAttributes & FILE_ATTRIBUTE_READONLY ? TRUE : FALSE;
			
			if (!mtk->m_info.m_attribute) mtk->m_age = ptr->m_uAge;			// ��s�ͦs�ɶ�
		}
		ReleaseSRWLockExclusive(&mtk->m_lock);								// ����(�ɮ�)

		if (!bFindFile && !bReadOnlyFile)
		{
//			AcquireSRWLockExclusive(&ptr->m_lockFileList);					// �W��(�ɮײM��)
			ary->Add(mtk);													// �s�W�ɮ�(array)
			map->SetAt(strFileName, (void*&)mtk);							// �s�W�ɮ�(map)
//			ReleaseSRWLockExclusive(&ptr->m_lockFileList);					// ����(�ɮײM��)

			bGrowUp = TRUE;													// �����X��
			ptr->fnRefFile(*mtk);											// �q�\�ɮרƥ�
		}	
	}

	ReleaseSRWLockExclusive(&ptr->m_lockFileList);
	finder.Close();

	// �ɮװh������
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
				strLog.Format("�����ɮ�![%s]", mtk->m_chFilename);
				log->DFLogSend(strLog, DF_SEVERITY_INFORMATION);

				ary->RemoveAt(i);
				map->RemoveKey(mtk->m_chFilename);	
				//ptr->fnUnRefFile(*mtk);

				CloseHandle(mtk->m_info.m_handle);
				delete mtk;
				mtk = NULL;

				bGrowUp = TRUE;													// �ɮ׼W��ݭn
			}
		}
	}
	ReleaseSRWLockExclusive(&ptr->m_lockFileList);							// ����(�ɮײM��)

	if (bGrowUp)
		InterlockedIncrement(&ptr->m_uGrowAge);

	ReleaseSRWLockExclusive(&ptr->m_lockRefreshFileList);							// ����(�ɮײM��)
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
		// �ɮ׸�T���c
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

		// �ɮ׳̫�g�J�ɶ�
		//FILETIME local;
		//FileTimeToLocalFileTime(&info.ftLastWriteTime, &local);
		//mtk->m_info.m_lastwrite = local;
		//mtk->m_info.m_lastwrite = info.ftLastWriteTime;
		//mtk->m_dwLastWrite = GetTickCount();
		if (CompareFileTime(&(mtk->m_info.m_lastwrite), &(info.ftLastWriteTime))!= 0/*(UINT64)*(UINT64*)&(mtk->m_info.m_lastwrite) != (UINT64)*(UINT64*)&(info.ftLastWriteTime)*/) // �૬�A���FILETIME
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

		// �ɮפj�p
		LARGE_INTEGER size;
		size.QuadPart = 0;
		size.HighPart = info.nFileSizeHigh;
		size.LowPart = info.nFileSizeLow;
		mtk->m_info.m_size = size.QuadPart;

		// �ɮ��ݩ�(��Ū?)
		mtk->m_info.m_attribute = info.dwFileAttributes & FILE_ATTRIBUTE_READONLY ? TRUE : FALSE;
	}

	//if (strcmp(mtk->m_chFilename, "20171103.QHk.dbo.data.mtk")==0)
	//{
	//	strLog.Format("DoUpdate Event Done!!!");
	//	log->DFLogSend(strLog, DF_SEVERITY_HIGH);
	//}

	ReleaseSRWLockExclusive(&mtk->m_lock);
}


