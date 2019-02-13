
// MtkFile.cpp : �w�q�ɮ����O�C
//

#include "stdafx.h"
#include "DFFile.h"
#include "DDFileClient.h"
#include "DDFileClientDlg.h"

// Static member initialize
//


// CDFFileRecord class implementation
// 
CDFFileRecord::CDFFileRecord(CDFFileMaster* master)
: m_FileMaster(master) 
{
	CreateItems();
}

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
	AddItem(new CXTPReportRecordItemText());
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
		case 1: // ���A��
			{
				AcquireSRWLockExclusive(&mtk->m_lock);
				if (mtk->m_master != NULL)
					pItemMetrics->strText.Format("%s:%d", mtk->m_master->m_NetServerIP, mtk->m_master->m_NetServerPort);
				else
					pItemMetrics->strText.Format("---");
				ReleaseSRWLockExclusive(&mtk->m_lock);
				break;
			}
		case 2: // �ɮצW��
			{
				AcquireSRWLockExclusive(&mtk->m_lock);
				// 0:���v 1:����x 2:�¦�! 3:�ť�
				if (mtk->m_master != NULL)
				{
					if (mtk->m_info.m_attribute == 0)
						pItemMetrics->nItemIcon = 0;
					else
						pItemMetrics->nItemIcon = 1;

					//DWORD dwDiff = GetTickCount()-mtk->m_dwLastwrite;
					//if (dwDiff>=1000 && (dwDiff/1000)>60)
					//	pItemMetrics->nItemIcon = 2;
				}
				else
					pItemMetrics->nItemIcon = 3;

				DWORD dwDiff = GetTickCount()-mtk->m_dwLastwrite;
				if (dwDiff>=1000 && (dwDiff/1000)>60)
					pItemMetrics->nItemIcon = 2;

				pItemMetrics->strText.Format("%s", mtk->m_chFilename);
				ReleaseSRWLockExclusive(&mtk->m_lock);
				break;
			}
		case 3: // MTK
			{
				// �R�A���->����
				pItemMetrics->strText.Format("%s", mtk->m_chMtkname);
				break;
			}
		case 4: // �̫��s�ɶ�
			{
				AcquireSRWLockExclusive(&mtk->m_lock);
				//if (mtk->m_master != NULL) 
				//{
					FILETIME local;
					SYSTEMTIME time;

					FileTimeToLocalFileTime(&mtk->m_info.m_lastwrite, &local);
					FileTimeToSystemTime(&local, &time);
					pItemMetrics->strText.Format("%04d-%02d-%02d %02d:%02d:%02d", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);	
				//}
				//else
				//{
				//	pItemMetrics->strText.Format("---");
				//}
				ReleaseSRWLockExclusive(&mtk->m_lock);
				break;
			}
		case 5: // �ɮפj�p
			{
				AcquireSRWLockExclusive(&mtk->m_lock);
				//if (mtk->m_master != NULL)
					pItemMetrics->strText.Format("%I64d", mtk->m_info.m_size);
				//else
				//	pItemMetrics->strText.Format("---");
				ReleaseSRWLockExclusive(&mtk->m_lock);
				break;
			}
		case 6: // ���m�ɶ�
			{
				AcquireSRWLockExclusive(&mtk->m_lock);
				//if (mtk->m_master != NULL)
				//{
					DWORD dwDiff = GetTickCount()-mtk->m_dwLastwrite;
					pItemMetrics->strText.Format("%d", dwDiff>=1000 ? dwDiff/1000 : 0);
				//}
				//else
				//{
				//	pItemMetrics->strText.Format("---");
				//}
				ReleaseSRWLockExclusive(&mtk->m_lock);
				break;
			}
		case 7: // ���y�q
			{
				AcquireSRWLockExclusive(&mtk->m_lock);
				pItemMetrics->strText.Format("%d(�̤j:%d)", mtk->m_flow, mtk->m_maxflow);
				ReleaseSRWLockExclusive(&mtk->m_lock);
				break;
			}
		case 8: // �y�q
			{
				pItemMetrics->strText.Format("---");
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


// class CMtkFileMaster implementation
// 
CDFFileMaster::CDFFileMaster(AFX_THREADPROC pfnThreadProc, LPVOID param)
: CMyThread(pfnThreadProc, param)
, m_nSortIndex(1)
, m_nSortType(0)
, m_nSortCount(0)
, m_ptSortList(NULL)
, m_strFilter("")
, m_uCount(0)
, m_uRefreshAge(0)
, m_uGrowAge(0)
, m_uAge(0)
{
	memset(m_chFilePath, NULL, 256);
	char* dir = theApp.m_chFileDir;
	memcpy(m_chFilePath, dir, strlen(dir));
	
	m_mapMtkFile.RemoveAll();
	m_arrMtkFile.RemoveAll();

	InitializeSRWLock(&m_lockFileList);
}

CDFFileMaster::~CDFFileMaster(void)
{
	for (int i=0; i<m_arrMtkFile.GetCount(); i++)
	{		
		if (m_arrMtkFile[i])
		{
			delete m_arrMtkFile[i];
			m_arrMtkFile[i] = NULL;
		}
	}
}

/*virtual */void CDFFileMaster::Go(void)
{
	CString strLog("");

	CDDFileClientDlg* dlg = (CDDFileClientDlg*)GetParentsHandle();
	CDFLogMaster* log = dlg->m_ptDFLog;

	CString	strRefreshEvent;
	//strRefreshEvent.Format("%srefresh", "d:\\quote\\mtk2\\");
	strRefreshEvent.Format("%srefresh", theApp.m_chFileDir);
	strRefreshEvent.MakeLower();
	strRefreshEvent.Replace("\\", "_");
	strRefreshEvent.Replace(":", "_");

	// �Ĥ@����l��
	fnDoRefresh();

	ReleaseSRWLockExclusive(&dlg->m_lockServer);

	DWORD tLastRefreshTime = GetTickCount();

	while (!IsStop())
	{	
		BOOL bSetFilter = (m_strFilter != dlg->m_FileGrid.GetFilterText());

		if (m_uRefreshAge < m_uGrowAge) // UI��s(�����n,����S���Y)
		{
			m_uRefreshAge = m_uGrowAge;

			// ���s�ƦC
			if (bSetFilter)
			{
				m_strFilter = dlg->m_FileGrid.GetFilterText();
				fnFileSort(m_nSortIndex, TRUE);
			}
			else
				fnFileSort(m_nSortIndex);
			
			dlg->m_FileGrid.SetVirtualMode(new CDFFileRecord(this), m_nSortCount);
			dlg->m_FileGrid.Populate();
		}
		else if (bSetFilter == TRUE) // Filter����(�����n,����,���T�w�S���D(?))
		{
			m_strFilter = dlg->m_FileGrid.GetFilterText();
			fnFileSort(m_nSortIndex, TRUE);

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
		
		DWORD tCurrentTime = GetTickCount();
		if ((tCurrentTime-tLastRefreshTime) >= 30000)
		{
			fnDoRefresh();
			tLastRefreshTime = GetTickCount();
		}

		Sleep(1000);
	}

	if (m_ptSortList)
		delete[] m_ptSortList;

	EndThread();
}

/*protected */int CDFFileMaster::fnFileCompare(const void* a, const void* b)
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

/*protected */void CDFFileMaster::fnFileSort(int type, BOOL filter)
{
	AcquireSRWLockExclusive(&this->m_lockFileList);
	
	if (m_ptSortList)
		delete[] m_ptSortList;

	UINT tick = GetTickCount();

	m_nSortCount = m_arrMtkFile.GetCount();
	m_ptSortList = new SORTUNIT[m_nSortCount];
	memset(m_ptSortList, NULL, sizeof(SORTUNIT)*m_nSortCount);

	CString strFilter("");
	if (filter)
		strFilter = m_strFilter;

	MTK* mtk = NULL; 

	switch (type)
	{
	case 0: case 2: case 7: case 8: default: // �ɮצW��
		{
			UINT idx = 0;

			for (UINT i=0; i<m_nSortCount; i++)
			{
				mtk = m_arrMtkFile.GetAt(i);

				if (mtk == NULL)
					continue;

				AcquireSRWLockExclusive(&mtk->m_lock);

				if (filter && CString(mtk->m_chFilename).Find(strFilter) < 0)
				{
					ReleaseSRWLockExclusive(&mtk->m_lock);
					continue;
				}
				
				m_ptSortList[idx].m_index = i;
				memcpy(m_ptSortList[idx].m_data, mtk->m_chFilename, strlen(mtk->m_chFilename));
				
				ReleaseSRWLockExclusive(&mtk->m_lock);
				idx ++;
			}

			m_nSortCount = idx;

			break;
		}
	case 1: // ���A��
		{
			UINT idx = 0;

			for (UINT i=0; i<m_nSortCount; i++)
			{
				mtk = m_arrMtkFile.GetAt(i);

				if (mtk == NULL)
					continue;

				AcquireSRWLockExclusive(&mtk->m_lock);

				if (filter && CString(mtk->m_chFilename).Find(strFilter) < 0)
				{
					ReleaseSRWLockExclusive(&mtk->m_lock);
					continue;
				}

				m_ptSortList[idx].m_index = i;
				if (mtk->m_master)
					memcpy(m_ptSortList[idx].m_data, mtk->m_master->m_NetServerIP, strlen(mtk->m_master->m_NetServerIP));
				
				ReleaseSRWLockExclusive(&mtk->m_lock);
				idx ++;
			}

			m_nSortCount = idx;

			break;
		}
	case 3: // MTK
		{
			UINT idx = 0;

			for (UINT i=0; i<m_nSortCount; i++)
			{
				mtk = m_arrMtkFile.GetAt(i);

				if (mtk == NULL)
					continue;

				AcquireSRWLockExclusive(&mtk->m_lock);

				if (filter && CString(mtk->m_chFilename).Find(strFilter) < 0)
				{
					ReleaseSRWLockExclusive(&mtk->m_lock);
					continue;
				}

				m_ptSortList[idx].m_index = i;
				memcpy(m_ptSortList[idx].m_data, mtk->m_chMtkname, strlen(mtk->m_chMtkname));
				
				ReleaseSRWLockExclusive(&mtk->m_lock);
				idx ++;
			}

			m_nSortCount = idx;

			break;
		}
	case 4: // �̫��s�ɶ�
		{
			UINT idx = 0;

			for (UINT i=0; i<m_nSortCount; i++)
			{
				mtk = m_arrMtkFile.GetAt(i);

				if (mtk == NULL)
					continue;

				AcquireSRWLockExclusive(&mtk->m_lock);

				if (filter && CString(mtk->m_chFilename).Find(strFilter) < 0)
				{
					ReleaseSRWLockExclusive(&mtk->m_lock);
					continue;
				}

				m_ptSortList[idx].m_index = i;
				SYSTEMTIME time;
				FileTimeToSystemTime(&mtk->m_info.m_lastwrite, &time);
				sprintf_s(m_ptSortList[idx].m_data, "%04d0%02d%02d%02d%02d%02d", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);
				
				ReleaseSRWLockExclusive(&mtk->m_lock);
				idx ++;
			}

			m_nSortCount = idx;

			break;
		}
	case 5: // �ɮפj�p
		{
			UINT idx = 0;

			for (UINT i=0; i<m_nSortCount; i++)
			{
				mtk = m_arrMtkFile.GetAt(i);

				if (mtk == NULL)
					continue;

				AcquireSRWLockExclusive(&mtk->m_lock);

				if (filter && CString(mtk->m_chFilename).Find(strFilter) < 0)
				{
					ReleaseSRWLockExclusive(&mtk->m_lock);
					continue;
				}

				m_ptSortList[idx].m_index = i;
				sprintf_s(m_ptSortList[idx].m_data, "%015d", mtk->m_info.m_size);
				
				ReleaseSRWLockExclusive(&mtk->m_lock);
				idx ++;
			}
			
			m_nSortCount = idx;
			
			break;
		}
	case 6: // ���m�ɶ�
		{
			UINT idx = 0;

			for (UINT i=0; i<m_nSortCount; i++)
			{
				mtk = m_arrMtkFile.GetAt(i);

				if (mtk == NULL)
					continue;

				AcquireSRWLockExclusive(&mtk->m_lock);

				if (filter && CString(mtk->m_chFilename).Find(strFilter) < 0)
				{
					ReleaseSRWLockExclusive(&mtk->m_lock);
					continue;
				}

				m_ptSortList[idx].m_index = i;
				memcpy(m_ptSortList[idx].m_data, &mtk->m_info.m_size, sizeof(UINT64));
				
				ReleaseSRWLockExclusive(&mtk->m_lock);
				idx ++;
			}
			
			m_nSortCount = idx;
			
			break;
		}
	}

	ReleaseSRWLockExclusive(&this->m_lockFileList);

	qsort(m_ptSortList, m_nSortCount, sizeof(SORTUNIT), &this->fnFileCompare);
}

void CDFFileMaster::fnDoRefresh()
{
	CFileFind finder;
	CString strFind("");
	CString strLog("");
	BOOL bGrowUp = FALSE;

	CDDFileClientDlg* dlg = (CDDFileClientDlg*)GetParentsHandle();
	CDFLogMaster* log = dlg->m_ptDFLog;

	strFind.Format("%s*.mtk", theApp.m_chFileDir);
	if (!finder.FindFile(strFind))
	{
		log->DFLogSend(CString("FindFile Failure!!!"), DF_SEVERITY_DISASTER);
		return;
	}
	
	InterlockedIncrement(&m_uAge);

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
		strFullFileName.Format("%s%s", theApp.m_chFileDir, strFileName);

		MTK* mtk = NULL;
		BOOL bFindFile = FALSE;
		BOOL bReadOnlyFile = FALSE;

		HANDLE handle = NULL;
		AcquireSRWLockExclusive(&m_lockFileList);
		if (!m_mapMtkFile.Lookup(strFileName, (void*&)mtk))
		{
			mtk = new MTK();
			memset(mtk, NULL, sizeof(MTK));
			
			// �����ɮצW��
			memcpy(mtk->m_chFilename, strFileName, strlen(strFileName));
			memcpy(mtk->m_chMtkname, chMtkName, strlen(chMtkName));
			mtk->m_uMtkver = atoi(chMtkVer);
			mtk->m_master = NULL;
			mtk->m_filter = fnCheckFileFilter(mtk->m_chFilename);
			mtk->m_ended = FALSE;
			mtk->m_age = m_uAge;
			InitializeSRWLock(&mtk->m_lock);
		
			if ((handle = CreateFile(strFullFileName, GENERIC_READ /*| GENERIC_WRITE*/, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING/*OPEN_ALWAYS*/, FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
			{
				if (GetLastError() == ERROR_ACCESS_DENIED)
				{
					// ��Ū�ɮ׸��L,�z�����|�Ө�o��
					strLog.Format("�}�ɿ��~(Readonly)![%d][%s]", GetLastError(), strFileName);
					log->DFLogSend(strLog, DF_SEVERITY_DISASTER);

					delete mtk;
					mtk = NULL;

					ReleaseSRWLockExclusive(&m_lockFileList);
					continue;
				}
				else
				{
					// �}�ɥ���
					strLog.Format("�}�ɿ��~![%d][%s]", GetLastError(), strFileName);
					log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
					
					delete mtk;
					mtk = NULL;

					ReleaseSRWLockExclusive(&m_lockFileList);
					continue;
				}
			}

			if (handle)
			{
				// �ɮ׸�T���c
				BY_HANDLE_FILE_INFORMATION info;
				memset(&info, NULL, sizeof(BY_HANDLE_FILE_INFORMATION));
				if (GetFileInformationByHandle(handle, &info) == 0)
				{
					// ���o�ɮ׸�T����
					strLog.Format("GetFileInformationByHandle����![%s]", mtk->m_chFilename);
					log->DFLogSend(strLog, DF_SEVERITY_HIGH);
					
					ReleaseSRWLockExclusive(&m_lockFileList);
					continue;
				}
			
				// �ɮ��ݩ�(��Ū?)
				mtk->m_info.m_attribute = info.dwFileAttributes & FILE_ATTRIBUTE_READONLY ? TRUE : FALSE;

				// �ɮ׳̫�g�J�ɶ�
				if (CompareFileTime(&(mtk->m_info.m_lastwrite), &(info.ftLastWriteTime))!= 0/*(UINT64)*(UINT64*)&(mtk->m_info.m_lastwrite) != (UINT64)*(UINT64*)&(info.ftLastWriteTime)*/) // �૬�A���FILETIME
				{
					mtk->m_info.m_lastwrite = info.ftLastWriteTime;
					mtk->m_dwLastwrite = GetTickCount();
				}

				// �ɮפj�p
				LARGE_INTEGER size;
				size.QuadPart = 0;
				size.HighPart = info.nFileSizeHigh;
				size.LowPart = info.nFileSizeLow;
				mtk->m_info.m_size = size.QuadPart;
			}
			
			// ���O��Ū�ɮ�=>�[�J�M��=>����ʱ�
			if (!mtk->m_info.m_attribute)
			{
				m_arrMtkFile.Add(mtk);
				m_mapMtkFile.SetAt(strFileName, (void*&)mtk);
				bGrowUp = TRUE;	
			}
			// ��Ū�ɮ�=>�R��=>���z�L
			else
			{
				delete mtk;
				mtk = NULL;
			}

			CloseHandle(handle);
			handle = NULL;
		}
		else
		{
			// 
			if ((handle = CreateFile(strFullFileName, GENERIC_READ /*| GENERIC_WRITE*/, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING/*OPEN_ALWAYS*/, FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
			{
				if (GetLastError() == ERROR_ACCESS_DENIED)
				{
					// ��Ū�ɮ׸��L,�z�����|�Ө�o��
					strLog.Format("�}�ɿ��~(Readonly)![%d][%s]", GetLastError(), strFileName);
					log->DFLogSend(strLog, DF_SEVERITY_DISASTER);

					delete mtk;
					mtk = NULL;

					ReleaseSRWLockExclusive(&m_lockFileList);
					continue;
				}
				else
				{
					// �}�ɥ���
					strLog.Format("�}�ɿ��~![%d][%s]", GetLastError(), strFileName);
					log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
					
					delete mtk;
					mtk = NULL;

					ReleaseSRWLockExclusive(&m_lockFileList);
					continue;
				}
			}

			if (handle)
			{
				// �ɮ׸�T���c
				BY_HANDLE_FILE_INFORMATION info;
				memset(&info, NULL, sizeof(BY_HANDLE_FILE_INFORMATION));
				if (GetFileInformationByHandle(handle, &info) == 0)
				{
					// ���o�ɮ׸�T����
					strLog.Format("GetFileInformationByHandle����![%s]", mtk->m_chFilename);
					log->DFLogSend(strLog, DF_SEVERITY_HIGH);
					
					ReleaseSRWLockExclusive(&m_lockFileList);
					continue;
				}
			
				// �ɮ��ݩ�(��Ū?)
				mtk->m_info.m_attribute = info.dwFileAttributes & FILE_ATTRIBUTE_READONLY ? TRUE : FALSE;

				// �ɮ׳̫�g�J�ɶ�
				if (CompareFileTime(&(mtk->m_info.m_lastwrite), &(info.ftLastWriteTime))!= 0/*(UINT64)*(UINT64*)&(mtk->m_info.m_lastwrite) != (UINT64)*(UINT64*)&(info.ftLastWriteTime)*/) // �૬�A���FILETIME
				{
					mtk->m_info.m_lastwrite = info.ftLastWriteTime;
					mtk->m_dwLastwrite = GetTickCount();
				}
				
				// �ɮפj�p
				LARGE_INTEGER size;
				size.QuadPart = 0;
				size.HighPart = info.nFileSizeHigh;
				size.LowPart = info.nFileSizeLow;
				mtk->m_info.m_size = size.QuadPart;
			}

			// ���O��Ū���ɮ�=>+1��=>�~�򬡵�
			if (!mtk->m_info.m_attribute) 
			{
				mtk->m_age = m_uAge;
			}
			// ��Ū���ɮ�=>�M���ӷ��D��=>���ۦ^��
			else 
			{
				mtk->m_master = NULL;
			}

			CloseHandle(handle);
			handle = NULL;
		}
		ReleaseSRWLockExclusive(&m_lockFileList);
	}
	
	finder.Close();

	// �U������
	AcquireSRWLockExclusive(&m_lockFileList);
	for (int i=0; i<m_arrMtkFile.GetCount();)
	{
		// ���z������
		//if (i >= m_arrMtkFile.GetCount())
		//	break;

		MTK* mtk = m_arrMtkFile.GetAt(i);
		AcquireSRWLockExclusive(&mtk->m_lock);
		BOOL bMaster = (mtk->m_master != NULL) ? TRUE : FALSE;
		ReleaseSRWLockExclusive(&mtk->m_lock);

		if (!bMaster /*&& mtk->m_info.m_attribute*/ && mtk->m_age < m_uAge)
		{
			strLog.Format("�����ɮ�![%s]", mtk->m_chFilename);
			log->DFLogSend(strLog, DF_SEVERITY_INFORMATION);
		
			m_arrMtkFile.RemoveAt(i);
			m_mapMtkFile.RemoveKey(mtk->m_chFilename);	
			
			CloseHandle(mtk->m_info.m_handle);
			delete mtk;
			mtk = NULL;
	
			bGrowUp = TRUE;
			continue;
		}
		else
		{
			i++;
			continue;
		}

		//if (mtk->m_ended && bMaster && !mtk->m_info.m_attribute && mtk->m_ended && (mtk->m_age+2) < m_uAge) // �ɮרS�ǧ�,��server�w��readonly�R�O,�B�L�F2�g��
		//{
		//	strLog.Format("�����ɮ�![ERROR][%s]", mtk->m_chFilename);
		//	log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
		//
		//	m_arrMtkFile.RemoveAt(i);
		//	m_mapMtkFile.RemoveKey(mtk->m_chFilename);	
		//	
		//	CString strFile("");
		//	strFile.Format("%s%s", theApp.m_chFileDir, mtk->m_chFilename);
		//	SetFileAttributes(strFile, FILE_ATTRIBUTE_READONLY);
		//	CloseHandle(mtk->m_info.m_handle);
		//	delete mtk;
		//	mtk = NULL;
	
		//	bGrowUp = TRUE;
		//	continue;
		//}

		//else
		//	mtk->m_age = m_uAge;
		// unlock
	}
	ReleaseSRWLockExclusive(&m_lockFileList);

	if (bGrowUp)
		InterlockedIncrement(&m_uGrowAge);
}

BOOL CDFFileMaster::fnCheckFileFilter(char* name)
{
	if (theApp.m_bFilter2Tick == FALSE)
		return FALSE;

	for (int i=0; i<theApp.m_arrFilter.GetCount(); i++)
	{
		CString strFilter = theApp.m_arrFilter.GetAt(i);
		char* filter = strFilter.GetBuffer(0);
		if (wildcmp(filter, name))
		{
			return TRUE;
		}
	}

	return FALSE;
}

int CDFFileMaster::wildcmp(const char *wild, const char *string) 
{
	// Written by Jack Handy - <A href="mailto:jakkhandy@hotmail.com">jakkhandy@hotmail.com</A>
	const char *cp = NULL, *mp = NULL;

	while ((*string) && (*wild != '*'))
	{
		if ((*wild != *string) && (*wild != '?')) 
		{
			return 0;
		}
		wild++;
		string++;
	}

	while (*string) 
	{
		if (*wild == '*') 
		{
			if (!*++wild) 
			{
				return 1;
			}
			mp = wild;
			cp = string+1;
		} 
		else if ((*wild == *string) || (*wild == '?')) 
		{
			wild++;
			string++;
		} 
		else 
		{
			wild = mp;
			string = cp++;
		}
	}

	while (*wild == '*') 
	{
		wild++;
	}
	
	return !*wild;
}
