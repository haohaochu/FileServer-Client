
// DFNet.cpp : �w�q�ɮ����O�C
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

	// ��s�u�H�W��+�}�l�W�u
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
	//	strLog.Format("���o�禡���Х���![%d]", WSAGetLastError());
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
		strLog.Format("WSACreateEvent����(Send)![%d]", WSAGetLastError());
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
		strLog.Format("WSACreateEvent����(Recv)![%d]", WSAGetLastError());
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

	// ���o�u�@����
	if (!GetFileFilterFromDB())
	{
		strLog.Format("QueryDB�s�u���~!��[%s]", m_host);
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

	// �Ĥ@�����o�u�@���e
	GetFileHandleFromMaster();
	DWORD dwTimeout = GetTickCount();
//	if (m_mapWork.GetCount() == 0)
//	{
//		strLog.Format("QueryDB�S���ɮ׳]�w!��[%s]", m_host);
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

	// �אּ�����ݼҦ�
	//ULONG mode = 1;
	//ioctlsocket(m_socket, FIONBIO, &mode);

	// �אּ�����X�e�X�Ҧ�
	BOOL bOptVal = TRUE;
	int	bOptLen = sizeof(BOOL);
	int err = setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, (char *) &bOptVal, bOptLen);
    if (err==SOCKET_ERROR) 
	{
		strLog.Format("setsockopt���~![%d]", GetLastError());
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

	// �}�l�u�@
	BOOL bPacketControl = TRUE;
	BOOL bForceSync = TRUE;
	BOOL bResetRecv = TRUE;
	UINT64 tra = 0; 

	while (!IsStop())
	{
		DWORD tick = GetTickCount();

		// Timeout
		if ((tick-dwTimeout) > 100000) // 5����
		{
			strLog.Format("[�D��:%s]�S������^��!��[%u:%u]", m_host, tick, dwTimeout);
			log->DFLogSend(strLog, DF_SEVERITY_HIGH);
			break;
		}

		// ��z�u�@
		UpdateFileHandleFromMaster();
		
		// �}�l�ǰe
//		if (m_debug)
//		{
//			strLog.Format("[�D��:%s]����ڪ��^�X!", m_host);
//			log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
//		}
		
		// �M�����
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
				break; // ��m�����o
			}

			remain = MAX_PACKET_SIZE-total-sizeof(MTKINFO)-sizeof(DWORD); 
			tick = GetTickCount();
			key = 0;
			work = NULL;
			m_mapWork.GetNextAssoc(pos, key, work);
			
			if (work == NULL) // �w�����Ѫ��ɮ�
			{
				// todo
				strLog.Format("[�D��:%s]�_�Ǫ��Ʊ� �����Ѫ��ɮ�!", m_host);
				log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
				continue;
			}

			if (work->m_bIsRef == FALSE) // �Q�����q�\���ɮ�
			{
				if (work->m_mtk->m_info.m_attribute == TRUE) // �٬O�⦳�q�\,�u�O���e���,�ҥH�n�ˬd�ɮ׵����F�S (PS.�]���p�G�����q�M��R��,�U���u�@�٬O�|�o�{�o���ɮפS�[�^��)
				{
					MTK* mtk = work->m_mtk;

					AcquireSRWLockExclusive(&m_lock);
					m_mapWork.RemoveKey(key);
					ReleaseSRWLockExclusive(&m_lock);

					InterlockedDecrement(&mtk->m_uRefCount);
					delete work;

					strLog.Format("�ǿ駹��(�����q�\)![�D��:%s][�ɮ�:%s][�Ѿl�q�\:%d]", m_host, mtk->m_chFilename, mtk->m_uRefCount);
					log->DFLogSend(strLog, DF_SEVERITY_INFORMATION);
				}
			}
			else if (work->m_heartbeat == 0) // ���s���ɮ�
			{
				DWORD filenamelen = strlen(work->m_mtk->m_chFilename);
				if (remain < filenamelen) break; // ��m�����o

				*(HANDLE*)&m_packet->m_pkt[total] = work->m_mtk->m_info.m_handle;
				*(DWORD*)&m_packet->m_pkt[total+4] = 2;
				*(UINT64*)&m_packet->m_pkt[total+8] = *(UINT64*)&work->m_overlapped.Offset;
				*(FILETIME*)&m_packet->m_pkt[total+16] = work->m_mtk->m_info.m_lastwrite;
				*(UINT*)&m_packet->m_pkt[total+24] = filenamelen;
				memcpy(&m_packet->m_pkt[total+28], work->m_mtk->m_chFilename, filenamelen);
				
				total += (28+filenamelen);
				work->m_heartbeat = tick;
			}
			else if (*(UINT64*)&work->m_overlapped.Offset < work->m_mtk->m_info.m_size) // �٦��S�Ǫ����
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
									strLog.Format("�_�Ǫ��Ʊ� �SŪ����![%d][%s]", retlen, work->m_mtk->m_chFilename);
									log->DFLogSend(strLog, DF_SEVERITY_WARNING);
								}
							}
						}
						else
						{
							// ERROR
							strLog.Format("�_�Ǫ�Ū�ɦ^�ǭ�![%d][%s]", dw, work->m_mtk->m_chFilename);
							log->DFLogSend(strLog, DF_SEVERITY_HIGH);
							continue;
						}
					}
					else
					{
						// ERROR
						strLog.Format("�ɮ�Ū������![%d][%s]", GetLastError(), work->m_mtk->m_chFilename);
						log->DFLogSend(strLog, DF_SEVERITY_HIGH);
						continue;
					}
				}

				ResetEvent(work->m_overlapped.hEvent);

				// CheckReadFile
				if (retlen <= 0)
				{
					strLog.Format("Readfile���`!?![%s][%d]", work->m_mtk->m_chFilename, retlen);
					log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
				}

				// CheckOffset
				if (uCheckOffset != *(UINT64*)&work->m_overlapped.Offset)
				{
					strLog.Format("Offset�ȳQ�ﱼ�F!?![%s][BE:%d][AF:%d][RET:%d]", work->m_mtk->m_chFilename, uCheckOffset, work->m_overlapped.Offset, retlen);
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
					strLog.Format("Offset�ȭp����~!?![%s][BE:%d][AF:%d][RET:%d]", work->m_mtk->m_chFilename, uCheckOffset, work->m_overlapped.Offset, retlen);
					log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
				}
			}
			else // ���ݭn�Ǹ��
			{
				BOOL bIsReadonly = work->m_mtk->m_info.m_attribute;
				BOOL bIsDone = (*(UINT64*)&work->m_overlapped.Offset == work->m_mtk->m_info.m_size);
				if ((bIsReadonly && bIsDone) || (tick - work->m_heartbeat >= 10000)) // �W�L10��"��"�ݩ�ReadOnly�n�eHB
				{
					*(HANDLE*)&m_packet->m_pkt[total] = work->m_mtk->m_info.m_handle;
					*(DWORD*)&m_packet->m_pkt[total+4] = /*work->m_mtk->m_info.m_attribute*/(bIsReadonly && bIsDone);
					*(UINT64*)&m_packet->m_pkt[total+8] = *(UINT64*)&work->m_overlapped.Offset;
					*(FILETIME*)&m_packet->m_pkt[total+16] = work->m_mtk->m_info.m_lastwrite;
					*(UINT*)&m_packet->m_pkt[total+24] = 0;

					total += 28;
					work->m_heartbeat = tick;
				}

				if (bIsReadonly && bIsDone) // ��Ū�ɮ�+�ǰe����=�Ӱh���F
				{
					MTK* mtk = work->m_mtk;

					AcquireSRWLockExclusive(&m_lock);
					m_mapWork.RemoveKey(key);
					ReleaseSRWLockExclusive(&m_lock);

					InterlockedDecrement(&mtk->m_uRefCount);
					delete work;
					
					strLog.Format("�ǿ駹��(�߸�����)![�D��:%s][�ɮ�:%s][�Ѿl�q�\:%d]", m_host, mtk->m_chFilename, mtk->m_uRefCount);
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
		if (total > 0) // ����ƭn�ǩΦ�HB�n�g=�~�򩹫e
		{
			m_packet->m_pktlen = total;

			if ((m_packet->m_pktlen>256) && m_compress && m_zstd && theApp.m_ptDict) // ���Y
			{
				UINT nOutLen = 0;
				if (!ZSTD_isError(nOutLen=(int)ZSTD_compress_usingCDict(m_zstd, (char*)m_sendbuf+sizeof(int), MAX_PACKET_SIZE, (const char*)m_packet, m_packet->m_pktlen+sizeof(int), theApp.m_ptDict)))
				{	
					// ���Y���\
					*(int*)m_sendbuf = nOutLen | ZSTD_COMPRESS;	// len(OR)���Y�X��

					//memset(&DataBuf, NULL, sizeof(WSABUF));
					//DataBuf.len = 0;
					//DataBuf.buf = NULL;
					DataBuf.len = nOutLen+4;
					DataBuf.buf = (char*)m_sendbuf;
				}
				else
				{
					// ���Y����
					strLog.Format("�_�Ǫ��Ʊ� ���Y����![com][%s][%d][%d]", ZSTD_getErrorName(nOutLen), nOutLen, total);
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
									strLog.Format("�_�Ǫ��Ʊ� �S�g�J���![%d]", sendlen);
									log->DFLogSend(strLog, DF_SEVERITY_WARNING);
								}
							}
					}
					else
					{
						strLog.Format("WSASend����![%d]", WSAGetLastError());
						log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
						break;
					}
				}

				WSAResetEvent(SendOverlapped.hEvent);

				if (DataBuf.len != sendlen)
				{
					strLog.Format("�_�Ǫ��Ʊ� �e�X���j�p���@��![com][%d][%d]", nOutLen+4, sendlen);
					log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
				}
				//}
				//else 
				//{
				//	// ���Y����=>���N�����Y
				//	strLog.Format("�_�Ǫ��Ʊ� ���Y����![com][%s][%d][%d]", ZSTD_getErrorName(nOutLen), nOutLen, total);
				//	log->DFLogSend(strLog, DF_SEVERITY_DEBUG);

				//	memset(&DataBuf, NULL, sizeof(WSABUF));
				//	DataBuf.len = m_packet->m_pktlen+4;
				//	DataBuf.buf = m_packet->m_pkt;

				//	DWORD dFlag = 0;
				//}
			}
			else // �����Y
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
									strLog.Format("�_�Ǫ��Ʊ� �S�g�J���![%d]", sendlen);
									log->DFLogSend(strLog, DF_SEVERITY_WARNING);
								}
							}

						//WSAResetEvent(SendOverlapped.hEvent);
					}
					else
					{
						strLog.Format("WSASend����![%d]", WSAGetLastError());
						log->DFLogSend(strLog, DF_SEVERITY_DISASTER);
						break;
					}
				}

				WSAResetEvent(SendOverlapped.hEvent);

				//TRACE("[%I64d]%d\n", tra++, sendlen);

				if (DataBuf.len != sendlen)
				{
					strLog.Format("�_�Ǫ��Ʊ� �e�X���j�p���@��![%d][%d]", total+4, sendlen);
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
				//					strLog.Format("�_�Ǫ��Ʊ� �S�g�X���![%d]", ret);
				//					log->DFLogSend(strLog, DF_SEVERITY_WARNING);
				//				}
				//			}
				//	}
				//	else
				//	{
				//		// ERROR
				//		strLog.Format("�ʥ]�ǰe����![%d]", GetLastError());
				//		log->DFLogSend(strLog, DF_SEVERITY_HIGH);
				//		continue;
				//	}
				//}

				//if ((n = send(m_socket, (char*)m_packet, total+4, 0)) <= 0)
				//{
				//	// Error
				//	strLog.Format("�s�u�ɮ׶ǰe����![%d][%s]", m_packet->m_pktlen, m_packet->m_pkt);
				//	log->DFLogSend(strLog, DF_SEVERITY_HIGH);
				//	closesocket(m_socket);
				//	break;
				//}
			}

			//// Packet Control
			//if (sendlen <= 10000) // 10K
			//{
			//	//strLog.Format("[�D��:%s]�ʥ]�ո`(1)!", m_host);
			//	//log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
			//	Sleep(200);
			//}
			//else if (sendlen <= 20000) // 20K
			//{
			//	//strLog.Format("[�D��:%s]�ʥ]�ո`(2)!", m_host);
			//	//log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
			//	Sleep(200);
			//}
			//else if (sendlen <= 30000) // 30K
			//{
			//	//strLog.Format("[�D��:%s]�ʥ]�ո`(3)!", m_host);
			//	//log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
			//	Sleep(150);
			//}
			//else if (sendlen <= 40000) // 40K
			//{
			//	//strLog.Format("[�D��:%s]�ʥ]�ո`(4)!", m_host);
			//	//log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
			//	Sleep(100);
			//}

			// Check WSASend
			if (sendlen == 0)
			{
				strLog.Format("[�D��:%s]����ƭn�ǰe�o�e�X0byte!", m_host);
				log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
			}

			// ��s�s�u��T(For UI)
			AcquireSRWLockExclusive(&m_lock);

			// �C���m�y�q�p��
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

			// Timeout���m
			dwTimeout = tick;
//			strLog.Format("[�D��:%s]Timeout���m[����ưe�X][dwTimeout:%d]", m_host, dwTimeout);
//			log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
		}
		else // �S��ƥi��+�SHB�i�g=�B�@100ms
		{
			Sleep(100);
			if (m_debug)
			{
				strLog.Format("[�D��:%s]�S�������ƭn�ǰe![Sleep:100ms]", m_host);
				log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
			}
//			continue;
		}

		if (m_debug)
		{
			strLog.Format("[�D��:%s]�����o�^�X!", m_host);
			log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
		}

		bPacketControl = TRUE;

		// �P�B��T
		DWORD recvlen = 0;
		DWORD recvFlag = 0;
		
		if (m_debug)
		{
			strLog.Format("�P[Host:%s]1.���ݱ����P�B��T!", m_host);
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
					strLog.Format("�_�Ǫ��Ʊ� ��ƪ��צ��~![%d]", recvlen);
					log->DFLogSend(strLog, DF_SEVERITY_WARNING);
					break;
				}
			}

			bForceSync = FALSE;
			bResetRecv = TRUE;
			WSAResetEvent(RecvOverlapped.hEvent);

			// Timeout���m
			dwTimeout = tick;
//			strLog.Format("[�D��:%s]Timeout���m[����P�B���][dwTimeout:%d]", m_host, dwTimeout);
//			log->DFLogSend(strLog, DF_SEVERITY_HIGH);
			//strLog.Format("Timeout���m![1][%s][%u]", m_host, dwTimeout);
			//log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
		}
		else if (dw == WAIT_TIMEOUT)
		{
			// TIMEOUT �S�ƪ�,�S���ɮ׻ݭn�P�B
			continue;
		}
		else
		{
			// FAIL ����
			strLog.Format("�_�Ǫ��Ʊ� �����D�^�Ǭƻ�![%d]", dw);
			log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
			break;
		}

		//TRACE("�P�B\n");

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
				strLog.Format("�P[Host:%s]2.�B�z�P�B��T����![len:%d][�w�B�z:%d]", m_host, recvlen, synclen);
				log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
			}

			recvid = *(UINT*)(m_recvbuf+synclen);
			recvhandle = *(UINT*)(m_recvbuf+synclen+4);
			recvoffset = *(UINT64*)(m_recvbuf+synclen+8);

			//memcpy(&recvhandle, (char*)m_recvbuf+synclen, sizeof(UINT));
			//memcpy(&recvoffset, (char*)m_recvbuf+synclen+sizeof(UINT), sizeof(UINT64));
				
			//TRACE("�P�B:%x:%I64d\n", recvhandle, recvoffset);

			if (m_debug)
			{
				strLog.Format("�P[Host:%s]3.�B�z�P�B��T���![handle:%d][offset:%I64d]", m_host, recvhandle, recvoffset);
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
				// �䤣��N��F
				strLog.Format("�_�Ǫ��Ʊ� [�D��:%s]�ǨӤ��{�Ѫ��ɮץN��!", m_host);
				log->DFLogSend(strLog, DF_SEVERITY_WARNING);

				synclen += syncdatalen;
				continue;
			}

			switch (recvid)
			{
			case 0: // �����q�\
				{
					if (m_debug)
					{
						strLog.Format("[�D��:%s]�����q�\�ɮ�[%s]!", m_host, work->m_mtk->m_chFilename);
						log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
					}
					
					work->m_bIsRef = FALSE;	
					
					break;
				}
			case 1: // �P�B�ɮ�
				{
					memcpy(&work->m_overlapped.Offset, &recvoffset, sizeof(UINT64));

					if (recvoffset > work->m_mtk->m_info.m_size)
					{
						strLog.Format("�_�Ǫ��Ʊ� [�D��:%s]�ɮפj��D����![%s][%I64d][%I64d]", m_host, work->m_mtk->m_chFilename, recvoffset, work->m_mtk->m_info.m_size);
						log->DFLogSend(strLog, DF_SEVERITY_WARNING);
					}
					
					break;
				}
			case 2: // ��������
				{
					work->m_heartbeat = 0;

					if (m_debug)
					{
						strLog.Format("[�D��:%s]���s�ǿ��ɮ�[%s]!", m_host, work->m_mtk->m_chFilename);
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
			//	// �s
			//	synclen += syncdatalen;
			//	continue;
			//}

			//WORK* work = NULL;
			//if (!m_mapWork.Lookup(recvhandle, (WORK*&)work))
			//{
			//	// �䤣��N��F
			//	strLog.Format("�_�Ǫ��Ʊ� [�D��:%s]�ǨӤ��{�Ѫ��ɮץN��!", m_host);
			//	log->DFLogSend(strLog, DF_SEVERITY_WARNING);

			//	synclen += syncdatalen;
			//	continue;
			//}
			//				
			//if (recvoffset == 0)
			//{
			//	if (m_debug)
			//	{
			//		strLog.Format("[�D��:%s]�����q�\�ɮ�[%s]!", m_host, work->m_mtk->m_chFilename);
			//		log->DFLogSend(strLog, DF_SEVERITY_DEBUG);
			//	}

			//	work->m_ref = FALSE;
			//}
			//else
			//{
			//	memcpy(&work->m_overlapped.Offset, &recvoffset, sizeof(UINT64));

			//	if (recvoffset > work->m_mtk->m_info.m_size)
			//	{
			//		strLog.Format("�_�Ǫ��Ʊ� [�D��:%s]�ɮפj��D����![%I64d][%I64d]", m_host, recvoffset, work->m_mtk->m_info.m_size);
			//		log->DFLogSend(strLog, DF_SEVERITY_WARNING);
			//	}

			//	//TRACE("�P�B: File:%s Size:%I64d Offset:%I64d\n", work->m_mtk->m_chFilename, (UINT64)work->m_mtk->m_info.m_size, (UINT64)work->m_overlapped.Offset);
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

	// ����Ҧ����u�@
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

	strLog.Format("�s�u���_ ����ǵ���![Thread:%d]", m_nThreadID);
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

		if ((find = strstr(buf+readi, "\r\n\r\n")) > 0) // ����}�Y
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

	//if ((find = strstr(buf, "\r\n\r\n")) > 0) // ����}�Y
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
		
		// �����@�ӷs�u�H
		if ((socket = accept(m_socket, (SOCKADDR*)&client_addr, &client_len)) == INVALID_SOCKET)
		{
			int err = GetLastError();
			if (err == 10004)
			{
				strLog.Format("�����s�u�ݤf�w����(10004)");
				log->DFLogSend(strLog, DF_SEVERITY_INFORMATION);
				continue;
			}

			strLog.Format("�����s�u����(%d)", GetLastError());
			log->DFLogSend(strLog, DF_SEVERITY_HIGH);
			continue;
		}

		// �����n�J��T
		int n=0;
		char chLogin[100]={0};
		if ((n = recv(socket, chLogin, 100, 0)) < 0)
		{
			strLog.Format("�����n�J����(%d)", GetLastError());
			log->DFLogSend(strLog, DF_SEVERITY_HIGH);
			closesocket(socket);
			continue;
		}

		// �B�z�n�J��T
		if (memcmp(chLogin, "LOGIN:", 6) != 0)
		{
			strLog.Format("�n�J��T�榡���~[%s]", chLogin);
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
		
		// ��s�u�H�W��+�}�l�W�u
		CString strKey("");
		strKey.Format("%s:%d", inet_ntoa(client_addr.sin_addr), htons(client_addr.sin_port));
//		AcquireSRWLockExclusive(&m_lockMaster);
//		m_mapSlaveList[strKey] = slave;
//		ReleaseSRWLockExclusive(&m_lockMaster);
		
		if (m_mapSlaveList[strKey] != NULL)
		{
			strLog.Format("�s�b���ƪ��s�u[%s]", strKey);
			log->DFLogSend(strLog, DF_SEVERITY_HIGH);
			closesocket(socket);
			continue;
		}

		try
		{
			CDFNetSlave* slave = new CDFNetSlave(CDFNetSlave::ThreadFunc, dlg, socket, inet_ntoa(client_addr.sin_addr), htons(client_addr.sin_port), chLoginName, chLoginKey);
			if (slave == NULL)
			{
				strLog.Format("�O���餣��!�L�k�}�ҷs���s�u![%s]", strKey);
				log->DFLogSend(strLog, DF_SEVERITY_HIGH);
				closesocket(socket);

				delete slave;
				slave = NULL;
				continue;
			}
		
			VERIFY(slave->CreateThread());
			//if (!slave->CreateThread())
			//{
			//	strLog.Format("�O���餣��!�L�k�}�ҷs�������![%s]", strKey);
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
			strLog.Format("[EXCEPTION]�O���餣��!�L�k����s�������![%s]", strKey);
			log->DFLogSend(strLog, DF_SEVERITY_HIGH);
			closesocket(socket);
			continue;
		}

		// ����
		strLog.Format("�s���u�H!!!(%s)", inet_ntoa(client_addr.sin_addr));
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
	//		strLog.Format("[�{������]�����ϥΪ�(Thread:%d)", tid);
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

			strLog.Format("�s�u���_![Host:%s]", slave->m_host);
			log->DFLogSend(strLog, DF_SEVERITY_WARNING);

			m_mapSlaveList.RemoveKey(key);
			delete slave;
			slave = NULL;
		}
	}
	m_mapSlaveList.RemoveAll();
		
	ReleaseSRWLockExclusive(&m_lockMaster);
}

