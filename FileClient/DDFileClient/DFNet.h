
// DFNet.h : 檔案
//

#pragma once

#ifndef __AFXWIN_H__
	#error "對 PCH 包含此檔案前先包含 'stdafx.h'"
#endif

#include "resource.h"		// 主要符號
#include "mythread.h"


// definition
#define MAX_BUFFER_SIZE 4*1024*1024
#define MAX_PACKET_SIZE MAX_BUFFER_SIZE-4
#define MAX_CONTENT_SIZE MAX_PACKET_SIZE-4096

#define PACKET_WRITE 0
#define PACKET_READONLY 1
#define PACKET_NEWFILE 2
#define PACKET_DELETEFILE 3

// thread variable
//extern __declspec(thread) ZSTD_CCtx* m_zstd;

// compress type
const unsigned int	NO_COMPRESS		= 0x00000000;	// 不壓縮
const unsigned int	ZSTD_COMPRESS	= 0x80000000;	// Zstd壓縮	

// structure WORK,PACKET
typedef struct CDFFile MTK;

// 1項工作
typedef struct CDFNetWork
{
	MTK*			m_mtk;
	DWORD			m_heartbeat;
	UINT64			m_offset;
} WORK;

// 1個封包
typedef struct CDFNetPacket
{
	UINT			m_pktlen;
	char			m_pkt[MAX_PACKET_SIZE];
} PACKET;

// 1個需求
typedef struct CDFNetRequest
{
	UINT			m_id;
	HANDLE			m_handle;
	UINT64			m_offset;
} REQUEST;


// class CDFNetMaster
// - Net管理員
class CDFNetMaster : public CMyThread
{
public:
	CDFNetMaster(AFX_THREADPROC pfnThreadProc, LPVOID param, UINT id, char* host, UINT port);
	~CDFNetMaster(void);

protected:
	virtual void	Go(void);
	SOCKET			m_socket;

	PACKET*			m_packet;
	PACKET*			m_depacket;
	BOOL			m_debug;

	bool			Login();
	bool			CreateSocket();
	void			DeleteSocket();

	// note:自己用的,不用鎖
	CMap<UINT, UINT&, MTK*, MTK*&> m_mapFileList; 
	//vector<UINT>	m_vecSyncList;
	vector<REQUEST*> m_vecSyncList;
public:
	// note:靜態資料,不用鎖
	UINT			m_NetServerID;
	char			m_NetServerIP[16];
	UINT			m_NetServerPort;
	//UINT			m_NetLocalPort;
	
	// note:給UI顯示的資訊,要鎖
	SRWLOCK			m_NetServerStatusLock;
	char			m_NetServerStartTime[18];
	DWORD			m_NetServerLastTick;
	UINT			m_NetServerFileCount;
	UINT64			m_NetServerVolumn;
	UINT64			m_NetServerTotalVolumn;
	UINT64			m_NetServerComVolumn;
	UINT64			m_NetServerTotalComVolumn;

	void			ForceStopMaster();
};


