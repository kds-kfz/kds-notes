#include "CloudNetDataApi.h"

#include <algorithm>
#include <cstring>
#include <intrin.h>
#include <map>
#include <mutex>
#include <new>
#include <string>

#define GetLastError Win32GetLastError
#include "IceRPCPush.h"
#undef GetLastError
#ifdef StartService
#undef StartService
#endif

namespace
{
// 统一错误码，便于 C 调用方判断失败原因。
const int s_iCloudNetOk = 0;
const int s_iCloudNetInvalidParam = -1;
const int s_iCloudNetNotFound = -2;
const int s_iCloudNetCreateFailed = -3;
const int s_iCloudNetCallFailed = -4;

// 连接上下文保存底层客户端句柄和推送回调信息。
struct ST_CONNECTION_CONTEXT
{
	HANDLE hClient;                    // IceRPCPush 客户端句柄，由 DeleteJsonICERPC 释放。
	PFN_CLOUD_NET_PUSH pfnPush;        // 调用方注册的推送回调。
	void* pPushParam;                  // 推送回调透传参数。
	std::string strConnName;           // 本库内部连接名。
	std::string strRegisterRet;        // 底层注册返回字符串缓存。

	ST_CONNECTION_CONTEXT()
	{
		hClient = NULL;
		pfnPush = NULL;
		pPushParam = NULL;
	}
};

// 异步调用上下文负责把 IceRPCPush 回调桥接为本库 C 回调。
struct ST_ASYNC_CONTEXT
{
	PFN_CLOUD_NET_ASYNC pfnCallback;   // 调用方异步回调。
	void* pParam;                      // 调用方透传参数。

	ST_ASYNC_CONTEXT()
	{
		pfnCallback = NULL;
		pParam = NULL;
	}
};

long SafeBufferLen(long p_lLen)
{
	if (p_lLen < 0)
	{
		return 0;
	}
	return p_lLen;
}

void SafeCopyError(char* p_szDest, size_t p_uDestLen, const char* p_szSrc)
{
	if (!p_szDest || p_uDestLen == 0)
	{
		return;
	}
	p_szDest[0] = '\0';
	if (!p_szSrc)
	{
		return;
	}
	strncpy_s(p_szDest, p_uDestLen, p_szSrc, _TRUNCATE);
}

bool CopyBuffer(ST_CLOUD_NET_BUFFER& p_refDest, const unsigned char* p_pSrc, long p_lLen)
{
	p_refDest.lLen = 0;
	p_refDest.pBuffer = NULL;
	long lLen = SafeBufferLen(p_lLen);
	if (lLen <= 0)
	{
		return true;
	}
	if (!p_pSrc)
	{
		return false;
	}
	p_refDest.pBuffer = new (std::nothrow) unsigned char[static_cast<size_t>(lLen)];
	if (!p_refDest.pBuffer)
	{
		return false;
	}
	memcpy_s(p_refDest.pBuffer, static_cast<size_t>(lLen), p_pSrc, static_cast<size_t>(lLen));
	p_refDest.lLen = lLen;
	return true;
}

void ReleaseBuffer(ST_CLOUD_NET_BUFFER& p_refBuffer)
{
	delete[] p_refBuffer.pBuffer;
	p_refBuffer.pBuffer = NULL;
	p_refBuffer.lLen = 0;
}

void ReleaseResult(ST_CLOUD_NET_RESULT* p_pResult)
{
	if (!p_pResult)
	{
		return;
	}
	ReleaseBuffer(p_pResult->stLParam);
	ReleaseBuffer(p_pResult->stWParam);
	ReleaseBuffer(p_pResult->stJsonReq);
	delete p_pResult;
}

ST_CLOUD_NET_RESULT* CopyResult(const ST_JSON_M_RESULT_TOP* p_pSrc)
{
	if (!p_pSrc)
	{
		return NULL;
	}
	ST_CLOUD_NET_RESULT* pResult = new (std::nothrow) ST_CLOUD_NET_RESULT;
	if (!pResult)
	{
		return NULL;
	}
	memset(pResult, 0, sizeof(ST_CLOUD_NET_RESULT));
	pResult->lRetVal = p_pSrc->lRetVal;
	pResult->lParam = p_pSrc->lParam;
	pResult->wParam = p_pSrc->wParam;
	pResult->lSynId = p_pSrc->lSynId;
	pResult->lFuncId = p_pSrc->lFuncId;
	pResult->pParam = p_pSrc->pParam;
	SafeCopyError(pResult->szErrInfo, sizeof(pResult->szErrInfo), p_pSrc->szErrInfo);
	if (!CopyBuffer(pResult->stLParam, p_pSrc->stLParam.pBuffer, p_pSrc->stLParam.lLen))
	{
		ReleaseResult(pResult);
		return NULL;
	}
	if (!CopyBuffer(pResult->stWParam, p_pSrc->stWParam.pBuffer, p_pSrc->stWParam.lLen))
	{
		ReleaseResult(pResult);
		return NULL;
	}
	if (!CopyBuffer(pResult->stJsonReq, p_pSrc->stJsonReq.pBuffer, p_pSrc->stJsonReq.lLen))
	{
		ReleaseResult(pResult);
		return NULL;
	}
	return pResult;
}

void FillJsonResult(ST_JSON_M_RESULT_TOP& p_refDest, const ST_CLOUD_NET_RESULT* p_pSrc)
{
	memset(&p_refDest, 0, sizeof(ST_JSON_M_RESULT_TOP));
	if (!p_pSrc)
	{
		return;
	}
	p_refDest.lRetVal = p_pSrc->lRetVal;
	p_refDest.lParam = p_pSrc->lParam;
	p_refDest.wParam = p_pSrc->wParam;
	p_refDest.lSynId = p_pSrc->lSynId;
	p_refDest.lFuncId = p_pSrc->lFuncId;
	p_refDest.pParam = p_pSrc->pParam;
	SafeCopyError(p_refDest.szErrInfo, sizeof(p_refDest.szErrInfo), p_pSrc->szErrInfo);
	p_refDest.stLParam.lLen = static_cast<int>(SafeBufferLen(p_pSrc->stLParam.lLen));
	p_refDest.stLParam.pBuffer = p_pSrc->stLParam.pBuffer;
	p_refDest.stWParam.lLen = static_cast<int>(SafeBufferLen(p_pSrc->stWParam.lLen));
	p_refDest.stWParam.pBuffer = p_pSrc->stWParam.pBuffer;
	p_refDest.stJsonReq.lLen = static_cast<int>(SafeBufferLen(p_pSrc->stJsonReq.lLen));
	p_refDest.stJsonReq.pBuffer = p_pSrc->stJsonReq.pBuffer;
}

ST_CLOUD_NET_REQUEST MakeRequest(short p_chMode, long long p_lSetCode, ST_JSON_M_RESULT_TOP* p_pResult, void* p_pParam)
{
	ST_CLOUD_NET_REQUEST stRequest;
	memset(&stRequest, 0, sizeof(ST_CLOUD_NET_REQUEST));
	if (!p_pResult)
	{
		return stRequest;
	}
	stRequest.lSynId = p_pResult->lSynId;
	stRequest.lFuncId = p_pResult->lFuncId;
	stRequest.lSetCode = p_lSetCode;
	stRequest.lParam = p_pResult->lParam;
	stRequest.wParam = p_pResult->wParam;
	stRequest.chMode = p_chMode;
	stRequest.stJsonReq.lLen = p_pResult->stJsonReq.lLen;
	stRequest.stJsonReq.pBuffer = p_pResult->stJsonReq.pBuffer;
	stRequest.stLParam.lLen = p_pResult->stLParam.lLen;
	stRequest.stLParam.pBuffer = p_pResult->stLParam.pBuffer;
	stRequest.stWParam.lLen = p_pResult->stWParam.lLen;
	stRequest.stWParam.pBuffer = p_pResult->stWParam.pBuffer;
	stRequest.pParam = p_pParam;
	return stRequest;
}

unsigned long __stdcall AsyncCallback(void* p_pParam)
{
	ST_JSON_M_RESULT_LEVEL* pJsonResult = reinterpret_cast<ST_JSON_M_RESULT_LEVEL*>(p_pParam);
	ST_ASYNC_CONTEXT* pContext = NULL;
	if (pJsonResult)
	{
		pContext = reinterpret_cast<ST_ASYNC_CONTEXT*>(pJsonResult->pParam);
	}
	if (pContext && pContext->pfnCallback)
	{
		ST_CLOUD_NET_RESULT* pResult = CopyResult(pJsonResult);
		if (pResult)
		{
			pResult->pParam = pContext->pParam;
			pContext->pfnCallback(pResult, pContext->pParam);
			ReleaseResult(pResult);
		}
	}
	if (pJsonResult)
	{
		IJsonMutiResultFree(pJsonResult);
	}
	delete pContext;
	return 0;
}

void PushCallback(long long p_lReqNo, const char* p_pBuf, int p_iLen, void* p_pParam)
{
	ST_CONNECTION_CONTEXT* pContext = reinterpret_cast<ST_CONNECTION_CONTEXT*>(p_pParam);
	if (!pContext || !pContext->pfnPush)
	{
		return;
	}
	pContext->pfnPush(p_lReqNo, p_pBuf, p_iLen, pContext->pPushParam);
}
}

// CloudNetDataApi 内部实现类，公共头只暴露不透明句柄。
class CCloudNetDataApi
{
public:
	CCloudNetDataApi()
	{
		m_hServer = NULL;
		m_hServerSem = NULL;
		m_pfnRequest = NULL;
		m_pRequestParam = NULL;
		m_strLastError.clear();
	}

	~CCloudNetDataApi()
	{
		StopService();
		std::lock_guard<std::mutex> clLock(m_clMutex);
		for (std::map<std::string, ST_CONNECTION_CONTEXT*>::iterator it = m_mapConnection.begin(); it != m_mapConnection.end(); ++it)
		{
			if (it->second && it->second->hClient)
			{
				DeleteJsonICERPC(it->second->hClient);
			}
			delete it->second;
		}
		m_mapConnection.clear();
	}

	int StartService(const char* p_szXmlPath, const char* p_szServiceName, int p_iSnappyCompress, PFN_CLOUD_NET_REQUEST p_pfnCallback, void* p_pParam)
	{
		if (!p_szXmlPath || !p_szServiceName || !p_pfnCallback)
		{
			SetLastError("StartService 参数无效");
			return s_iCloudNetInvalidParam;
		}
		StopService();
		HANDLE hSem = NULL;
		HANDLE hServer = CreateJsonICEServer(p_szXmlPath, p_szServiceName, hSem, p_iSnappyCompress != 0);
		if (!hServer)
		{
			SetLastError("创建 Ice 服务端失败");
			return s_iCloudNetCreateFailed;
		}
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			m_hServer = hServer;
			m_hServerSem = hSem;
			m_pfnRequest = p_pfnCallback;
			m_pRequestParam = p_pParam;
		}
		RegServerCallBackFunc(hServer, &CCloudNetDataApi::ServerCallback, this);
		SetLastError("");
		return s_iCloudNetOk;
	}

	int StopService()
	{
		HANDLE hServer = NULL;
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			hServer = m_hServer;
			m_hServer = NULL;
			m_hServerSem = NULL;
			m_pfnRequest = NULL;
			m_pRequestParam = NULL;
		}
		if (hServer)
		{
			DeleteJsonICERPC(hServer);
		}
		return s_iCloudNetOk;
	}

	int Connect(const char* p_szXmlPath, const char* p_szProxyName, const char* p_szConnName, int p_iThreadPool)
	{
		if (!p_szXmlPath || !p_szProxyName || !p_szConnName || p_szConnName[0] == '\0')
		{
			SetLastError("Connect 参数无效");
			return s_iCloudNetInvalidParam;
		}
		std::string strConnName = p_szConnName;
		Disconnect(p_szConnName);
		HANDLE hSem = NULL;
		HANDLE hClient = CreateJsonICEClient(p_szXmlPath, p_szProxyName, hSem, p_iThreadPool);
		if (!hClient)
		{
			SetLastError("创建 Ice 客户端失败");
			return s_iCloudNetCreateFailed;
		}
		ST_CONNECTION_CONTEXT* pContext = new (std::nothrow) ST_CONNECTION_CONTEXT;
		if (!pContext)
		{
			DeleteJsonICERPC(hClient);
			SetLastError("创建连接上下文失败");
			return s_iCloudNetCreateFailed;
		}
		pContext->hClient = hClient;
		pContext->strConnName = strConnName;
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			m_mapConnection[strConnName] = pContext;
		}
		SetLastError("");
		return s_iCloudNetOk;
	}

	int Disconnect(const char* p_szConnName)
	{
		if (!p_szConnName || p_szConnName[0] == '\0')
		{
			SetLastError("Disconnect 参数无效");
			return s_iCloudNetInvalidParam;
		}
		ST_CONNECTION_CONTEXT* pContext = NULL;
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			std::map<std::string, ST_CONNECTION_CONTEXT*>::iterator it = m_mapConnection.find(p_szConnName);
			if (it == m_mapConnection.end())
			{
				SetLastError("连接不存在");
				return s_iCloudNetNotFound;
			}
			pContext = it->second;
			m_mapConnection.erase(it);
		}
		if (pContext && pContext->hClient)
		{
			DeleteJsonICERPC(pContext->hClient);
		}
		delete pContext;
		SetLastError("");
		return s_iCloudNetOk;
	}

	int RegisterPush(const char* p_szConnName, const char* p_szPluginId, const char* p_szSubInfo, PFN_CLOUD_NET_PUSH p_pfnCallback, void* p_pParam)
	{
		if (!p_szPluginId || !p_pfnCallback)
		{
			SetLastError("RegisterPush 参数无效");
			return s_iCloudNetInvalidParam;
		}
		ST_CONNECTION_CONTEXT* pContext = FindConnection(p_szConnName);
		if (!pContext)
		{
			return s_iCloudNetNotFound;
		}
		pContext->pfnPush = p_pfnCallback;
		pContext->pPushParam = p_pParam;
		const char* pRet = RegisterJsonICEClient2(pContext->hClient, p_szPluginId, p_szSubInfo ? p_szSubInfo : "", PushCallback, 1, pContext);
		if (!pRet)
		{
			SetLastError("注册推送失败");
			return s_iCloudNetCallFailed;
		}
		pContext->strRegisterRet = pRet;
		SetLastError("");
		return s_iCloudNetOk;
	}

	int UnregisterPush(const char* p_szConnName, const char* p_szPluginId, const char* p_szSubInfo)
	{
		if (!p_szPluginId)
		{
			SetLastError("UnregisterPush 参数无效");
			return s_iCloudNetInvalidParam;
		}
		ST_CONNECTION_CONTEXT* pContext = FindConnection(p_szConnName);
		if (!pContext)
		{
			return s_iCloudNetNotFound;
		}
		const char* pRet = RegisterJsonICEClient2(pContext->hClient, p_szPluginId, p_szSubInfo ? p_szSubInfo : "", PushCallback, 0, pContext);
		if (!pRet)
		{
			SetLastError("注销推送失败");
			return s_iCloudNetCallFailed;
		}
		pContext->pfnPush = NULL;
		pContext->pPushParam = NULL;
		pContext->strRegisterRet = pRet;
		SetLastError("");
		return s_iCloudNetOk;
	}

	ST_CLOUD_NET_RESULT* CallSync(const char* p_szConnName, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lJsonLen)
	{
		ST_CONNECTION_CONTEXT* pContext = FindConnection(p_szConnName);
		if (!pContext)
		{
			return NULL;
		}
		ST_JSON_M_RESULT_LEVEL* pJsonResult = JsonBinClientRPC(pContext->hClient, p_lSynId, p_lFuncId, p_lSetCode, p_szJsonReq ? p_szJsonReq : "", SafeBufferLen(p_lJsonLen));
		ST_CLOUD_NET_RESULT* pResult = CopyResult(pJsonResult);
		if (pJsonResult)
		{
			IJsonMutiResultFree(pJsonResult);
		}
		if (!pResult)
		{
			SetLastError("同步 RPC 调用失败");
		}
		else
		{
			SetLastError("");
		}
		return pResult;
	}

	long long CallAsync(const char* p_szConnName, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lJsonLen, PFN_CLOUD_NET_ASYNC p_pfnCallback, void* p_pParam)
	{
		if (!p_pfnCallback)
		{
			SetLastError("CallAsync 回调为空");
			return s_iCloudNetInvalidParam;
		}
		ST_CONNECTION_CONTEXT* pContext = FindConnection(p_szConnName);
		if (!pContext)
		{
			return s_iCloudNetNotFound;
		}
		ST_ASYNC_CONTEXT* pAsync = CreateAsyncContext(p_pfnCallback, p_pParam);
		if (!pAsync)
		{
			return s_iCloudNetCreateFailed;
		}
		long long lRet = JsonBinClientRPC_async(pContext->hClient, p_lSynId, p_lFuncId, p_lSetCode, p_szJsonReq ? p_szJsonReq : "", SafeBufferLen(p_lJsonLen), AsyncCallback, pAsync);
		if (lRet <= 0)
		{
			SetLastError("异步 RPC 调用失败");
		}
		else
		{
			SetLastError("");
		}
		return lRet;
	}

	ST_CLOUD_NET_RESULT* PutSync(const char* p_szConnName, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lJsonLen, const ST_CLOUD_NET_RESULT* p_pPutData)
	{
		ST_CONNECTION_CONTEXT* pContext = FindConnection(p_szConnName);
		if (!pContext)
		{
			return NULL;
		}
		ST_JSON_M_RESULT_TOP stPutData;
		FillJsonResult(stPutData, p_pPutData);
		ST_JSON_M_RESULT_LEVEL* pJsonResult = JsonBinClientPUT(pContext->hClient, p_lSynId, p_lFuncId, p_lSetCode, p_szJsonReq ? p_szJsonReq : "", SafeBufferLen(p_lJsonLen), &stPutData);
		ST_CLOUD_NET_RESULT* pResult = CopyResult(pJsonResult);
		if (pJsonResult)
		{
			IJsonMutiResultFree(pJsonResult);
		}
		if (!pResult)
		{
			SetLastError("同步 PUT 调用失败");
		}
		else
		{
			SetLastError("");
		}
		return pResult;
	}

	long long PutAsync(const char* p_szConnName, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lJsonLen, const ST_CLOUD_NET_RESULT* p_pPutData, PFN_CLOUD_NET_ASYNC p_pfnCallback, void* p_pParam)
	{
		if (!p_pfnCallback)
		{
			SetLastError("PutAsync 回调为空");
			return s_iCloudNetInvalidParam;
		}
		ST_CONNECTION_CONTEXT* pContext = FindConnection(p_szConnName);
		if (!pContext)
		{
			return s_iCloudNetNotFound;
		}
		ST_ASYNC_CONTEXT* pAsync = CreateAsyncContext(p_pfnCallback, p_pParam);
		if (!pAsync)
		{
			return s_iCloudNetCreateFailed;
		}
		ST_JSON_M_RESULT_TOP stPutData;
		FillJsonResult(stPutData, p_pPutData);
		long long lRet = JsonBinClientPUT_async(pContext->hClient, p_lSynId, p_lFuncId, p_lSetCode, p_szJsonReq ? p_szJsonReq : "", SafeBufferLen(p_lJsonLen), &stPutData, AsyncCallback, pAsync);
		if (lRet <= 0)
		{
			SetLastError("异步 PUT 调用失败");
		}
		else
		{
			SetLastError("");
		}
		return lRet;
	}

	const char* GetLastErrorText() const
	{
		return m_strLastError.c_str();
	}

private:
	ST_CONNECTION_CONTEXT* FindConnection(const char* p_szConnName)
	{
		if (!p_szConnName || p_szConnName[0] == '\0')
		{
			SetLastError("连接名无效");
			return NULL;
		}
		std::lock_guard<std::mutex> clLock(m_clMutex);
		std::map<std::string, ST_CONNECTION_CONTEXT*>::iterator it = m_mapConnection.find(p_szConnName);
		if (it == m_mapConnection.end() || !it->second || !it->second->hClient)
		{
			SetLastError("连接不存在");
			return NULL;
		}
		return it->second;
	}

	ST_ASYNC_CONTEXT* CreateAsyncContext(PFN_CLOUD_NET_ASYNC p_pfnCallback, void* p_pParam)
	{
		ST_ASYNC_CONTEXT* pAsync = new (std::nothrow) ST_ASYNC_CONTEXT;
		if (!pAsync)
		{
			SetLastError("创建异步上下文失败");
			return NULL;
		}
		pAsync->pfnCallback = p_pfnCallback;
		pAsync->pParam = p_pParam;
		return pAsync;
	}

	void SetLastError(const char* p_szError)
	{
		std::lock_guard<std::mutex> clLock(m_clErrorMutex);
		m_strLastError = p_szError ? p_szError : "";
	}

	static void ServerCallback(void* p_pParam, short p_chMode, long long p_lSetCode, ST_JSON_M_RESULT_TOP* p_pResult)
	{
		CCloudNetDataApi* pThis = reinterpret_cast<CCloudNetDataApi*>(p_pParam);
		if (!pThis)
		{
			return;
		}
		pThis->HandleServerCallback(p_chMode, p_lSetCode, p_pResult);
	}

	void HandleServerCallback(short p_chMode, long long p_lSetCode, ST_JSON_M_RESULT_TOP* p_pResult)
	{
		PFN_CLOUD_NET_REQUEST pfnRequest = NULL;
		void* pRequestParam = NULL;
		HANDLE hServer = NULL;
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			pfnRequest = m_pfnRequest;
			pRequestParam = m_pRequestParam;
			hServer = m_hServer;
		}
		if (!pfnRequest || !p_pResult || !hServer)
		{
			return;
		}
		ST_CLOUD_NET_REQUEST stRequest = MakeRequest(p_chMode, p_lSetCode, p_pResult, pRequestParam);
		ST_CLOUD_NET_RESULT stResult;
		memset(&stResult, 0, sizeof(ST_CLOUD_NET_RESULT));
		stResult.lSynId = stRequest.lSynId;
		stResult.lFuncId = stRequest.lFuncId;
		pfnRequest(&stRequest, &stResult, pRequestParam);
		// 底层传入的 p_pResult 携带 Ice AMD 回包句柄，必须原对象回填后交回 JsonICEResponseData。
		FillJsonResult(*p_pResult, &stResult);
		JsonICEResponseData(hServer, p_pResult);
	}

private:
	mutable std::mutex m_clMutex;                              // 保护服务句柄和连接表。
	mutable std::mutex m_clErrorMutex;                         // 保护最后错误字符串。
	HANDLE m_hServer;                                          // 当前服务端句柄。
	HANDLE m_hServerSem;                                       // 底层服务端信号量句柄，暂由 IceRPCPush 管理。
	PFN_CLOUD_NET_REQUEST m_pfnRequest;                        // 服务端请求回调。
	void* m_pRequestParam;                                     // 服务端请求回调透传参数。
	std::map<std::string, ST_CONNECTION_CONTEXT*> m_mapConnection; // 客户端连接表。
	std::string m_strLastError;                                // 最近一次错误信息。
};

extern "C"
{
HCLOUD_NET_API Create()
{
	CCloudNetDataApi* pApi = new (std::nothrow) CCloudNetDataApi;
	return reinterpret_cast<HCLOUD_NET_API>(pApi);
}

void Destroy(HCLOUD_NET_API p_hApi)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	delete pApi;
}

int StartService(HCLOUD_NET_API p_hApi, const char* p_szXmlPath, const char* p_szServiceName, int p_iSnappyCompress, PFN_CLOUD_NET_REQUEST p_pfnCallback, void* p_pParam)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	if (!pApi)
	{
		return s_iCloudNetInvalidParam;
	}
	return pApi->StartService(p_szXmlPath, p_szServiceName, p_iSnappyCompress, p_pfnCallback, p_pParam);
}

int StopService(HCLOUD_NET_API p_hApi)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	if (!pApi)
	{
		return s_iCloudNetInvalidParam;
	}
	return pApi->StopService();
}

int Connect(HCLOUD_NET_API p_hApi, const char* p_szXmlPath, const char* p_szProxyName, const char* p_szConnName, int p_iThreadPool)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	if (!pApi)
	{
		return s_iCloudNetInvalidParam;
	}
	return pApi->Connect(p_szXmlPath, p_szProxyName, p_szConnName, p_iThreadPool);
}

int Disconnect(HCLOUD_NET_API p_hApi, const char* p_szConnName)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	if (!pApi)
	{
		return s_iCloudNetInvalidParam;
	}
	return pApi->Disconnect(p_szConnName);
}

int RegisterPush(HCLOUD_NET_API p_hApi, const char* p_szConnName, const char* p_szPluginId, const char* p_szSubInfo, PFN_CLOUD_NET_PUSH p_pfnCallback, void* p_pParam)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	if (!pApi)
	{
		return s_iCloudNetInvalidParam;
	}
	return pApi->RegisterPush(p_szConnName, p_szPluginId, p_szSubInfo, p_pfnCallback, p_pParam);
}

int UnregisterPush(HCLOUD_NET_API p_hApi, const char* p_szConnName, const char* p_szPluginId, const char* p_szSubInfo)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	if (!pApi)
	{
		return s_iCloudNetInvalidParam;
	}
	return pApi->UnregisterPush(p_szConnName, p_szPluginId, p_szSubInfo);
}

ST_CLOUD_NET_RESULT* CallSync(HCLOUD_NET_API p_hApi, const char* p_szConnName, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lJsonLen)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	if (!pApi)
	{
		return NULL;
	}
	return pApi->CallSync(p_szConnName, p_lSynId, p_lFuncId, p_lSetCode, p_szJsonReq, p_lJsonLen);
}

long long CallAsync(HCLOUD_NET_API p_hApi, const char* p_szConnName, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lJsonLen, PFN_CLOUD_NET_ASYNC p_pfnCallback, void* p_pParam)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	if (!pApi)
	{
		return s_iCloudNetInvalidParam;
	}
	return pApi->CallAsync(p_szConnName, p_lSynId, p_lFuncId, p_lSetCode, p_szJsonReq, p_lJsonLen, p_pfnCallback, p_pParam);
}

ST_CLOUD_NET_RESULT* PutSync(HCLOUD_NET_API p_hApi, const char* p_szConnName, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lJsonLen, const ST_CLOUD_NET_RESULT* p_pPutData)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	if (!pApi)
	{
		return NULL;
	}
	return pApi->PutSync(p_szConnName, p_lSynId, p_lFuncId, p_lSetCode, p_szJsonReq, p_lJsonLen, p_pPutData);
}

long long PutAsync(HCLOUD_NET_API p_hApi, const char* p_szConnName, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lJsonLen, const ST_CLOUD_NET_RESULT* p_pPutData, PFN_CLOUD_NET_ASYNC p_pfnCallback, void* p_pParam)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	if (!pApi)
	{
		return s_iCloudNetInvalidParam;
	}
	return pApi->PutAsync(p_szConnName, p_lSynId, p_lFuncId, p_lSetCode, p_szJsonReq, p_lJsonLen, p_pPutData, p_pfnCallback, p_pParam);
}

void FreeResult(ST_CLOUD_NET_RESULT* p_pResult)
{
	ReleaseResult(p_pResult);
}

const char* GetLastError(HCLOUD_NET_API p_hApi)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	if (!pApi)
	{
		return "CloudNetDataApi 句柄无效";
	}
	return pApi->GetLastErrorText();
}
}