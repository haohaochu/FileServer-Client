
// DFNet.cpp : 定義檔案類別。
//

#include "stdafx.h"
#include "DFNet.h"
#include "MtkFileServerDlg.h"
#include "MtkFileServer.h"

__declspec(thread) ZSTD_CCtx* m_zstd;
__declspec(thread) LPFN_TRANSMITPACKETS fnTransmitPackets;

// CDFNetSlave class implementation
// 
CDFNetSlave::CDFNetSlave(AFX_THREADPROC pfnThreadProc, LPVOID param, SOCKET sock, char* host, UINT port, char* name, char* key)
: CMyThread(pfnThreadProc, param)
, m_socket(sock)
, m_port(port)
, m_debug(FALSE)
, m_compress(TRUE)
, m_packet(NULL)
, m_age(0)
, m_lastsec(0)
, m_flow(0)
, m_maxflow(0)
, m_comvolumn(0)
, m_realvolumn(0)
{
	m_vecFilter.clear();
	m_mapWork.RemoveAll();

	memset(m_host, NULL, 20);
	memcpy(m_host, host, strlen(host));
	
	memset(m_name, NULL, 32);
	memcpy(m_name, name, strlen(name));

	memset(m_key, NULL, 32);
	memcpy(m_key, key, strlen(key));

	GetLocalTime(&m_currenttime);
	GetLocalTime(&m_starttime);
	InitializeSRWLock(&m_lock);
}

/*virtual */CDFNetSlave::~CDFNetSlave(void)
{
/*	for (POSITION pos = m_mapWork.GetStartPosition(); pos != NULL; )
	{
		UINT key = 0;
		WORK* work = NULL;
		m_mapWork.GetNextAssoc(pos, key, work);

		if (work)
		{
			InterlockedDecrement(&work->m_mtk->m_uRefCount);
			delete work;
		}
	}

	if (m_packet)
	{
		delete m_packet;
		m_packet = NULL;
	}

	m_mapWork.RemoveAll();*/
}

/*virtual */void CDFNetSlave::Go(void)
{
	CMtkFileServerDlg* dlg = (CMtkFileServerDlg*)GetParentsHandle();
	CDFLogMaster* log = dlg->m_ptDFLog;
	CString strLog("");

	// 更新工人名單+開始上工
	CString strKey("");
	strKey.Format("%s:%d", m_host, m_port);
	AcquireSRWLockExclusive(&dlg->m_ptDFNet->m_lockMaster);
	dlg->m_ptDFNet->m_mapSlaveList[strKey] = this;
	dlg->m_ptDFNet->m_bRefresh = TRUE;
	ReleaseSRWLockExclusive(&dlg->m_ptDFNet->m_lockMaster);

	//m_packet = new PACKET();
	//memset(m_packet, NULL, sizeof(PACKET));

	//m_sendbuf = new char[sizeof(PACKET)];
	//memset(m_sendbuf, NULL, sizeof(PACKET));

	//m_recvbuf = new char[20000];
	//memset(m_recvbuf, NULL, 20000);

	//m_zstd = ZSTD_createCCtx();

	//TRANSMIT_PACKETS_ELEMENT packet;
	//memset(&packet, NULL, sizeof(TRANSMIT_PACKETS_ELEMENT));
	//packet.dwElFlags = TP_ELEMENT_MEMORY;
	//packet.cLength = 0;//nOutLen+4;
	//packet.pBuffer = NULL;//(char*)m_sendbuf;
	//
	//OVERLAPPED ovl;
	//memset(&ovl, NULL, sizeof(OVERLAPPED));
	//ovl.hEvent = CreateEvent(NULL, true, false, NULL);

	//DWORD dwBytes = 0;
	//GUID guid = WSAID_TRANSMITPACKETS;
	//if (WSAIoctl(m_socket, SIO_GET_EXTENSION_FUNCTION_POINTER, (LPVOID)&guid, sizeof(guid), (LPVOID)&fnTransmitPackets, sizeof(fnTransmitPackets), &dwBytes, NULL, NULL) != 0)
	//{
	//	strLog.Format("取得函式指標失敗![%d]", WSAGetLastError());
	//	log->DFLogSend(strLog, DF_SEVERITY_HIGH);
	//	closesocket(m_socket);
	//	EndThread();
	//	return;
	//}

	// Initial WSA Buffer For Send/Recv
	WSABUF DataBuf;
	memset(&DataBuf, NULL, sizeof(WSABUF));

	WSABUF RecvDataBuf;
	memset(&RecvDataBuf, NULL, sizeof(WSABUF));

	// Initial WSA Overlapped For Send
	WSAEVENT SendEvent = WSACreateEvent();
	if (SendEvent == WSA_INVALID_EVENT)
	{
		strLog.Format("WSACreateEvent失敗(Send)![%d]", WSAGetLastError());
		log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
		closesocket(m_socket);
		
//		CString strKey("");
//		strKey.Format("%s:%d", m_host, m_port);
		AcquireSRWLockExclusive(&dlg->m_ptDFNet->m_lockMaster);
//		dlg->m_ptDFNet->m_mapSlaveList.RemoveKey(strKey);
		dlg->m_ptDFNet->m_bRefresh = TRUE;
		ReleaseSRWLockExclusive(&dlg->m_ptDFNet->m_lockMaster);
		
		EndThread();
		return;
	}

	WSAOVERLAPPED SendOverlapped;
	memset(&SendOverlapped, NULL, sizeof(WSAOVERLAPPED));
	SendOverlapped.hEvent = SendEvent;

	// Initial WSA Overlapped For Recv
	WSAEVENT RecvEvent = WSACreateEvent();
	if (RecvEvent == WSA_INVALID_EVENT)
	{
		strLog.Format("WSACreateEvent失敗(Recv)![%d]", WSAGetLastError());
		log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
		closesocket(m_socket);
		
//		CString strKey("");
//		strKey.Format("%s:%d", m_host, m_port);
		AcquireSRWLockExclusive(&dlg->m_ptDFNet->m_lockMaster);
//		dlg->m_ptDFNet->m_mapSlaveList.RemoveKey(strKey);
		dlg->m_ptDFNet->m_bRefresh = TRUE;
		ReleaseSRWLockExclusive(&dlg->m_ptDFNet->m_lockMaster);
		
		EndThread();
		return;
	}

	WSAOVERLAPPED RecvOverlapped;
	memset(&RecvOverlapped, NULL, sizeof(WSAOVERLAPPED));
	RecvOverlapped.hEvent = RecvEvent;

	// 取得工作條件
	if (!GetFileFilterFromDB())
	{
		strLog.Format("QueryDB連線有誤!踢掉[%s]", m_host);
		log->DFLogSend(strLog, DF_SEVERITY_HIGH);
		closesocket(m_socket);
		
//		CString strKey("");
//		strKey.Format("%s:%d", m_host, m_port);
		AcquireSRWLockExclusive(&dlg->m_ptDFNet->m_lockMaster);
//		dlg->m_ptDFNet->m_mapSlaveList.RemoveKey(strKey);
		dlg->m_ptDFNet->m_bRefresh = TRUE;
		ReleaseSRWLockExclusive(&dlg->m_ptDFNet->m_lockMaster);
		
		EndThread();
		return;
	}

	// 第一次取得工作內容
	GetFileHandleFromMaster();
	DWORD dwTimeout = GetTickCount();
//	if (m_mapWork.GetCount() == 0)
//	{
//		strLog.Format("QueryDB沒有檔案設定!踢掉[%s]", m_host);
//		log->DFLogSend(strLog, DF_SEVERITY_HIGH);
//		closesocket(m_socket);
//	
////		CString strKey("");
////		strKey.Format("%s:%d", m_host, m_port);
//		AcquireSRWLockExclusive(&dlg->m_ptDFNet->m_lockMaster);
//		dlg->m_ptDFNet->m_mapSlaveList.RemoveKey(strKey);
//		ReleaseSRWLockExclusive(&dlg->m_ptDFNet->m_lockMaster);
//		
//		EndThread();
//		return;
//	}

	// 改為不等待模式
	//ULONG mode = 1;
	//ioctlsocket(m_socket, FIONBIO, &mode);

	// 改為不集合送出模式
	BOOL bOptVal = TRUE;
	int	bOptLen = sizeof(BOOL);
	int err = setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, (char *) &bOptVal, bOptLen);
    if (err==SOCKET_ERROR) 
	{
		strLog.Format("setsockopt錯誤![%d]", GetLastError());
		log->DFLogSend(strLog, DF_SEVERITY_HIGH);
        closesocket(m_socket);

//		CString strKey("");
//		strKey.Format("%s:%d", m_host, m_port);
		AcquireSRWLockExclusive(&dlg->m_ptDFNet->m_lockMaster);
//		dlg->m_ptDFNet->m_mapSlaveList.RemoveKey(strKey);
		dlg->m_ptDFNet->m_bRefresh = TRUE;
		ReleaseSRWLockExclusive(&dlg->m_ptDFNet->m_lockMaster);
		
		EndThread();
		return;
    } 
	else
    {
		strLog.Format("setsockopt(TCP_NODELAY)!");
		log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
	}

	m_packet = new PACKET();
	memset(m_packet, NULL, sizeof(PACKET));

	m_sendbuf = new char[sizeof(PACKET)];
	memset(m_sendbuf, NULL, sizeof(PACKET));

	m_recvbuf = new char[20000];
	memset(m_recvbuf, NULL, 20000);

	m_zstd = ZSTD_createCCtx();

	// 開始工作
	BOOL bPacketControl = TRUE;
	BOOL bForceSync = TRUE;
	BOOL bResetRecv = TRUE;
	UINT64 tra = 0; 

	while (!IsStop())
	{
		DWORD tick = GetTickCount();

		// Timeout
		if ((tick-dwTimeout) > 100000) // 5分鐘
		{
			strLog.Format("[主機:%s]沒有任何回應!踢掉[%u:%u]", m_host, tick, dwTimeout);
			log->DFLogSend(strLog, DF_SEVERITY_HIGH);
			break;
		}

		// 整理工作
		UpdateFileHandleFromMaster();
		
		// 開始傳送
//		if (m_debug)
//		{
//			strLog.Format("[主機:%s]輪到我的回合!", m_host);
//			log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
//		}
		
		// 清除資料
		//memset(m_packet, NULL, sizeof(PACKET));
		m_packet->m_pktlen = 0;
		m_packet->m_pkt[0] = 0;
		UINT total = 0;

		DWORD remain = 0;
		UINT key = 0;
		WORK* work = NULL;

		for (POSITION pos = m_mapWork.GetStartPosition(); pos != NULL; )
		{
			if (total+sizeof(MTKINFO)+sizeof(DWORD) > MAX_PACKET_SIZE)
			{
				break; // 位置不夠囉
			}

			remain = MAX_PACKET_SIZE-total-sizeof(MTKINFO)-sizeof(DWORD); 
			tick = GetTickCount();
			key = 0;
			work = NULL;
			m_mapWork.GetNextAssoc(pos, key, work);
			
			if (work == NULL) // 已不提供的檔案
			{
				// todo
				strLog.Format("[主機:%s]奇怪的事情 不提供的檔案!", m_host);
				log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
				continue;
			}

			if (work->m_bIsRef == FALSE) // 被取消訂閱的檔案
			{
				if (work->m_mtk->m_info.m_attribute == TRUE) // 還是算有訂閱,只是不送資料,所以要檢查檔案結束了沒 (PS.因為如果直接從清單刪除,下次工作還是會發現這個檔案又加回來)
				{
					MTK* mtk = work->m_mtk;

					AcquireSRWLockExclusive(&m_lock);
					m_mapWork.RemoveKey(key);
					ReleaseSRWLockExclusive(&m_lock);

					InterlockedDecrement(&mtk->m_uRefCount);
					delete work;

					strLog.Format("傳輸完成(取消訂閱)![主機:%s][檔案:%s][剩餘訂閱:%d]", m_host, mtk->m_chFilename, mtk->m_uRefCount);
					log->DFLogSend(strLog, DF_SEVERITY_INFORMATION);
				}
			}
			else if (work->m_heartbeat == 0) // 全新的檔案
			{
				DWORD filenamelen = strlen(work->m_mtk->m_chFilename);
				if (remain < filenamelen) break; // 位置不夠囉

				*(HANDLE*)&m_packet->m_pkt[total] = work->m_mtk->m_info.m_handle;
				*(DWORD*)&m_packet->m_pkt[total+4] = 2;
				*(UINT64*)&m_packet->m_pkt[total+8] = *(UINT64*)&work->m_overlapped.Offset;
				*(FILETIME*)&m_packet->m_pkt[total+16] = work->m_mtk->m_info.m_lastwrite;
				*(UINT*)&m_packet->m_pkt[total+24] = filenamelen;
				memcpy(&m_packet->m_pkt[total+28], work->m_mtk->m_chFilename, filenamelen);
				
				total += (28+filenamelen);
				work->m_heartbeat = tick;
			}
			else if (*(UINT64*)&work->m_overlapped.Offset < work->m_mtk->m_info.m_size) // 還有沒傳的資料
			{
				DWORD retlen = 0;
				char *buf = &m_packet->m_pkt[total+28];

				UINT64 uCheckOffset = *(UINT64*)&work->m_overlapped.Offset;
				
				if (!ReadFile(work->m_mtk->m_info.m_handle, buf, remain, &retlen, &work->m_overlapped))
				{
					if (GetLastError() == ERROR_IO_PENDING)
					{
						DWORD dw = WaitForSingleObject(work->m_overlapped.hEvent, INFINITE);
						if(dw == WAIT_OBJECT_0)
						{
							if (GetOverlappedResult(work->m_mtk->m_info.m_handle, &work->m_overlapped, &retlen, TRUE) != 0)   
							{
								if (retlen <= 0) 
								{
									strLog.Format("奇怪的事情 沒讀到資料![%d][%s]", retlen, work->m_mtk->m_chFilename);
									log->DFLogSend(strLog, DF_SEVERITY_WARNING);
								}
							}
						}
						else
						{
							// ERROR
							strLog.Format("奇怪的讀檔回傳值![%d][%s]", dw, work->m_mtk->m_chFilename);
							log->DFLogSend(strLog, DF_SEVERITY_HIGH);
							continue;
						}
					}
					else
					{
						// ERROR
						strLog.Format("檔案讀取失敗![%d][%s]", GetLastError(), work->m_mtk->m_chFilename);
						log->DFLogSend(strLog, DF_SEVERITY_HIGH);
						continue;
					}
				}

				ResetEvent(work->m_overlapped.hEvent);

				// CheckReadFile
				if (retlen <= 0)
				{
					strLog.Format("Readfile異常!?![%s][%d]", work->m_mtk->m_chFilename, retlen);
					log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
				}

				// CheckOffset
				if (uCheckOffset != *(UINT64*)&work->m_overlapped.Offset)
				{
					strLog.Format("Offset值被改掉了!?![%s][BE:%d][AF:%d][RET:%d]", work->m_mtk->m_chFilename, uCheckOffset, work->m_overlapped.Offset, retlen);
					log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
				}

				*(HANDLE*)&m_packet->m_pkt[total] = work->m_mtk->m_info.m_handle;
				*(DWORD*)&m_packet->m_pkt[total+4] = 0;
				*(UINT64*)&m_packet->m_pkt[total+8] = *(UINT64*)&work->m_overlapped.Offset;
				*(FILETIME*)&m_packet->m_pkt[total+16] = work->m_mtk->m_info.m_lastwrite;
				*(UINT*)&m_packet->m_pkt[total+24] = retlen;
				
				total += (28+retlen);
				*(UINT64*)&work->m_overlapped.Offset += retlen;
				work->m_heartbeat = tick;
				
				// CheckOffset
				if (uCheckOffset+retlen != *(UINT64*)&work->m_overlapped.Offset)
				{
					strLog.Format("Offset值計算錯誤!?![%s][BE:%d][AF:%d][RET:%d]", work->m_mtk->m_chFilename, uCheckOffset, work->m_overlapped.Offset, retlen);
					log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
				}
			}
			else // 不需要傳資料
			{
				BOOL bIsReadonly = work->m_mtk->m_info.m_attribute;
				BOOL bIsDone = (*(UINT64*)&work->m_overlapped.Offset == work->m_mtk->m_info.m_size);
				if ((bIsReadonly && bIsDone) || (tick - work->m_heartbeat >= 10000)) // 超過10秒"或"屬性ReadOnly要送HB
				{
					*(HANDLE*)&m_packet->m_pkt[total] = work->m_mtk->m_info.m_handle;
					*(DWORD*)&m_packet->m_pkt[total+4] = /*work->m_mtk->m_info.m_attribute*/(bIsReadonly && bIsDone);
					*(UINT64*)&m_packet->m_pkt[total+8] = *(UINT64*)&work->m_overlapped.Offset;
					*(FILETIME*)&m_packet->m_pkt[total+16] = work->m_mtk->m_info.m_lastwrite;
					*(UINT*)&m_packet->m_pkt[total+24] = 0;

					total += 28;
					work->m_heartbeat = tick;
				}

				if (bIsReadonly && bIsDone) // 唯讀檔案+傳送完畢=該退場了
				{
					MTK* mtk = work->m_mtk;

					AcquireSRWLockExclusive(&m_lock);
					m_mapWork.RemoveKey(key);
					ReleaseSRWLockExclusive(&m_lock);

					InterlockedDecrement(&mtk->m_uRefCount);
					delete work;
					
					strLog.Format("傳輸完成(心跳結尾)![主機:%s][檔案:%s][剩餘訂閱:%d]", m_host, mtk->m_chFilename, mtk->m_uRefCount);
					log->DFLogSend(strLog, DF_SEVERITY_INFORMATION);
				}
			}
		}

		//if (bPacketControl && total<10000)
		//{
		//	Sleep(100);
		//	bPacketControl = FALSE;
		//	goto PACK;
		//}

		DWORD sendlen = 0;
		//memset(m_sendbuf, NULL, sizeof(PACKET));
		m_sendbuf[0] = 0;
		if (total > 0) // 有資料要傳或有HB要寫=繼續往前
		{
			m_packet->m_pktlen = total;

			if ((m_packet->m_pktlen>256) && m_compress && m_zstd && theApp.m_ptDict) // 壓縮
			{
				UINT nOutLen = 0;
				if (!ZSTD_isError(nOutLen=(int)ZSTD_compress_usingCDict(m_zstd, (char*)m_sendbuf+sizeof(int), MAX_PACKET_SIZE, (const char*)m_packet, m_packet->m_pktlen+sizeof(int), theApp.m_ptDict)))
				{	
					// 壓縮成功
					*(int*)m_sendbuf = nOutLen | ZSTD_COMPRESS;	// len(OR)壓縮旗標

					//memset(&DataBuf, NULL, sizeof(WSABUF));
					//DataBuf.len = 0;
					//DataBuf.buf = NULL;
					DataBuf.len = nOutLen+4;
					DataBuf.buf = (char*)m_sendbuf;
				}
				else
				{
					// 壓縮失敗
					strLog.Format("奇怪的事情 壓縮失敗![com][%s][%d][%d]", ZSTD_getErrorName(nOutLen), nOutLen, total);
					log->DFLogSend(strLog, DF_SEVERITY_DEBUG);

					//memset(&DataBuf, NULL, sizeof(WSABUF));
					//DataBuf.len = 0;
					//DataBuf.buf = NULL;
					DataBuf.len = m_packet->m_pktlen+4;
					DataBuf.buf = (char*)m_packet;
				}

				DWORD dFlag = 0;
				if (WSASend(m_socket, &DataBuf, 1, &sendlen, 0, &SendOverlapped, NULL) == SOCKET_ERROR)
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
					}
					else
					{
						strLog.Format("WSASend失敗![%d]", WSAGetLastError());
						log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
						break;
					}
				}

				WSAResetEvent(SendOverlapped.hEvent);

				if (DataBuf.len != sendlen)
				{
					strLog.Format("奇怪的事情 送出的大小不一樣![com][%d][%d]", nOutLen+4, sendlen);
					log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
				}
				//}
				//else 
				//{
				//	// 壓縮失敗=>那就不壓縮
				//	strLog.Format("奇怪的事情 壓縮失敗![com][%s][%d][%d]", ZSTD_getErrorName(nOutLen), nOutLen, total);
				//	log->DFLogSend(strLog, DF_SEVERITY_DEBUG);

				//	memset(&DataBuf, NULL, sizeof(WSABUF));
				//	DataBuf.len = m_packet->m_pktlen+4;
				//	DataBuf.buf = m_packet->m_pkt;

				//	DWORD dFlag = 0;
				//}
			}
			else // 不壓縮
			{
				//memcpy(m_sendbuf, m_packet, total+4);

				//memset(&DataBuf, NULL, sizeof(WSABUF));
				//DataBuf.len = total+4;
				//DataBuf.buf = (char*)m_sendbuf;
				//memset(&DataBuf, NULL, sizeof(WSABUF));
				//DataBuf.len = 0;
				//DataBuf.buf = NULL;
				DataBuf.len = m_packet->m_pktlen+4;
				DataBuf.buf = (char*)m_packet;

				DWORD dFlag = 0;
				DWORD n = 0;
				if (WSASend(m_socket, &DataBuf, 1, &sendlen, 0, &SendOverlapped, NULL) == SOCKET_ERROR)
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

				WSAResetEvent(SendOverlapped.hEvent);

				//TRACE("[%I64d]%d\n", tra++, sendlen);

				if (DataBuf.len != sendlen)
				{
					strLog.Format("奇怪的事情 送出的大小不一樣![%d][%d]", total+4, sendlen);
					log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
				}

				//TRANSMIT_PACKETS_ELEMENT packet;
				//memset(&packet, NULL, sizeof(TRANSMIT_PACKETS_ELEMENT));
				//packet.dwElFlags = TP_ELEMENT_MEMORY;
				//packet.cLength = total+4;
				//packet.pBuffer = (char*)m_packet;
				
				//OVERLAPPED ovl;
				//memset(&ovl, NULL, sizeof(OVERLAPPED));
				//ovl.hEvent = CreateEvent(NULL, true, false, NULL);
				//ResetEvent(ovl.hEvent);

				//if (!fnTransmitPackets(m_socket, &packet, 1, packet.cLength, &ovl, 0))
				//{
				//	if (WSAGetLastError() == WSA_IO_PENDING)
				//	{
				//		DWORD dw = WaitForSingleObject(ovl.hEvent, INFINITE);
				//		if (dw == WAIT_OBJECT_0)
				//			if (GetOverlappedResult((HANDLE)m_socket, &ovl, &ret, TRUE) != 0)
				//			{
				//				if (ret <= 0)
				//				{
				//					strLog.Format("奇怪的事情 沒寫出資料![%d]", ret);
				//					log->DFLogSend(strLog, DF_SEVERITY_WARNING);
				//				}
				//			}
				//	}
				//	else
				//	{
				//		// ERROR
				//		strLog.Format("封包傳送失敗![%d]", GetLastError());
				//		log->DFLogSend(strLog, DF_SEVERITY_HIGH);
				//		continue;
				//	}
				//}

				//if ((n = send(m_socket, (char*)m_packet, total+4, 0)) <= 0)
				//{
				//	// Error
				//	strLog.Format("連線檔案傳送失敗![%d][%s]", m_packet->m_pktlen, m_packet->m_pkt);
				//	log->DFLogSend(strLog, DF_SEVERITY_HIGH);
				//	closesocket(m_socket);
				//	break;
				//}
			}

			//// Packet Control
			//if (sendlen <= 10000) // 10K
			//{
			//	//strLog.Format("[主機:%s]封包調節(1)!", m_host);
			//	//log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
			//	Sleep(200);
			//}
			//else if (sendlen <= 20000) // 20K
			//{
			//	//strLog.Format("[主機:%s]封包調節(2)!", m_host);
			//	//log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
			//	Sleep(200);
			//}
			//else if (sendlen <= 30000) // 30K
			//{
			//	//strLog.Format("[主機:%s]封包調節(3)!", m_host);
			//	//log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
			//	Sleep(150);
			//}
			//else if (sendlen <= 40000) // 40K
			//{
			//	//strLog.Format("[主機:%s]封包調節(4)!", m_host);
			//	//log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
			//	Sleep(100);
			//}

			// Check WSASend
			if (sendlen == 0)
			{
				strLog.Format("[主機:%s]有資料要傳送卻送出0byte!", m_host);
				log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
			}

			// 更新連線資訊(For UI)
			AcquireSRWLockExclusive(&m_lock);

			// 每秒重置流量計算
			if ((GetTickCount()-m_lastsec) >= 1000)
			{	
				m_lastsec = GetTickCount();
				m_flow = 0;
			}

			m_flow += total;
			if (m_flow > m_maxflow) m_maxflow = m_flow;
			GetLocalTime(&m_currenttime);

			m_comvolumn = sendlen;
			m_realvolumn = total;

			ReleaseSRWLockExclusive(&m_lock); 

			// Timeout重置
			dwTimeout = tick;
//			strLog.Format("[主機:%s]Timeout重置[有資料送出][dwTimeout:%d]", m_host, dwTimeout);
//			log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
		}
		else // 沒資料可傳+沒HB可寫=處罰100ms
		{
			Sleep(100);
			if (m_debug)
			{
				strLog.Format("[主機:%s]沒有任何資料要傳送![Sleep:100ms]", m_host);
				log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
			}
//			continue;
		}

		if (m_debug)
		{
			strLog.Format("[主機:%s]結束這回合!", m_host);
			log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
		}

		bPacketControl = TRUE;

		// 同步資訊
		DWORD recvlen = 0;
		DWORD recvFlag = 0;
		
		if (m_debug)
		{
			strLog.Format("與[Host:%s]1.等待接收同步資訊!", m_host);
			log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
		}
			
		if (bResetRecv)
		{
			memset(m_recvbuf, NULL, 20000);
			memset(&RecvDataBuf, NULL, sizeof(WSABUF));

			RecvDataBuf.len = 20000;
			RecvDataBuf.buf = m_recvbuf;
			
			recvlen = 0;
			recvFlag = 0;
			WSARecv(m_socket, &RecvDataBuf, 1, &recvlen, &recvFlag, &RecvOverlapped, NULL);
			bResetRecv = FALSE;
		}

		DWORD dw = WaitForSingleObject(RecvOverlapped.hEvent, bForceSync ? INFINITE : 0);
		if (dw == WAIT_OBJECT_0)
		{
			if (WSAGetOverlappedResult(m_socket, &RecvOverlapped, &recvlen, FALSE, &recvFlag))
			{
				if (recvlen == 0)
					continue;

				if ((recvlen % 16) != 0)
				{
					strLog.Format("奇怪的事情 資料長度有誤![%d]", recvlen);
					log->DFLogSend(strLog, DF_SEVERITY_WARNING);
					break;
				}
			}

			bForceSync = FALSE;
			bResetRecv = TRUE;
			WSAResetEvent(RecvOverlapped.hEvent);

			// Timeout重置
			dwTimeout = tick;
//			strLog.Format("[主機:%s]Timeout重置[收到同步資料][dwTimeout:%d]", m_host, dwTimeout);
//			log->DFLogSend(strLog, DF_SEVERITY_HIGH);
			//strLog.Format("Timeout重置![1][%s][%u]", m_host, dwTimeout);
			//log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
		}
		else if (dw == WAIT_TIMEOUT)
		{
			// TIMEOUT 沒事的,沒有檔案需要同步
			continue;
		}
		else
		{
			// FAIL 結束
			strLog.Format("奇怪的事情 不知道回傳甚麼![%d]", dw);
			log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
			break;
		}

		//TRACE("同步\n");

		DWORD synclen = 0;
		//DWORD syncdatalen = sizeof(HANDLE)+sizeof(UINT64);
		DWORD syncdatalen = 16;

		while (synclen+syncdatalen <= recvlen)
		{
			UINT recvid = 0;
			UINT recvhandle = 0;
			UINT64 recvoffset = 0;
					
			if (m_debug)
			{
				strLog.Format("與[Host:%s]2.處理同步資訊長度![len:%d][已處理:%d]", m_host, recvlen, synclen);
				log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
			}

			recvid = *(UINT*)(m_recvbuf+synclen);
			recvhandle = *(UINT*)(m_recvbuf+synclen+4);
			recvoffset = *(UINT64*)(m_recvbuf+synclen+8);

			//memcpy(&recvhandle, (char*)m_recvbuf+synclen, sizeof(UINT));
			//memcpy(&recvoffset, (char*)m_recvbuf+synclen+sizeof(UINT), sizeof(UINT64));
				
			//TRACE("同步:%x:%I64d\n", recvhandle, recvoffset);

			if (m_debug)
			{
				strLog.Format("與[Host:%s]3.處理同步資訊資料![handle:%d][offset:%I64d]", m_host, recvhandle, recvoffset);
				log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
			}

			if (recvhandle == 0)
			{
				synclen += syncdatalen;
				continue;
			}

			WORK* work = NULL;
			if (!m_mapWork.Lookup(recvhandle, (WORK*&)work))
			{
				// 找不到就算了
				strLog.Format("奇怪的事情 [主機:%s]傳來不認識的檔案代號!", m_host);
				log->DFLogSend(strLog, DF_SEVERITY_WARNING);

				synclen += syncdatalen;
				continue;
			}

			switch (recvid)
			{
			case 0: // 取消訂閱
				{
					if (m_debug)
					{
						strLog.Format("[主機:%s]取消訂閱檔案[%s]!", m_host, work->m_mtk->m_chFilename);
						log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
					}
					
					work->m_bIsRef = FALSE;	
					
					break;
				}
			case 1: // 同步檔案
				{
					memcpy(&work->m_overlapped.Offset, &recvoffset, sizeof(UINT64));

					if (recvoffset > work->m_mtk->m_info.m_size)
					{
						strLog.Format("奇怪的事情 [主機:%s]檔案大於主機端![%s][%I64d][%I64d]", m_host, work->m_mtk->m_chFilename, recvoffset, work->m_mtk->m_info.m_size);
						log->DFLogSend(strLog, DF_SEVERITY_WARNING);
					}
					
					break;
				}
			case 2: // 全部重來
				{
					work->m_heartbeat = 0;

					if (m_debug)
					{
						strLog.Format("[主機:%s]重新傳輸檔案[%s]!", m_host, work->m_mtk->m_chFilename);
						log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
					}

					break;
				}
			case 3: // 
				{
					break;
				}
			default:
				break;
			}
			//if (recvhandle == 0)
			//{
			//	// 零
			//	synclen += syncdatalen;
			//	continue;
			//}

			//WORK* work = NULL;
			//if (!m_mapWork.Lookup(recvhandle, (WORK*&)work))
			//{
			//	// 找不到就算了
			//	strLog.Format("奇怪的事情 [主機:%s]傳來不認識的檔案代號!", m_host);
			//	log->DFLogSend(strLog, DF_SEVERITY_WARNING);

			//	synclen += syncdatalen;
			//	continue;
			//}
			//				
			//if (recvoffset == 0)
			//{
			//	if (m_debug)
			//	{
			//		strLog.Format("[主機:%s]取消訂閱檔案[%s]!", m_host, work->m_mtk->m_chFilename);
			//		log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
			//	}

			//	work->m_ref = FALSE;
			//}
			//else
			//{
			//	memcpy(&work->m_overlapped.Offset, &recvoffset, sizeof(UINT64));

			//	if (recvoffset > work->m_mtk->m_info.m_size)
			//	{
			//		strLog.Format("奇怪的事情 [主機:%s]檔案大於主機端![%I64d][%I64d]", m_host, recvoffset, work->m_mtk->m_info.m_size);
			//		log->DFLogSend(strLog, DF_SEVERITY_WARNING);
			//	}

			//	//TRACE("同步: File:%s Size:%I64d Offset:%I64d\n", work->m_mtk->m_chFilename, (UINT64)work->m_mtk->m_info.m_size, (UINT64)work->m_overlapped.Offset);
			//}

			synclen += syncdatalen;
			continue;
		}
	}

//	CString strKey("");
//	strKey.Format("%s:%d", m_host, m_port);
	AcquireSRWLockExclusive(&dlg->m_ptDFNet->m_lockMaster);
//	dlg->m_ptDFNet->m_mapSlaveList.RemoveKey(strKey);
	dlg->m_ptDFNet->m_bRefresh = TRUE;
	ReleaseSRWLockExclusive(&dlg->m_ptDFNet->m_lockMaster);

	closesocket(m_socket);

	// 釋放所有的工作
	for (POSITION pos = m_mapWork.GetStartPosition(); pos != NULL; )
	{
		UINT key = 0;
		WORK* work = NULL;
		m_mapWork.GetNextAssoc(pos, key, work);

		if (work)
		{
			InterlockedDecrement(&work->m_mtk->m_uRefCount);
			delete work;
		}
	}

	if (m_packet)
	{
		delete m_packet;
		m_packet = NULL;
	}

	if (m_sendbuf)
	{
		delete m_sendbuf;
		m_sendbuf = NULL;
	}

	if (m_recvbuf)
	{
		delete m_recvbuf;
		m_recvbuf = NULL;
	}

//	if (head)
//	{
//		delete head;
//		head = NULL;
//	}

	m_mapWork.RemoveAll();
	ZSTD_freeCCtx(m_zstd);

	strLog.Format("連線中斷 執行序結束![Thread:%d]", m_nThreadID);
	log->DFLogSend(strLog, DF_SEVERITY_WARNING);

	EndThread();
	return;
}

bool CDFNetSlave::ApplyFilter(CString& fname)
{
	for (UINT i=0; i < m_vecFilter.size(); i++)
	{
		CString filter(m_vecFilter[i]);

		if (filter == "*")
			return TRUE;

		filter.Replace("*", "");
		if (fname.Find(filter) >= 0)
			return TRUE;
	}

	return FALSE;
}

bool CDFNetSlave::GetFileFilterFromDB()
{
	CMtkFileServerDlg* dlg = (CMtkFileServerDlg*)GetParentsHandle();
	CDFLogMaster* log = dlg->m_ptDFLog;
	CString strLog("");

	SOCKET sock;

	// CREATE
	if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		strLog.Format("Socket Error(Invalid Socket)[%s]", "");
		log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
		m_socket = NULL;
		return FALSE;
	}
	
	// CONNECT
	struct sockaddr_in server_addr;
	
	memset(&server_addr, NULL, sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(theApp.m_uServerPort);
	server_addr.sin_addr.s_addr = inet_addr(theApp.m_chServerIP);

	if (connect(sock, (SOCKADDR*)&server_addr, sizeof(struct sockaddr_in)))
	{
		strLog.Format("Connect Error(%d)[%s]", GetLastError(), "");
		log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
		closesocket(sock);
		return FALSE;
	}

	// SEND
	int n = 0;
	char cmd[250] = {0};
	char buf[5000] = {0};
	
	//sprintf_s(cmd, "GET /SpXFileDir?ip=%s HTTP/1.1\r\nConnection: keep-alive\r\n\r\n", m_host);
	//sprintf_s(cmd, "GET /SpXFileDir2?listen=%d&ip=%s:%s HTTP/1.1\r\nConnection: keep-alive\r\n\r\n", theApp.m_uListenPort, m_host, m_key);
	sprintf_s(cmd, "GET /SpXFileDir?listen=%d&ip=%s:%s HTTP/1.1\r\nConnection: keep-alive\r\n\r\n", theApp.m_uListenPort, m_host, m_key);
	
	if ((n = send(sock, cmd, strlen(cmd), 0)) <= 0)
	{
		strLog.Format("Send Error(%d)[%s]", GetLastError(), "");
		log->DFLogSend(strLog, DF_SEVERITY_INFORMATION);
		closesocket(sock);
		return FALSE;
	}

	// RECV
	char* find = NULL;
	char* content = NULL;
	int contentlen = -1;

	UINT recvi = 0;
	UINT readi = 0;
	while ((n = recv(sock, buf+recvi, 5000-recvi, 0)) > 0)
	{
		recvi += n;

		if ((find = strstr(buf+readi, "Content-Length: ")) > 0)
		{
			contentlen = atoi(find+14+2);
//			strLog.Format("contentlen:%d", contentlen);
//			log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
			//content = buf+strlen(buf)-contentlen;
		}

		if ((find = strstr(buf+readi, "\r\n\r\n")) > 0) // 內文開頭
		{
			content = find+4;
//			strLog.Format("content:%s", content);
//			log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
		}

		if (contentlen >= 0 && strlen(content) < contentlen)
		{
			readi += n;
			continue;
		}
		else
		{
			break;
		}
	}

	//if ((n = recv(sock, buf, 5000, 0)) <= 0)
	//{
	//	strLog.Format("Recv Error(%d)[%s]", GetLastError(), "");
	//	log->DFLogSend(strLog, DF_SEVERITY_INFORMATION);
	//	closesocket(sock);
	//	return FALSE;
	//}

	// GET DATA
	//char* find = NULL;
	//char* content = NULL;
	//UINT contentlen = 0;
	//if ((find = strstr(buf, "Content-Length: ")) > 0)
	//{
	//	contentlen = atoi(find+14+2);
	//	strLog.Format("contentlen:%d", contentlen);
	//	log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
	//	//content = buf+strlen(buf)-contentlen;
	//}

	//if ((find = strstr(buf, "\r\n\r\n")) > 0) // 內文開頭
	//{
	//	content = find+2;
	//	strLog.Format("content:%s", content);
	//	log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
	//}

	///// check
	//strLog.Format("len:%d", strlen(buf));
	//log->DFLogSend(strLog, DF_SEVERITY_DISASTER);

	//strLog.Format("contentlen:%d content:%s", contentlen, content);
	//log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
	
	// PARSE DATA
	if (contentlen > 0)
	{
		while ((find = strstr(content, "\n")) > 0)
		{
			CString strFilter(content, find-content);
			if (strFilter == "NoZlib")
				m_compress = FALSE;
			else
				m_vecFilter.push_back(CString(content, find-content));

			content = find + 1;
		}

		CString strFilter(content);
		if (strFilter == "NoZlib")
			m_compress = FALSE;
		else
		{
			m_vecFilter.push_back(CString(content));
//			log->DFLogSend(CString(content), DF_SEVERITY_DISASTER);
		}
	}
	closesocket(sock);
	return TRUE;
}

void CDFNetSlave::GetFileHandleFromMaster()
{
	CMtkFileServerDlg* dlg = (CMtkFileServerDlg*)GetParentsHandle();
	
	AcquireSRWLockExclusive(&dlg->m_ptFileThread->m_lockFileList);

	CArray<MTK*, MTK*&>* ary = &dlg->m_ptFileThread->m_arrMtkFile;
	for (int i=0; i<ary->GetCount(); i++)
	{
		MTK* mtk = ary->GetAt(i);
		if ((mtk->m_info.m_attribute == FALSE) && ApplyFilter(CString(mtk->m_chFilename)))
		{
			WORK* work = new WORK();
			memset(work, NULL, sizeof(WORK));
			
			work->m_mtk = mtk;
			work->m_heartbeat = 0;
			work->m_uAge = dlg->m_ptFileThread->m_uAge;
			work->m_bIsRef = TRUE;
			work->m_overlapped.Offset = 0;
			work->m_overlapped.OffsetHigh = 0;
			work->m_overlapped.hEvent = CreateEvent(NULL, true, false, NULL);
			if (work->m_overlapped.hEvent == NULL)
			{
				delete work;
				continue;	
			}

			AcquireSRWLockExclusive(&m_lock);
			m_mapWork[(UINT&)work->m_mtk->m_info.m_handle] = work;
			ReleaseSRWLockExclusive(&m_lock);

			InterlockedIncrement(&mtk->m_uRefCount);
		}
	}

	m_age = dlg->m_ptFileThread->m_uAge;

	ReleaseSRWLockExclusive(&dlg->m_ptFileThread->m_lockFileList);
}

void CDFNetSlave::UpdateFileHandleFromMaster()
{
	CMtkFileServerDlg* dlg = (CMtkFileServerDlg*)GetParentsHandle();

	if (!dlg->m_ptFileThread)
		return;

	AcquireSRWLockExclusive(&dlg->m_ptFileThread->m_lockFileList);

	if (m_age == dlg->m_ptFileThread->m_uGrowAge)
	{
		ReleaseSRWLockExclusive(&dlg->m_ptFileThread->m_lockFileList);
		return;
	}
	else
	{
		CArray<MTK*, MTK*&>* ary = &dlg->m_ptFileThread->m_arrMtkFile;
		for (int i=0; i<ary->GetCount(); i++)
		{
			MTK* mtk = ary->GetAt(i);
			if ((mtk->m_info.m_attribute == FALSE) && ApplyFilter(CString(mtk->m_chFilename)))
			{
				WORK* work = NULL;
				if (!m_mapWork.Lookup((UINT&)mtk->m_info.m_handle, work))
				{
					work = new WORK();
					memset(work, NULL, sizeof(WORK));

					work->m_mtk = mtk;
					work->m_heartbeat = 0;
					work->m_uAge = dlg->m_ptFileThread->m_uAge;
					work->m_bIsRef = TRUE;
					work->m_overlapped.Offset = 0;
					work->m_overlapped.OffsetHigh = 0;
					work->m_overlapped.hEvent = CreateEvent(NULL, true, false, NULL);
					if (work->m_overlapped.hEvent == NULL)
					{
						delete work;
						continue;	
					}

					AcquireSRWLockExclusive(&m_lock);
					m_mapWork[(UINT&)work->m_mtk->m_info.m_handle] = work;
					ReleaseSRWLockExclusive(&m_lock);

					InterlockedIncrement(&mtk->m_uRefCount);
				}
				else
				{
					work->m_uAge = dlg->m_ptFileThread->m_uAge;
				}
			}
		}

		m_age = dlg->m_ptFileThread->m_uGrowAge;
	}

	ReleaseSRWLockExclusive(&dlg->m_ptFileThread->m_lockFileList);
}

/*public */void CDFNetSlave::ForceStopSlave()
{
	closesocket(m_socket);
	StopThread();
}

/*public */BOOL CDFNetSlave::IsStopSlave()
{
	return IsStop();
}

// CDFNetMaster class implementation
// 
CDFNetMaster::CDFNetMaster(AFX_THREADPROC pfnThreadProc, LPVOID param, UINT port)
: CMyThread(pfnThreadProc, param)
, m_socket(NULL)
, m_port(port)
{
	m_mapSlaveList.RemoveAll();
	m_bRefresh = TRUE;
	InitializeSRWLock(&m_lockMaster);
}

/*virtual */CDFNetMaster::~CDFNetMaster(void)
{

}

/*virtual */void CDFNetMaster::Go(void)
{
	CMtkFileServerDlg* dlg = (CMtkFileServerDlg*)GetParentsHandle();
	CDFLogMaster* log = dlg->m_ptDFLog;
	CString strLog("");

	CreateSocket();
	ReleaseSRWLockExclusive(&dlg->m_lockServer);

	while (!IsStop())
	{
		if (m_socket == NULL)
			CreateSocket();
		
		SOCKET socket = NULL;
		struct sockaddr_in client_addr;
		int client_len = sizeof(struct sockaddr_in);
		memset(&client_addr, NULL, client_len);
		
		// 接收一個新工人
		if ((socket = accept(m_socket, (SOCKADDR*)&client_addr, &client_len)) == INVALID_SOCKET)
		{
			int err = GetLastError();
			if (err == 10004)
			{
				strLog.Format("接收連線端口已關閉(10004)");
				log->DFLogSend(strLog, DF_SEVERITY_INFORMATION);
				continue;
			}

			strLog.Format("接收連線失敗(%d)", GetLastError());
			log->DFLogSend(strLog, DF_SEVERITY_HIGH);
			continue;
		}

		// 接收登入資訊
		int n=0;
		char chLogin[100]={0};
		if ((n = recv(socket, chLogin, 100, 0)) < 0)
		{
			strLog.Format("接收登入失敗(%d)", GetLastError());
			log->DFLogSend(strLog, DF_SEVERITY_HIGH);
			closesocket(socket);
			continue;
		}

		// 處理登入資訊
		if (memcmp(chLogin, "LOGIN:", 6) != 0)
		{
			strLog.Format("登入資訊格式有誤[%s]", chLogin);
			log->DFLogSend(strLog, DF_SEVERITY_HIGH);
			closesocket(socket);
			continue;
		}

		char* chLoginName = &chLogin[6];
		char* find = strstr(&chLogin[6], ":");
		char* chLoginKey = find+1;
		find[0] = 0;

		//struct sockaddr_in client;
		//memset(&client, NULL, sizeof(struct sockaddr_in));
		//int len = sizeof(struct sockaddr_in);
		
		// 更新工人名單+開始上工
		CString strKey("");
		strKey.Format("%s:%d", inet_ntoa(client_addr.sin_addr), htons(client_addr.sin_port));
//		AcquireSRWLockExclusive(&m_lockMaster);
//		m_mapSlaveList[strKey] = slave;
//		ReleaseSRWLockExclusive(&m_lockMaster);
		
		if (m_mapSlaveList[strKey] != NULL)
		{
			strLog.Format("存在重複的連線[%s]", strKey);
			log->DFLogSend(strLog, DF_SEVERITY_HIGH);
			closesocket(socket);
			continue;
		}

		try
		{
			CDFNetSlave* slave = new CDFNetSlave(CDFNetSlave::ThreadFunc, dlg, socket, inet_ntoa(client_addr.sin_addr), htons(client_addr.sin_port), chLoginName, chLoginKey);
			if (slave == NULL)
			{
				strLog.Format("記憶體不足!無法開啟新的連線![%s]", strKey);
				log->DFLogSend(strLog, DF_SEVERITY_HIGH);
				closesocket(socket);

				delete slave;
				slave = NULL;
				continue;
			}
		
			VERIFY(slave->CreateThread());
			//if (!slave->CreateThread())
			//{
			//	strLog.Format("記憶體不足!無法開啟新的執行序![%s]", strKey);
			//	log->DFLogSend(strLog, DF_SEVERITY_HIGH);
			//	closesocket(socket);
			//
			//	delete slave;
			//	slave = NULL;
			//	continue;
			//}
		}
		catch (...)
		{
			strLog.Format("[EXCEPTION]記憶體不足!無法執行新的執行序![%s]", strKey);
			log->DFLogSend(strLog, DF_SEVERITY_HIGH);
			closesocket(socket);
			continue;
		}

		// 紀錄
		strLog.Format("新的工人!!!(%s)", inet_ntoa(client_addr.sin_addr));
		log->DFLogSend(strLog, DF_SEVERITY_INFORMATION);
	}

	DeleteSocket();

	//AcquireSRWLockExclusive(&m_lockMaster);
	//for (POSITION pos=m_mapSlaveList.GetStartPosition(); pos!=NULL; )
	//{
	//	CString key;
	//	CDFNetSlave* slave = NULL;

	//	m_mapSlaveList.GetNextAssoc(pos, key, (void*&)slave);
	//	if (slave)
	//	{
	//		UINT tid = slave->m_nThreadID;
	//		slave->StopThread();
	//		WaitForSingleObject(slave->m_hThread, INFINITE);
	//		strLog.Format("[程式結束]關閉使用者(Thread:%d)", tid);
	//		log->DFLogSend(strLog, DF_SEVERITY_INFORMATION);
	//		delete slave;
	//		slave = NULL;
	//		m_mapSlaveList.RemoveKey(key);
	//	}
	//}
	//ReleaseSRWLockExclusive(&m_lockMaster);

	EndThread();
	return;
}

bool CDFNetMaster::CreateSocket()
{
	CMtkFileServerDlg* dlg = (CMtkFileServerDlg*)GetParentsHandle();
	CDFLogMaster* log = dlg->m_ptDFLog;
	CString strLog("");

	DeleteSocket();

	// CREATE
	if ((m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		log->DFLogSend(CString("Socket Error(Invalid Socket)"), DF_SEVERITY_DISASTER);
		m_socket = NULL;
		return FALSE;
	}

	// BIND
	struct sockaddr_in server_addr;
	
	memset(&server_addr, NULL, sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(m_port); // SET
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(m_socket, (SOCKADDR*)&server_addr, sizeof(struct sockaddr_in)) < 0)
	{
		strLog.Format("Bind Error (%d)", GetLastError());
		log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
		DeleteSocket();
		m_socket = NULL;
		return FALSE;
	}

	// LISTEN
	if (listen(m_socket, 5) < 0)
	{
		strLog.Format("Listen Error (%d)", GetLastError());
		log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
		DeleteSocket();
		m_socket = NULL;
		return FALSE;
	}

	return TRUE;
}

void CDFNetMaster::DeleteSocket()
{
	if (m_socket)
		closesocket(m_socket);
	
	m_socket = NULL;
}

/*public */void CDFNetMaster::ForceStopMaster()
{
	CMtkFileServerDlg* dlg = (CMtkFileServerDlg*)GetParentsHandle();
	CDFLogMaster* log = dlg->m_ptDFLog;
	CString strLog("");

	StopThread();
	DeleteSocket();
	
	AcquireSRWLockExclusive(&m_lockMaster);
	
	CString key("");
	CDFNetSlave* slave = NULL;
	for (POSITION pos=m_mapSlaveList.GetStartPosition(); pos!=NULL; )
	{
		m_mapSlaveList.GetNextAssoc(pos, key, (void*&)slave);

		if (slave)
		{
			slave->ForceStopSlave();
			slave->StopThread();
			WaitForSingleObject(slave->m_hThread, INFINITE);

			strLog.Format("連線中斷![Host:%s]", slave->m_host);
			log->DFLogSend(strLog, DF_SEVERITY_WARNING);

			m_mapSlaveList.RemoveKey(key);
			delete slave;
			slave = NULL;
		}
	}
	m_mapSlaveList.RemoveAll();
		
	ReleaseSRWLockExclusive(&m_lockMaster);
}

