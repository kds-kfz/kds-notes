#include "publicfunc.h"
#include "sexport.h"

#include <map>
#include <mutex>

// 本地最小声明新版 SocketServer 接口，避免包含 SocketServer.h 时把其 dllexport 类符号带入本 DLL 导出表。
enum EN_TCP_SOCK_NOTIFY_TYPE
{
	enTc_Close = 0,
	enTc_Connect,
	enTc_Error,
	enTc_Data,
};

typedef void(*TCP_NOTIFY_PROC)(
	void* p_refServerHandle,
	void* p_refClientHandle,
	EN_TCP_SOCK_NOTIFY_TYPE p_enType,
	const void* p_pData,
	unsigned int p_uiDataLen,
	const char* p_szClientIp,
	unsigned short p_unClientPort,
	void* p_pErrData);

class CSocketServerCompat
{
public:
	virtual ~CSocketServerCompat() {}
	virtual bool CreateTcpSock(const char* p_szIp, unsigned short p_unPort, unsigned int p_uiRBufLen, unsigned int p_uiMaxConnectNum, unsigned int p_uiMaxAcceptNum, TCP_NOTIFY_PROC p_pfnTcpHandle, unsigned int p_uiThreadNum, unsigned int p_uiQueueNum, char* p_szErr, const char* p_szLogFold = nullptr) = 0;
	virtual void StopTcpSock() = 0;
	virtual bool TcpSockSend(void* p_refServer, void* p_refClient, const char* p_szData, int p_iDataLen) = 0;
	virtual void TcpSockClose(void* p_refServer, void* p_refClient, const char* p_szData, int p_iDataLen) = 0;
	virtual int TcpSockCompare(void* p_refSrcClient, void* p_refObjClient) = 0;
	virtual bool TcpSockIsAlive(void* p_refServer, void* p_refClient) = 0;
};

extern "C" CSocketServerCompat* CreateTcpSockInstance();
extern "C" void DelTcpSockInstance(CSocketServerCompat*& p_pInstance);

// 旧 s.dll 服务句柄的内部实现，保存新版 SocketServer 实例和旧回调。
struct ST_TAG_HS
{
	// 新版 SocketServer 对象，由 s_create 创建并由 s_destroy 释放。
	CSocketServerCompat* pServer = nullptr;
	// 旧通知回调和调用方参数，桥接回调时原样透传。
	S_NOTIFY_PROC pfnCallback = nullptr;
	void* pParam = nullptr;
	std::mutex mutexDataLock;
	// 兼容 s_setdata/s_getdata 的 per-client 附加数据表。
	std::map<HCLIENT, DWORD> mapClientData;
};

// 客户端句柄只作为不透明指针比较和回传。
struct ST_TAG_HCLIENT
{
};

// 将新版 SocketServer 通知枚举转换为旧 TCP 推送模块识别的枚举。
static EN_S_NOTIFY_TYPE ToOldNotify(EN_TCP_SOCK_NOTIFY_TYPE p_enType)
{
	switch (p_enType)
	{
	case enTc_Close:
		return EN_S_TYPE_CLOSE;
	case enTc_Connect:
		return EN_S_TYPE_CONNECTED;
	case enTc_Error:
		return EN_S_TYPE_ERROR;
	case enTc_Data:
		return EN_S_TYPE_DATA;
	default:
		return EN_S_TYPE_ERROR;
	}
}

// 新旧回调桥接函数，隐藏新版回调参数差异并保持旧 ABI。
static void TcpNotifyBridge(
	void* p_hServerHandle,
	void* p_hClientHandle,
	EN_TCP_SOCK_NOTIFY_TYPE p_enType,
	const void* p_pData,
	unsigned int p_uDataLen,
	const char* p_szIp,
	unsigned short p_uPort,
	void* p_pErr)
{
	auto* p_hHandle = reinterpret_cast<HS>(p_hServerHandle);
	if (!p_hHandle || !p_hHandle->pfnCallback)
	{
		return;
	}

	p_hHandle->pfnCallback(
		p_hHandle,
		reinterpret_cast<HCLIENT>(p_hClientHandle),
		ToOldNotify(p_enType),
		p_pData,
		static_cast<int>(p_uDataLen),
		p_szIp,
		p_uPort,
		p_hHandle->pParam);
}

// 创建 TCP 服务，参数含义保持旧 s_create 约定。
extern "C" HS s_create(
	unsigned short p_uPort,
	int p_iRBufLen,
	int p_iMaxConnectNum,
	int p_iMaxConnectNumPerIp,
	int p_iMaxIdleSecond,
	S_NOTIFY_PROC p_pfnCallback,
	void* p_pParam,
	int p_iHandleThreadNum,
	char* p_szErr)
{
	CSocketServerCompat* pServer = CreateTcpSockInstance();
	if (!pServer)
	{
		if (p_szErr)
		{
			strcpy_s(p_szErr, 1024, "CreateTcpSockInstance failed");
		}
		return nullptr;
	}

	auto* p_hHandle = new ST_TAG_HS;
	p_hHandle->pServer = pServer;
	p_hHandle->pfnCallback = p_pfnCallback;
	p_hHandle->pParam = p_pParam;

	char szLocalErr[1024] = {0};
	const bool bOk = pServer->CreateTcpSock(
		"0.0.0.0",
		p_uPort,
		static_cast<unsigned int>(p_iRBufLen),
		static_cast<unsigned int>(p_iMaxConnectNum),
		static_cast<unsigned int>(p_iMaxConnectNumPerIp),
		TcpNotifyBridge,
		static_cast<unsigned int>(p_iHandleThreadNum),
		1,
		p_szErr ? p_szErr : szLocalErr,
		nullptr);
	if (!bOk)
	{
		DelTcpSockInstance(pServer);
		delete p_hHandle;
		return nullptr;
	}

	return p_hHandle;
}

extern "C" void s_send(HS p_hHandle, HCLIENT p_hUser, char* p_pData, int p_iDataLen)
{
	if (p_hHandle && p_hHandle->pServer)
	{
		p_hHandle->pServer->TcpSockSend(p_hHandle, p_hUser, p_pData, p_iDataLen);
	}
}

// 旧库遍历接口暂未被当前推送逻辑使用，保留空实现维持链接兼容。
extern "C" void s_travel(HS p_hHandle, S_TRAVEL_FUNC p_pfnTravel, void* p_pParam)
{
}

extern "C" void s_close(HS p_hHandle, HCLIENT p_hUser)
{
	if (p_hHandle && p_hHandle->pServer)
	{
		p_hHandle->pServer->TcpSockClose(p_hHandle, p_hUser, nullptr, 0);
	}
}

extern "C" void s_destroy(HS p_hHandle)
{
	if (!p_hHandle)
	{
		return;
	}
	if (p_hHandle->pServer)
	{
		p_hHandle->pServer->StopTcpSock();
		CSocketServerCompat* pServer = p_hHandle->pServer;
		DelTcpSockInstance(pServer);
		p_hHandle->pServer = nullptr;
	}
	delete p_hHandle;
}

extern "C" void s_setdata(HS p_hHandle, HCLIENT p_hUser, DWORD p_dwData)
{
	if (!p_hHandle)
	{
		return;
	}
	std::lock_guard<std::mutex> clLock(p_hHandle->mutexDataLock);
	p_hHandle->mapClientData[p_hUser] = p_dwData;
}

extern "C" void s_cleardata(HS p_hHandle, HCLIENT p_hUser)
{
	if (!p_hHandle)
	{
		return;
	}
	std::lock_guard<std::mutex> clLock(p_hHandle->mutexDataLock);
	p_hHandle->mapClientData.erase(p_hUser);
}

extern "C" bool s_getdata(HS p_hHandle, HCLIENT p_hUser, DWORD* p_pdwData)
{
	if (!p_hHandle || !p_pdwData)
	{
		return false;
	}
	std::lock_guard<std::mutex> clLock(p_hHandle->mutexDataLock);
	auto it = p_hHandle->mapClientData.find(p_hUser);
	if (it == p_hHandle->mapClientData.end())
	{
		return false;
	}
	*p_pdwData = it->second;
	return true;
}

// 旧代码通过 s_compare 判断连接句柄是否仍是同一个客户端。
extern "C" int s_compare(HCLIENT p_hFirst, HCLIENT p_hSecond)
{
	if (p_hFirst == p_hSecond)
	{
		return 0;
	}
	return p_hFirst > p_hSecond ? 1 : -1;
}