
// DFNet.h : �ɮ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�� PCH �]�t���ɮ׫e���]�t 'stdafx.h'"
#endif

#include "resource.h"		// �D�n�Ÿ�
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
const unsigned int	NO_COMPRESS		= 0x00000000;	// �����Y
const unsigned int	ZSTD_COMPRESS	= 0x80000000;	// Zstd���Y	

// structure WORK,PACKET
typedef struct CDFFile MTK;

// 1���u�@
typedef struct CDFNetWork
{
	MTK*			m_mtk;
	DWORD			m_heartbeat;
	UINT64			m_offset;
} WORK;

// 1�ӫʥ]
typedef struct CDFNetPacket
{
	UINT			m_pktlen;
	char			m_pkt[MAX_PACKET_SIZE];
} PACKET;

// 1�ӻݨD
typedef struct CDFNetRequest
{
	UINT			m_id;
	HANDLE			m_handle;
	UINT64			m_offset;
} REQUEST;


// class CDFNetMaster
// - Net�޲z��
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

	// note:�ۤv�Ϊ�,������
	CMap<UINT, UINT&, MTK*, MTK*&> m_mapFileList; 
	//vector<UINT>	m_vecSyncList;
	vector<REQUEST*> m_vecSyncList;
public:
	// note:�R�A���,������
	UINT			m_NetServerID;
	char			m_NetServerIP[16];
	UINT			m_NetServerPort;
	//UINT			m_NetLocalPort;
	
	// note:��UI��ܪ���T,�n��
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


