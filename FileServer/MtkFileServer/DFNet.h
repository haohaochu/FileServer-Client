
// DFNet.h : 檔案
//

#pragma once

#ifndef __AFXWIN_H__
	#error "對 PCH 包含此檔案前先包含 'stdafx.h'"
#endif

#include "resource.h"		// 主要符號
#include "mythread.h"
#include "C:\\Users\\hao\\Desktop\\zstd-v1.1.2-win64\\include\\zstd.h"

// definition
#define MAX_BUFFER_SIZE 4*1024*1024
#define MAX_PACKET_SIZE MAX_BUFFER_SIZE-4

// command packet type definition
#define PACKET_SYNC 0

// data packet type definition 
#define PACKET_WRITE 0
#define PACKET_READONLY 1
#define PACKET_NEWFILE 2
#define PACKET_DELETEFILE 3

// compress type
const unsigned int	NO_COMPRESS		= 0x00000000;	// 不壓縮
const unsigned int	ZSTD_COMPRESS	= 0x80000000;	// Zstd壓縮	

// thread variable
extern __declspec(thread) ZSTD_CCtx* m_zstd;
extern __declspec(thread) LPFN_TRANSMITPACKETS fnTransmitPackets;

// structure WORK,PACKET
typedef struct CMtkFile MTK;

// 1項工作
typedef struct CDFNetWork
{
	MTK*			m_mtk;
	OVERLAPPED		m_overlapped;
	DWORD			m_heartbeat;
	UINT			m_uAge;
	BOOL			m_bIsRef;
} WORK;

// 1個封包
typedef struct CDFNetPacket
{
	UINT			m_pktlen;
	char			m_pkt[MAX_PACKET_SIZE];
} PACKET;

// 1個封包的頭
typedef struct CDFNetPacketHeader
{
	HANDLE			m_handle;		// 4byte
	UINT			m_attribute;	// 4byte
	UINT64			m_offset;		// 8byte
	FILETIME		m_lastwrite;	// 8byte
	UINT			m_datalen;		// 4byte
	char			m_dummy[4];		// 4byte
} HEADER; // 32 byte

// class CDFNetSlave
// - Net工人
class CDFNetSlave : public CMyThread
{
public:
	CDFNetSlave(AFX_THREADPROC pfnThreadProc, LPVOID param, SOCKET sock, char* host, UINT port, char* name, char* key);
	~CDFNetSlave(void);

protected:
	virtual void	Go(void);
	SOCKET			m_socket;
	
	PACKET*			m_packet;
	char*			m_sendbuf;
	char*			m_recvbuf;

	bool			ApplyFilter(CString& fname);
	bool			GetFileFilterFromDB();
	void			GetFileHandleFromMaster();
	void			UpdateFileHandleFromMaster();

public:
	char			m_host[20];
	UINT			m_port;
	char			m_name[32];
	char			m_key[32];
	BOOL			m_debug;
	BOOL			m_compress;
	
	SRWLOCK			m_lock;
	UINT			m_age;
	SYSTEMTIME		m_starttime;
	SYSTEMTIME		m_currenttime;
	DWORD			m_lastsec;
	DWORD			m_flow;
	DWORD			m_maxflow;
	DWORD			m_comvolumn;
	DWORD			m_realvolumn;

	vector<CString>				m_vecFilter;
	CMap<UINT, UINT&, WORK*, WORK*&> m_mapWork;

	void			ForceStopSlave();
	BOOL			IsStopSlave();
};


// class CDFNetMaster
// - Net管理員
class CDFNetMaster : public CMyThread
{
public:
	CDFNetMaster(AFX_THREADPROC pfnThreadProc, LPVOID param, UINT port);
	~CDFNetMaster(void);

protected:
	virtual void	Go(void);
	SOCKET			m_socket;
	UINT			m_port;

	bool			CreateSocket();
	void			DeleteSocket();

public:
	//CArray<CDFNetSlave*, CDFNetSlave*&> m_arySlaveList;
	SRWLOCK			m_lockMaster;
	CMapStringToPtr m_mapSlaveList;
	BOOL			m_bRefresh;

	void			ForceStopMaster();
};


