
// DFNet.cpp : 定義檔案類別。
//

#include "stdafx.h"
#include "DFNet.h"
#include "DDFileClient.h"
#include "DDFileClientDlg.h"
#include "zstd.h"

__declspec(thread) ZSTD_DCtx* m_zstd;

// CDFNetMaster class implementation
// 
CDFNetMaster::CDFNetMaster(AFX_THREADPROC pfnThreadProc, LPVOID param, UINT id, char* host, UINT port)
: CMyThread(pfnThreadProc, param)
, m_socket(NULL)
, m_packet(NULL)
, m_debug(TRUE)
, m_NetServerID(id)
, m_NetServerPort(port)
, m_NetServerLastTick(0)
, m_NetServerFileCount(0)
, m_NetServerVolumn(0)
, m_NetServerTotalVolumn(0)
, m_NetServerComVolumn(0)
, m_NetServerTotalComVolumn(0)
{
	// 更新UI狀態
	InitializeSRWLock(&m_NetServerStatusLock);
	AcquireSRWLockExclusive(&m_NetServerStatusLock);
	SYSTEMTIME start;
	GetLocalTime(&start);
	sprintf_s(m_NetServerStartTime, "%02d-%02d %02d:%02d:%02d", start.wMonth, start.wDay, start.wHour, start.wMinute, start.wSecond);
	m_NetServerLastTick = GetTickCount();
	ReleaseSRWLockExclusive(&m_NetServerStatusLock);
	
	memset(m_NetServerIP, NULL, 16);
	memcpy(m_NetServerIP, host, strlen(host));
	
	m_mapFileList.RemoveAll();
	m_vecSyncList.clear();
}

/*virtual */CDFNetMaster::~CDFNetMaster(void)
{
	m_mapFileList.RemoveAll();
	m_vecSyncList.clear();
}

/*virtual */void CDFNetMaster::Go(void)
{
	CDDFileClientDlg* dlg = (CDDFileClientDlg*)GetParentsHandle();
	CDFFileMaster* file = dlg->m_ptDFFile;
	CDFLogMaster* log = dlg->m_ptDFLog;
	CString strLog("");
	
	ReleaseSRWLockExclusive(&dlg->m_lockServer);

START:
	int infolen = sizeof(MTKINFO);
	int wordlen = sizeof(DWORD);
	
	while (1)
	{
		if (IsStop())
			break;

		if (CreateSocket())
		{
			if (Login())
			{
				break;
			}
			else 
			{
				strLog.Format("登入失敗!重新連線...");
				log->DFLogSend(strLog, DF_SEVERITY_WARNING);
				Sleep(10000);
				DeleteSocket();
			}
		}

		strLog.Format("連線失敗!重新連線...");
		log->DFLogSend(strLog, DF_SEVERITY_WARNING);
		Sleep(10000);
	}

	m_packet = new PACKET();
	memset(m_packet, NULL, sizeof(PACKET));

	m_depacket = new PACKET();
	memset(m_depacket, NULL, sizeof(PACKET));

//	HEADER *head = new HEADER();
//	memset(head, NULL, sizeof(HEADER));

	m_zstd = ZSTD_createDCtx();

	WSABUF DataBuf;
	memset(&DataBuf, NULL, sizeof(WSABUF));

	WSAEVENT RecvEvent = WSACreateEvent();
	WSAEVENT SendEvent = WSACreateEvent();

	if (RecvEvent == WSA_INVALID_EVENT || SendEvent == WSA_INVALID_EVENT)
	{
		strLog.Format("WSACreateEvent失敗![%d]", WSAGetLastError());
		log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
		EndThread();
		return;
	}

	WSAOVERLAPPED RecvOverlapped;
	memset(&RecvOverlapped, NULL, sizeof(WSAOVERLAPPED));
	RecvOverlapped.hEvent = RecvEvent;

	WSAOVERLAPPED SendOverlapped;
	memset(&SendOverlapped, NULL, sizeof(WSAOVERLAPPED));
	SendOverlapped.hEvent = SendEvent;

	DWORD dwHeartBeat = GetTickCount();
//	BOOL bForceSync = TRUE;
	BOOL bResetRecv = TRUE;
	DWORD dFlag = 0;
	DWORD n=0;
	UINT64 tra = 0;

	BOOL bDisconnect = FALSE;

	// FOR DEBUG
	while (!bDisconnect && !IsStop())
	{
		DWORD dwHB = GetTickCount();
		BOOL bCompress = FALSE;
//		dFlag = 0;
//		n = 0;

		if (bResetRecv)
		{
			memset(&DataBuf, NULL, sizeof(WSABUF));
			memset(m_packet, NULL, sizeof(PACKET));
			
			DataBuf.len = sizeof(DWORD);
			DataBuf.buf = (char*)&m_packet->m_pktlen;
			
			n=0;
			dFlag = 0;
			WSARecv(m_socket, &DataBuf, 1, &n, &dFlag, &RecvOverlapped, NULL);
			bResetRecv = FALSE;
		}

		DWORD nRecv = 0; // 取代n,會有同步問題,所以n不能用
		DWORD dw = WaitForSingleObject(RecvOverlapped.hEvent, 1000);
		if (dw == WAIT_OBJECT_0)
		{
			if (GetOverlappedResult((HANDLE)m_socket, &RecvOverlapped, &n, TRUE) != 0)   
			{
				if (n != 4) 
				{
					strLog.Format("奇怪的事情 資料長度有誤![%d]", n);
					log->DFLogSend(strLog, DF_SEVERITY_WARNING);
					break;
				}
			}
			else
			{
				strLog.Format("連線中斷![0]");
				log->DFLogSend(strLog, DF_SEVERITY_WARNING);
				break;
			}
			
			bCompress = ((m_packet->m_pktlen&ZSTD_COMPRESS) == ZSTD_COMPRESS);
			m_packet->m_pktlen &= ~ZSTD_COMPRESS;
			nRecv = 0;
			while (nRecv < m_packet->m_pktlen)
				nRecv += recv(m_socket, m_packet->m_pkt+nRecv, m_packet->m_pktlen-nRecv, 0);

			if ((nRecv==0) || (nRecv != m_packet->m_pktlen))
			{
				strLog.Format("奇怪的事情 資料有誤![%d][%d]", nRecv, m_packet->m_pktlen);
				log->DFLogSend(strLog, DF_SEVERITY_WARNING);
				break;
			}

			bResetRecv = TRUE;
			WSAResetEvent(RecvOverlapped.hEvent);
		}
		else if (dw == WAIT_TIMEOUT)
		{
			// TIMEOUT (沒事的,只是現在傳送的資料較少) (順便檢查一下連線狀況好了)
			if (!WSAGetOverlappedResult(m_socket, &RecvOverlapped, &n, FALSE, &dFlag) && GetLastError() != ERROR_IO_INCOMPLETE)
			{
				strLog.Format("連線中斷![1]");
				log->DFLogSend(strLog, DF_SEVERITY_WARNING);
				break;
			}

			//if (!GetOverlappedResult((HANDLE)m_socket, &RecvOverlapped, &n, FALSE) && GetLastError() != ERROR_IO_INCOMPLETE)
			//{
			//	strLog.Format("連線中斷![1]");
			//	log->DFLogSend(strLog, DF_SEVERITY_WARNING);
			//	break;
			//}
		}
		else
		{
			// FAIL 結束
			strLog.Format("奇怪的事情 不知道回傳甚麼![%d]", dw);
			log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
			break;
		}

		if ((nRecv>0) && (nRecv==m_packet->m_pktlen/*+4*/))
		{
			//TRACE("[%I64d]%d\n", tra++, nRecv+4);

			if (bCompress) // 需解壓縮
			{
				UINT nOutLen = 0;
				memset(m_depacket, NULL, sizeof(PACKET));
				if (!ZSTD_isError(nOutLen=(int)ZSTD_decompress_usingDDict(m_zstd, (char*)m_depacket, sizeof(PACKET), (const char*)m_packet->m_pkt, m_packet->m_pktlen, theApp.m_ptDict/*, theApp.m_uDictSize*/)))
				{
					// 解壓縮成功
					memset(m_packet, NULL, sizeof(PACKET));
						
					//m_packet->m_pktlen = nOutLen-sizeof(int);	// len(OR)壓縮旗標
					memcpy(m_packet, m_depacket, nOutLen);

					if (m_packet->m_pktlen != nOutLen-sizeof(int))
					{
						strLog.Format("奇怪的事情 解壓縮長度不符合![%d][%d]", m_packet->m_pktlen, nOutLen-sizeof(int));
						log->DFLogSend(strLog, DF_SEVERITY_HIGH);
					}
				}
				else
				{
					strLog.Format("解壓縮失敗![%s][%d]", ZSTD_getErrorName(nOutLen), m_packet->m_pktlen);
					log->DFLogSend(strLog, DF_SEVERITY_HIGH);
					continue;
				}
			}

			AcquireSRWLockExclusive(&m_NetServerStatusLock);
			DWORD lasttick = GetTickCount();
			if ((lasttick-m_NetServerLastTick) >= 1000)
			{
				m_NetServerComVolumn = nRecv/*rcvlen*/;									// 每秒傳輸量(壓縮)(重置)
				m_NetServerVolumn = m_packet->m_pktlen;									// 每秒傳輸量(實際)(重置)	
				m_NetServerLastTick = lasttick;	
			}
			else 
			{
				m_NetServerComVolumn += nRecv/*rcvlen*/;								// 每秒傳輸量(壓縮)
				m_NetServerVolumn += m_packet->m_pktlen;								// 每秒傳輸量(實際)
			}

			m_NetServerTotalComVolumn += nRecv/*rcvlen*/;										// 總傳輸量(壓縮)
			m_NetServerTotalVolumn += m_packet->m_pktlen;								// 總傳輸量(實際)
			m_NetServerFileCount = m_mapFileList.GetCount();							// 總檔案數												
			ReleaseSRWLockExclusive(&m_NetServerStatusLock);

			// 收到資料
			UINT idx = 0;
			UINT total = m_packet->m_pktlen;

			DWORD tick = GetTickCount();

			while (!bDisconnect && (idx < total))
			{
				MTKINFO* info = (MTKINFO*)&m_packet->m_pkt[idx];
				DWORD datalen = 0;
				memcpy(&datalen,  &m_packet->m_pkt[idx+infolen], wordlen);
				char* data = &m_packet->m_pkt[idx+infolen+wordlen];

				if (info->m_handle == NULL)
				{
					strLog.Format("錯誤資料![handle:NULL]");
					log->DFLogSend(strLog, DF_SEVERITY_HIGH);
					idx += (infolen + wordlen + datalen);
					continue;
				}

				//if (info->m_size > 4294868990)
				//{
				//	TRACE("");
				//}

				switch ((DWORD)info->m_attribute)
				{
				case PACKET_WRITE:
					{
						MTK* mtk = NULL;
						if (!m_mapFileList.Lookup((UINT&)info->m_handle, (MTK*&)mtk))
						{
							if (m_debug)
							{
								strLog.Format("找不到的檔案![0][handle:%x]", info->m_handle);
								log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
							}

							// 也許是漏了新檔案通知封包 同步檔名
							REQUEST *req = new REQUEST;
							memset(req, NULL, sizeof(REQUEST));

							req->m_id = 2;
							req->m_handle = info->m_handle;
							req->m_offset = 0;

							m_vecSyncList.push_back(req);
							idx += (infolen + wordlen + datalen);
							continue;
						}
						
						if (mtk == NULL)
						{
							strLog.Format("檔案已被刪除![0]");
							log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
							break;
						}

						if (mtk->m_info.m_handle == NULL)
						{
							strLog.Format("檔案操作失敗![0][%s]", mtk->m_chFilename);
							log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
							break;
						}

						if (mtk->m_master && (mtk->m_master->m_NetServerID != this->m_NetServerID))
						{
							strLog.Format("不是我的檔案![0][%s]", mtk->m_chFilename);
							log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
							break;
						}

						if (mtk->m_info.m_attribute == TRUE)
						{
							strLog.Format("唯讀檔案![0][%s]", mtk->m_chFilename);
							log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
								
							// 取消訂閱
							REQUEST *req = new REQUEST;
							memset(req, NULL, sizeof(REQUEST));

							req->m_id = 0;
							req->m_handle = info->m_handle;
							req->m_offset = 0;

							m_vecSyncList.push_back(req);
							//m_vecSyncList.push_back((UINT)info->m_handle);
							idx += (infolen + wordlen + datalen);
							continue;
						}

						DWORD ret = 0;
						if (datalen > 0)
						{

							// DEBUG
							//if (strcmp(mtk->m_chFilename, "20181011.QNyse.dbo.ordr.mtk") == 0)
							//{
							//	strLog.Format("CHECKFILE![0][%s][%I64d][%I64d]", mtk->m_chFilename, *(UINT64*)&mtk->m_overlapped.Offset, *(UINT64*)&info->m_size);
							//	log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
							//}
							
							if (*(UINT64*)&mtk->m_overlapped.Offset != info->m_size/*其實是OFFSET*/)
							{
								strLog.Format("檔案不同步![0][%s][%I64d][%I64d]", mtk->m_chFilename, *(UINT64*)&mtk->m_overlapped.Offset, *(UINT64*)&info->m_size);
								log->DFLogSend(strLog, DF_SEVERITY_DEBUG);

								// 同步檔案
								REQUEST* req = new REQUEST;
								memset(req, NULL, sizeof(REQUEST));

								req->m_id = 1;
								req->m_handle = info->m_handle;
								req->m_offset = *(UINT64*)&mtk->m_overlapped.Offset;

								m_vecSyncList.push_back(req);
								//m_vecSyncList.push_back((UINT)info->m_handle);
								idx += (infolen + wordlen + datalen);
								continue;
							}

							if (!WriteFile(mtk->m_info.m_handle, data, datalen, &ret, &mtk->m_overlapped))
							{
								if (GetLastError() == ERROR_IO_PENDING)
								{
									DWORD dw = WaitForSingleObject(mtk->m_overlapped.hEvent, INFINITE);

									if (dw == WAIT_OBJECT_0)
									{
										if (GetOverlappedResult(mtk->m_info.m_handle, &mtk->m_overlapped, &ret, TRUE) != 0)
										{
											if (ret <= 0)
											{
												strLog.Format("奇怪的事情 沒寫入資料![0][%d][%s]", ret, mtk->m_chFilename);
												log->DFLogSend(strLog, DF_SEVERITY_WARNING);
											}
										}
									}
									else
									{
										strLog.Format("奇怪的事情 奇怪的結果![0][%d][%d][%s]", dw, ret, mtk->m_chFilename);
										log->DFLogSend(strLog, DF_SEVERITY_WARNING);
									}

									//ResetEvent(mtk->m_overlapped.hEvent);
								}
								else
								{
									strLog.Format("檔案寫入失敗![0][%d][%s]", GetLastError(), mtk->m_chFilename);
									log->DFLogSend(strLog, DF_SEVERITY_HIGH);
										
									bDisconnect = TRUE;

									idx += (infolen + wordlen + datalen);
									continue;
								}
							}

							ResetEvent(mtk->m_overlapped.hEvent);
						}
						else 
						{
							// DEBUG
							//if (strcmp(mtk->m_chFilename, "20181011.QNyse.dbo.ordr.mtk") == 0)
							//{
							//	strLog.Format("CHECKFILE![0][%s][%I64d][%I64d]", mtk->m_chFilename, *(UINT64*)&mtk->m_overlapped.Offset, *(UINT64*)&info->m_size);
							//	log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
							//}

							if (*(UINT64*)&mtk->m_overlapped.Offset != info->m_size)
							{
								strLog.Format("檔案不同步!?[0][File:%s][Local:%I64d][Remote:%I64d]", mtk->m_chFilename, *(UINT64*)&mtk->m_overlapped.Offset, *(UINT64*)&info->m_size);
								log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
							
								// 同步檔案
								REQUEST* req = new REQUEST;
								memset(req, NULL, sizeof(REQUEST));

								req->m_id = 1;
								req->m_handle = info->m_handle;
								req->m_offset = *(UINT64*)&mtk->m_overlapped.Offset;

								m_vecSyncList.push_back(req);
								//m_vecSyncList.push_back((UINT)info->m_handle);
								idx += (infolen + wordlen + datalen);
								continue;
							}
						}

						if (datalen != ret)
						{
							strLog.Format("寫檔不完全!?[0][File:%s][Recv:%d][Write:%d]", mtk->m_chFilename, datalen, ret);
							log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
						}

						// DEBUG
						//if (strcmp(mtk->m_chFilename, "20171024.QOpt.dbo.list1500t.mtk")==0)
						//{
						//	strLog.Format("除錯![0][File:%s][Local:%I64d][Recv:%d][Write:%d][Remote:%I64d]", mtk->m_chFilename, *(UINT64*)&mtk->m_overlapped.Offset, datalen, ret, *(UINT64*)&info->m_size);
						//	log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
						//}

						// FOR DEBUG
						//if	(( strcmp(mtk->m_chFilename, "20171020.QOpt.dbo.list1500.mtk")==0 
						//	|| strcmp(mtk->m_chFilename, "20171020.QOpt.dbo.list1500t.mtk")==0 
						//	|| strcmp(mtk->m_chFilename, "20171020.QOpt.dbo.ordr1500.mtk")==0 
						//	|| strcmp(mtk->m_chFilename, "20171020.QOpt.dbo.ordr1500t.mtk")==0 ) && ((mtk->m_info.m_size+datalen) != info->m_size))
						//{
						//	FILETIME local;
						//	FileTimeToLocalFileTime(&info->m_lastwrite, &local);

						//	SYSTEMTIME syslocal;
						//	FileTimeToSystemTime(&local, &syslocal);

						//	TRACE("%s: Pkt(%d) Data(%d) LOCAL(%I64d) REMOTE(%I64d) LAST(%02d%02d%02d)\n", mtk->m_chFilename, m_packet->m_pktlen, datalen, mtk->m_info.m_size, info->m_size, syslocal.wHour, syslocal.wMinute, syslocal.wSecond);
						//}

						//SetFileTime(mtk->m_info.m_handle, NULL, NULL, &info->m_lastwrite);

						AcquireSRWLockExclusive(&mtk->m_lock);
						
						//[2018/02/09] 每層0-40秒延遲,造成四~五層架構中,HeartBeat延遲過高,發生警示 => 改為收到資料(或HeartBeat),即刻押上當前時間,收到ReadOnly仍維持押上Server檔案時間(與Server端同步)
						//mtk->m_info.m_lastwrite = info->m_lastwrite;
						SYSTEMTIME stime;
						FILETIME ftime;
						GetSystemTime(&stime);
						SystemTimeToFileTime(&stime, &ftime);
						
						if (CompareFileTime(&(mtk->m_info.m_lastwrite), &(info->m_lastwrite))== -1)
						{
							SetFileTime(mtk->m_info.m_handle, NULL, NULL, &ftime);
							mtk->m_info.m_lastwrite = ftime;
							mtk->m_dwLastwrite = tick;
						}
						mtk->m_info.m_size += ret;
						
						//mtk->m_info.m_attribute = FALSE;

						// FOR UI
						DWORD dwtick = GetTickCount();
						if ((dwtick-mtk->m_lasttime) >= 1000) 
						{
							mtk->m_flow = ret;
							mtk->m_lasttime = dwtick;
						}
						else 
						{
							mtk->m_flow += ret;
						}

						if (mtk->m_flow > mtk->m_maxflow) 
							mtk->m_maxflow = mtk->m_flow;

						ReleaseSRWLockExclusive(&mtk->m_lock);

						UINT64 offset = *(UINT64*)&mtk->m_overlapped.Offset + ret;
						memcpy(&mtk->m_overlapped.Offset, &offset, sizeof(UINT64));

						// CHECKSUM(TODO)

						//// 更新UI status
						//if (m_debug)
						//{
						//	strLog.Format("更新UI資訊開始[0]");
						//	log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
						//}

						//AcquireSRWLockExclusive(&m_NetServerStatusLock);
						//DWORD lasttick = GetTickCount();
						//if ((lasttick-m_NetServerLastTick) >= 1000)
						//{
						//	m_NetServerVolumn = ret;											// 每秒傳輸量(Reset)	
						//	m_NetServerLastTick = lasttick;	
						//}
						//else 
						//	m_NetServerVolumn += ret;											// 每秒傳輸量
						//
						//m_NetServerTotalVolumn += ret;											// 總傳輸量
						//m_NetServerFileCount = m_mapFileList.GetCount();						// 總檔案數
						//										
						//ReleaseSRWLockExclusive(&m_NetServerStatusLock);

						//if (m_debug)
						//{
						//	strLog.Format("更新UI資訊完畢[0]");
						//	log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
						//}

						break;
					}
				case PACKET_READONLY:
					{
						MTK* mtk = NULL;
						if (!m_mapFileList.Lookup((UINT&)info->m_handle, (MTK*&)mtk))
						{
							if (m_debug)
							{
								strLog.Format("找不到的檔案![1][handle:%x]", info->m_handle);
								log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
							}

							// 同步檔名
							REQUEST* req = new REQUEST;
							memset(req, NULL, sizeof(REQUEST));

							req->m_id = 2;
							req->m_handle = info->m_handle;
							req->m_offset = 0;

							m_vecSyncList.push_back(req);
							idx += (infolen + wordlen + datalen);
							continue;
						}
						
						if (mtk == NULL)
						{
							strLog.Format("檔案已被刪除![1]");
							log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
							break;
						}

						if (mtk->m_info.m_handle == NULL)
						{
							strLog.Format("檔案操作失敗![1][%s]", mtk->m_chFilename);
							log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
							break;
						}

						if (mtk->m_master->m_NetServerID != this->m_NetServerID)
						{
							strLog.Format("不是我的檔案![1][%s]", mtk->m_chFilename);
							log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
							break;
						}

						if (mtk->m_info.m_attribute == TRUE)
						{
							strLog.Format("唯讀檔案![1][%s]", mtk->m_chFilename);
							log->DFLogSend(strLog, DF_SEVERITY_DISASTER);

							// 取消訂閱
							REQUEST* req = new REQUEST;
							memset(req, NULL, sizeof(REQUEST));

							req->m_id = 0;
							req->m_handle = info->m_handle;
							req->m_offset = 0;

							m_vecSyncList.push_back(req);
							//m_vecSyncList.push_back((UINT)info->m_handle);
							idx += (infolen + wordlen + datalen);
							continue;
						}

						DWORD ret = 0;
						if (datalen > 0)	
						{
							// DEBUG
							//if (strcmp(mtk->m_chFilename, "20181011.QNyse.dbo.ordr.mtk") == 0)
							//{
							//	strLog.Format("CHECKFILE![1][%s][%I64d][%I64d]", mtk->m_chFilename, *(UINT64*)&mtk->m_overlapped.Offset, *(UINT64*)&info->m_size);
							//	log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
							//}

							// DEBUG
							if (*(UINT64*)&mtk->m_overlapped.Offset != info->m_size/*其實是OFFSET*/)
							{
								strLog.Format("檔案不同步![1][%s][%I64d][%I64d]", mtk->m_chFilename, *(UINT64*)&mtk->m_overlapped.Offset, *(UINT64*)&info->m_size);
								log->DFLogSend(strLog, DF_SEVERITY_DEBUG);

								// 同步檔案
								REQUEST* req = new REQUEST;
								memset(req, NULL, sizeof(REQUEST));

								req->m_id = 1;
								req->m_handle = info->m_handle;
								req->m_offset = *(UINT64*)&mtk->m_overlapped.Offset;

								m_vecSyncList.push_back(req);
								//m_vecSyncList.push_back((UINT)info->m_handle);
								idx += (infolen + wordlen + datalen);
								continue;
							}

							if (!WriteFile(mtk->m_info.m_handle, data, datalen, &ret, &mtk->m_overlapped))
							{
								if (GetLastError() == ERROR_IO_PENDING)
								{
									DWORD dw = WaitForSingleObject(mtk->m_overlapped.hEvent, INFINITE);

									if (dw == WAIT_OBJECT_0)
									{
										if (GetOverlappedResult(mtk->m_info.m_handle, &mtk->m_overlapped, &ret, TRUE) != 0)
										{
											if (ret <= 0)
											{
												strLog.Format("奇怪的事情 沒寫入資料![1][%d][%s]", ret, mtk->m_chFilename);
												log->DFLogSend(strLog, DF_SEVERITY_WARNING);
											}
										}
									}
									else
									{
										strLog.Format("奇怪的事情 奇怪的結果![0][%d][%d][%s]", dw, ret, mtk->m_chFilename);
										log->DFLogSend(strLog, DF_SEVERITY_WARNING);
									}

									//ResetEvent(mtk->m_overlapped.hEvent);
								}
								else
								{
									strLog.Format("檔案寫入失敗![1][%d][%s]", GetLastError(), mtk->m_chFilename);
									log->DFLogSend(strLog, DF_SEVERITY_HIGH);
										
									bDisconnect = TRUE;

									idx += (infolen + wordlen + datalen);
									continue;
								}
							}

							ResetEvent(mtk->m_overlapped.hEvent);
						}
						else
						{
							// DEBUG
							//if (strcmp(mtk->m_chFilename, "20181011.QNyse.dbo.ordr.mtk") == 0)
							//{
							//	strLog.Format("CHECKFILE![1][%s][%I64d][%I64d]", mtk->m_chFilename, *(UINT64*)&mtk->m_overlapped.Offset, *(UINT64*)&info->m_size);
							//	log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
							//}

							if (*(UINT64*)&mtk->m_overlapped.Offset != info->m_size)
							{
								strLog.Format("檔案不同步!?[1][File:%s][Local:%I64d][Remote:%I64d]", mtk->m_chFilename, *(UINT64*)&mtk->m_overlapped.Offset, *(UINT64*)&info->m_size);
								log->DFLogSend(strLog, DF_SEVERITY_DISASTER);

								// 同步檔案
								REQUEST* req = new REQUEST;
								memset(req, NULL, sizeof(REQUEST));

								req->m_id = 1;
								req->m_handle = info->m_handle;
								req->m_offset = *(UINT64*)&mtk->m_overlapped.Offset;

								m_vecSyncList.push_back(req);
								//m_vecSyncList.push_back((UINT)info->m_handle);
								idx += (infolen + wordlen + datalen);
								continue;
							}
						}

						if (datalen != ret)
						{
							strLog.Format("寫檔不完全!?[1][File:%s][Recv:%d][Write:%d]", mtk->m_chFilename, datalen, ret);
							log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
						}

						SetFileTime(mtk->m_info.m_handle, NULL, NULL, &info->m_lastwrite);

						AcquireSRWLockExclusive(&mtk->m_lock);
						mtk->m_info.m_lastwrite = info->m_lastwrite;
						mtk->m_info.m_size += ret;
						mtk->m_ended = TRUE;
						mtk->m_dwLastwrite = tick;
						//mtk->m_info.m_attribute = TRUE;

						// FOR UI
						DWORD dwtick = GetTickCount();
						if ((dwtick-mtk->m_lasttime) >= 1000) 
						{
							mtk->m_flow = ret;
							mtk->m_lasttime = dwtick;
						}
						else 
						{
							mtk->m_flow += ret;
						}

						if (mtk->m_flow > mtk->m_maxflow) 
							mtk->m_maxflow = mtk->m_flow;

						ReleaseSRWLockExclusive(&mtk->m_lock);

						UINT64 offset = *(UINT64*)&mtk->m_overlapped.Offset + ret;
						memcpy(&mtk->m_overlapped.Offset, &offset, sizeof(UINT64));

						if (mtk->m_info.m_size == info->m_size) // 接收完畢
						{
							CString strFile("");
							strFile.Format("%s%s", theApp.m_chFileDir, mtk->m_chFilename);
							SetFileAttributes(strFile, FILE_ATTRIBUTE_READONLY);							
							m_mapFileList.RemoveKey((UINT&)info->m_handle);
								
							AcquireSRWLockExclusive(&mtk->m_lock);
							CloseHandle(mtk->m_info.m_handle);
							mtk->m_info.m_handle = NULL;
							mtk->m_info.m_attribute = TRUE;
							mtk->m_master = NULL;
							ReleaseSRWLockExclusive(&mtk->m_lock);

							strLog.Format("檔案接收完畢![%s]", mtk->m_chFilename);
							log->DFLogSend(strLog, DF_SEVERITY_INFORMATION);
						}

						break;
					}
				case PACKET_NEWFILE: // 新的檔案
					{
						char chFilename[256]={0};
						memcpy(chFilename, data, datalen); 
	
						char chMtkVer[9] = {0};
						char* chMtkName = NULL;			
						memcpy(chMtkVer, chFilename, 8);
						chMtkName = chFilename+9;

						AcquireSRWLockExclusive(&file->m_lockFileList);
						MTK* mtk = NULL;
						if (!file->m_mapMtkFile.Lookup(chFilename, (void*&)mtk))
						{
							// 全新的檔案, 所以要新增一個
							mtk = new MTK();
							memset(mtk, NULL, sizeof(MTK));
							memcpy(mtk->m_chFilename, data, datalen); 
							memcpy(mtk->m_chMtkname, chMtkName, strlen(chMtkName));
							mtk->m_uMtkver = atoi(chMtkVer);
							mtk->m_master = this;
							mtk->m_filter = file->fnCheckFileFilter(mtk->m_chFilename);
							mtk->m_ended = FALSE;
							mtk->m_age = file->m_uAge;
							InitializeSRWLock(&mtk->m_lock);

							if (mtk->m_filter)
							{
								delete mtk;
								mtk = NULL;
									
								// 取消訂閱
								REQUEST* req = new REQUEST;
								memset(req, NULL, sizeof(REQUEST));

								req->m_id = 0;
								req->m_handle = info->m_handle;
								req->m_offset = 0;

								m_vecSyncList.push_back(req);
								//m_vecSyncList.push_back((UINT)info->m_handle);
								idx += (infolen + wordlen + datalen);
								ReleaseSRWLockExclusive(&file->m_lockFileList);
								continue;
							}

							file->m_arrMtkFile.Add(mtk);
							file->m_mapMtkFile.SetAt(mtk->m_chFilename, (void*&)mtk);
							InterlockedIncrement(&file->m_uGrowAge);
						}
						else
						{
							// 已有紀錄的檔案
							AcquireSRWLockExclusive(&mtk->m_lock);
								
							BOOL bMaster = (mtk->m_master == NULL) ? FALSE : TRUE;
							if (!bMaster)
								mtk->m_master = this;
								
							BOOL bFilter = mtk->m_filter;
							if (bFilter)
								mtk->m_master = NULL;
								
							BOOL bReadonly = mtk->m_info.m_attribute;
								
							ReleaseSRWLockExclusive(&mtk->m_lock);

							if (bFilter)
							{
								// 取消訂閱
								REQUEST* req = new REQUEST;
								memset(req, NULL, sizeof(REQUEST));

								req->m_id = 0;
								req->m_handle = info->m_handle;
								req->m_offset = 0;

								m_vecSyncList.push_back(req);
								//m_vecSyncList.push_back((UINT)info->m_handle);
								idx += (infolen + wordlen + datalen);
								ReleaseSRWLockExclusive(&file->m_lockFileList);
								continue;
							}

							if (bMaster && (m_NetServerID != mtk->m_master->m_NetServerID))
							{
								strLog.Format("已有其他資料源提供![主機:%d][其他主機:%d][%s]", m_NetServerID, mtk->m_master->m_NetServerID, mtk->m_chFilename);
								log->DFLogSend(strLog, DF_SEVERITY_HIGH);

								// 取消訂閱
								REQUEST* req = new REQUEST;
								memset(req, NULL, sizeof(REQUEST));

								req->m_id = 0;
								req->m_handle = info->m_handle;
								req->m_offset = 0;

								m_vecSyncList.push_back(req);
								//m_vecSyncList.push_back((UINT)info->m_handle);
								idx += (infolen + wordlen + datalen);
								ReleaseSRWLockExclusive(&file->m_lockFileList);
								continue;
							}

							if (bReadonly)
							{
								// 取消訂閱
								REQUEST* req = new REQUEST;
								memset(req, NULL, sizeof(REQUEST));

								req->m_id = 0;
								req->m_handle = info->m_handle;
								req->m_offset = 0;

								m_vecSyncList.push_back(req);
								//m_vecSyncList.push_back((UINT)info->m_handle);
								idx += (infolen + wordlen + datalen);
								ReleaseSRWLockExclusive(&file->m_lockFileList);
								continue;
							}
						}
						ReleaseSRWLockExclusive(&file->m_lockFileList);

						mtk->m_overlapped.Offset = 0;
						mtk->m_overlapped.OffsetHigh = 0;
						if (mtk->m_overlapped.hEvent == NULL) // 只做一次
						{
							mtk->m_overlapped.hEvent = CreateEvent(NULL, true, false, NULL);
							if (mtk->m_overlapped.hEvent == NULL)
							{
								mtk->m_info.m_handle = NULL;
								mtk->m_master = NULL;

								strLog.Format("可怕的事情!檔案OVERLAPPED.EVENT建立失敗![%s]", mtk->m_chFilename);
								log->DFLogSend(strLog, DF_SEVERITY_HIGH);
								
								// 取消訂閱
								REQUEST* req = new REQUEST;
								memset(req, NULL, sizeof(REQUEST));

								req->m_id = 0;
								req->m_handle = info->m_handle;
								req->m_offset = 0;

								m_vecSyncList.push_back(req);
								//m_vecSyncList.push_back((UINT)info->m_handle);
								idx += (infolen + wordlen + datalen);
								continue;	
							}
						}
							
						CString strFilename("");
						strFilename.Format("%s%s", theApp.m_chFileDir, mtk->m_chFilename);
						if (mtk->m_info.m_handle == NULL) // 只做一次
						{
							if ((mtk->m_info.m_handle = CreateFile(strFilename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
							{
								UINT err = GetLastError();
								if (err = ERROR_ACCESS_DENIED)
								{
									strLog.Format("檔案開啟失敗!可能是唯讀檔案(?)[%d][%s]", err, strFilename);
									log->DFLogSend(strLog, DF_SEVERITY_WARNING);
								}
								else
								{
									strLog.Format("檔案開啟失敗![%d][%s]", err, strFilename);
									log->DFLogSend(strLog, DF_SEVERITY_HIGH);
								}

								// 不管是不是唯讀,只要打不開,這個檔案就不訂了
								mtk->m_info.m_handle = NULL;
								
								AcquireSRWLockExclusive(&mtk->m_lock);
								mtk->m_master = NULL;
								ReleaseSRWLockExclusive(&mtk->m_lock);

								// 取消訂閱
								REQUEST* req = new REQUEST;
								memset(req, NULL, sizeof(REQUEST));

								req->m_id = 0;
								req->m_handle = info->m_handle;
								req->m_offset = 0;

								m_vecSyncList.push_back(req);
								//m_vecSyncList.push_back((UINT)info->m_handle);
								idx += (infolen + wordlen + datalen);
								continue;
							}
						}

						// 通過這裡的檔案就一定要收

						if (mtk->m_info.m_handle) // 若檔案原本就存在,取得檔案資訊,若是新的也可以順便初始化:)
						{
							// 檔案資訊結構
							BY_HANDLE_FILE_INFORMATION info;
							memset(&info, NULL, sizeof(BY_HANDLE_FILE_INFORMATION));
							if (GetFileInformationByHandle(mtk->m_info.m_handle, &info) == 0)
							{
								strLog.Format("GetFileInformationByHandle失敗![%s]", mtk->m_chFilename);
								log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
								return;
							}
			
							AcquireSRWLockExclusive(&mtk->m_lock);
							
							// 檔案屬性(唯讀?)
							mtk->m_info.m_attribute = (info.dwFileAttributes & FILE_ATTRIBUTE_READONLY) ? TRUE : FALSE;
							mtk->m_dwLastwrite = GetTickCount();

							// 檔案最後寫入時間
							mtk->m_info.m_lastwrite = info.ftLastWriteTime;

							// 檔案大小
							LARGE_INTEGER size;
							size.QuadPart = 0;
							size.HighPart = info.nFileSizeHigh;
							size.LowPart = info.nFileSizeLow;
							mtk->m_info.m_size = size.QuadPart;
							
							// FOR UI
							mtk->m_lasttime = GetTickCount();
							mtk->m_flow = 0;
							mtk->m_maxflow = 0;

							ReleaseSRWLockExclusive(&mtk->m_lock);

							// ***必須更新OVERLAPPED,才能續寫
							memcpy(&mtk->m_overlapped.Offset, &size.QuadPart, sizeof(LARGE_INTEGER));
						}

						if (mtk->m_info.m_size > 0) // 已存在的檔案,需要跟sevrer同步offset
						{	
							// 取消訂閱
							REQUEST* req = new REQUEST;
							memset(req, NULL, sizeof(REQUEST));

							req->m_id = 1;
							req->m_handle = info->m_handle;
							req->m_offset = *(UINT64*)&mtk->m_overlapped.Offset;

							m_vecSyncList.push_back(req);
							//m_vecSyncList.push_back((UINT)info->m_handle);
						}

						// 加入工作列
						m_mapFileList.SetAt((UINT&)info->m_handle, mtk);
							
						strLog.Format("新的檔案![主機:%d][%s]", m_NetServerID, strFilename);
						log->DFLogSend(strLog, DF_SEVERITY_INFORMATION);
							
						break;
					}
				case PACKET_DELETEFILE:
					{
						break;
					}
				}

				idx += (infolen + wordlen + datalen);
			}

			dwHeartBeat = dwHB;
		}
		
		// 處理同步資訊
		char* chSendBuf = NULL;
		
		// 沒有需要同步的資料
		if (m_vecSyncList.size() == 0)
		{
/*			if (bForceSync)
			{
				REQUEST* req = new REQUEST;
				memset(req, NULL, sizeof(REQUEST));

				req->m_id = 0;
				req->m_handle = 0;
				req->m_offset = 0;

				m_vecSyncList.push_back(req);
			}
			else */if ((dwHB-dwHeartBeat)>30000)
			{
				REQUEST* req = new REQUEST;
				memset(req, NULL, sizeof(REQUEST));

				req->m_id = 0;
				req->m_handle = 0;
				req->m_offset = 0;

				m_vecSyncList.push_back(req);

				strLog.Format("我還活著!");
				log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
			}
			else
			{
				continue; // 離開同步模式
			}
		}
//		bForceSync = FALSE;

		int total = 0;
		//DWORD dwSendLen = m_vecSyncList.size() * 12;
		DWORD dwSendLen = m_vecSyncList.size() * 16;
		chSendBuf = new char[dwSendLen];
		memset(chSendBuf, NULL, dwSendLen);

		while (m_vecSyncList.size() != 0)
		{
			REQUEST* req = m_vecSyncList.back();
			//UINT key = m_vecSyncList.back();
			m_vecSyncList.pop_back();
					
			memcpy(chSendBuf+total, req, 16);
			total += 16;
			//MTK* mtk = NULL;
			//int idx = total;

			//if (!m_mapFileList.Lookup(key, mtk))
			//{
			//	// 找不到的取消訂閱
			//	memcpy(chSendBuf+idx, &key, sizeof(HANDLE));
			//	idx += sizeof(HANDLE);

			//	UINT64 zero = 0;
			//	memcpy(chSendBuf+idx, &zero, sizeof(UINT64));
			//}
			//else if (mtk->m_info.m_attribute == TRUE)
			//{
			//	// 找到卻唯讀的取消訂閱
			//	memcpy(chSendBuf+idx, &key, sizeof(HANDLE));
			//	idx += sizeof(HANDLE);
			//	
			//	UINT64 zero = 0;
			//	memcpy(chSendBuf+idx, &zero, sizeof(UINT64));
			//}
			//else
			//{
			//	// 找的到的更新offset
			//	memcpy(chSendBuf+idx, &key, sizeof(HANDLE));
			//	idx += sizeof(HANDLE);

			//	memcpy(chSendBuf+idx, &mtk->m_info.m_size, sizeof(UINT64));
			//	TRACE("同步: File:%s Size:%I64d Offset:%I64d\n", mtk->m_chFilename, (UINT64)mtk->m_info.m_size, (UINT64)mtk->m_overlapped.Offset);
			//}

			//idx += sizeof(UINT64);
			//total = idx;
			delete req;
		}

		//memcpy(chSendBuf, (char*)&total, sizeof(int));

		WSABUF SendDataBuf;
		//DataBuf.len = total+4;
		//DataBuf.buf = (char*)m_packet;
		SendDataBuf.len = dwSendLen;
		SendDataBuf.buf = chSendBuf;
		DWORD sendlen = 0;
		if (WSASend(m_socket, &SendDataBuf, 1, &sendlen, 0, &SendOverlapped, NULL) == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSA_IO_PENDING)
			{
				DWORD dw = WaitForSingleObject(SendOverlapped.hEvent, INFINITE);
				if (dw == WAIT_OBJECT_0)
					if (!WSAGetOverlappedResult(m_socket, &SendOverlapped, &sendlen, FALSE, &dFlag))
					{
						if (sendlen <= 0)
						{
							strLog.Format("奇怪的事情 沒寫入資料![%d]", sendlen);
							log->DFLogSend(strLog, DF_SEVERITY_WARNING);
						}
					}

				//WSAResetEvent(SendOverlapped.hEvent);
			}
			else
			{
				strLog.Format("WSASend失敗![%d]", WSAGetLastError());
				log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
				break;
			}
		}

		strLog.Format("送出同步!");
		log->DFLogSend(strLog, DF_SEVERITY_DEBUG);

		WSAResetEvent(SendOverlapped.hEvent);
		delete[] chSendBuf;

		dwHeartBeat = dwHB;
	}

	// 工作結束
	delete m_packet;
	m_packet = NULL;

	delete m_depacket;
	m_depacket = NULL;
	
//	delete head;
//	head = NULL;

	DeleteSocket();

	// 釋放全部的檔案
	UINT retkey = 0;
	MTK* retmtk = NULL;
	for (POSITION pos = m_mapFileList.GetStartPosition(); pos!=NULL; )
	{
		m_mapFileList.GetNextAssoc(pos, retkey, retmtk);

		if (retmtk == NULL)
			continue;

		if (retmtk->m_master == this)
			retmtk->m_master = NULL;
	}

	// 清空工作清單
	m_mapFileList.RemoveAll();

	ZSTD_freeDCtx(m_zstd);

	strLog.Format("連線中斷 執行續結束![Thread:%d]", m_nThreadID);
	log->DFLogSend(strLog, DF_SEVERITY_WARNING);

	if (theApp.m_bAutoReconnect && !IsStop())
	{
		bDisconnect = FALSE;
		strLog.Format("[自動重新連線]重新建立連線![Thread:%d]", m_nThreadID);
		log->DFLogSend(strLog, DF_SEVERITY_WARNING);
		closesocket(m_socket);
		goto START;
	}

	EndThread();
	return;
}

bool CDFNetMaster::Login()
{
	CDDFileClientDlg* dlg = (CDDFileClientDlg*)GetParentsHandle();
	CDFLogMaster* log = dlg->m_ptDFLog;

	char chLogin[256]={0};
	sprintf_s(chLogin, "LOGIN:%s:%s", theApp.m_chName, theApp.m_chKey);

	int n=0;
	if ((n = send(m_socket, chLogin, strlen(chLogin), 0)) < 0)
	{
		log->DFLogSend(CString("登入連線失敗!"), DF_SEVERITY_WARNING);
		return false;
	}

	return true;
}

bool CDFNetMaster::CreateSocket()
{
	CDDFileClientDlg* dlg = (CDDFileClientDlg*)GetParentsHandle();
	CDFLogMaster* log = dlg->m_ptDFLog;
	CString strLog("");

	DeleteSocket();

	// CREATE
	if ((m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		strLog.Format("[%s:%d]Socket Error(%d)", m_NetServerIP, m_NetServerPort, GetLastError());
		log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
		closesocket(m_socket);
		m_socket = NULL;
		return FALSE;
	}

	// CONNECT
	struct sockaddr_in server_addr;
	memset(&server_addr, NULL, sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(m_NetServerPort); // SET
	server_addr.sin_addr.s_addr = inet_addr(m_NetServerIP); // SET

	//struct sockaddr_in client_addr;
	//memset(&client_addr, NULL, sizeof(struct sockaddr_in));
	//client_addr.sin_family = AF_INET;
	//client_addr.sin_port = htons(m_NetLocalPort); // SET
	//client_addr.sin_addr.s_addr = htonl(INADDR_ANY); // SET

	//if (bind(m_socket, (SOCKADDR*)&client_addr, sizeof(struct sockaddr_in)) < 0)
	//{
	//	strLog.Format("Bind Error(%d)", GetLastError());
	//	log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
	//	closesocket(m_socket);
	//	return FALSE;
	//}

	if (connect(m_socket, (SOCKADDR*)&server_addr, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
	{
		strLog.Format("[%s:%d]Connect Error(%d)", m_NetServerIP, m_NetServerPort, GetLastError());
		log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
		closesocket(m_socket);
		m_socket = NULL;
		return FALSE;
	}

	// 改為不集合送出模式
	//BOOL bOptVal = TRUE;
	//int	bOptLen = sizeof(BOOL);
	//int err = setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, (char *) &bOptVal, bOptLen);
 //   if (err==SOCKET_ERROR) 
	//{
	//	strLog.Format("setsockopt錯誤![%d]", GetLastError());
	//	log->DFLogSend(strLog, DF_SEVERITY_HIGH);
 //       closesocket(m_socket);
	//	return FALSE;
 //   } 
	//else
 //   {
	//	strLog.Format("setsockopt(TCP_NODELAY)!");
	//	log->DFLogSend(strLog, DF_SEVERITY_INFORMATION);
	//}

	return TRUE;
}

void CDFNetMaster::DeleteSocket()
{
	if (m_socket)
		closesocket(m_socket);
	
	m_socket = NULL;
}

void CDFNetMaster::ForceStopMaster()
{
	DeleteSocket();
}
