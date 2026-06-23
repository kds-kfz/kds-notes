// IceRPCPush.cpp : 实现 IceRPCPush DLL 的导出入口。
// 旧版函数名和签名保持不变，导出名由 IceRPCPush.def 控制。
#include "publicfunc.h"
#include "CJsonBinRPCImp.h"
#include "vld.h"
#include "ClientThreadPool.h"
#include "Log.h"

/*
旧 properties 配置示例：
Markets=SZ,SH
MarketsNetID=JsonBinRPCSZSH
JsonBinRPCSZSH.Endpoints=tcp -z -h * -p 30001:udp -z -h * -p 30001
JsonBinRPCSZSH.Proxy=Corbastockio:tcp -h 192.168.0.156 -p 30001:udp -h 192.168.0.156 -p 30001
JsonBinRPCSZSH.Client.Endpoints=default -h localhost
注意：客户端注册推送后需要定时续约，避免服务端清理过期连接。

*/
namespace
{
const int s_iIceRPCPushOk = 0;
const int s_iIceRPCPushInvalidParam = -20001;
const int s_iIceRPCPushCreateFailed = -20002;
const int s_iIceRPCPushStateError = -20003;
const int s_iIceRPCPushCallFailed = -20004;
const int s_iIceRPCPushIceException = -20005;
const int s_iIceRPCPushStdException = -20006;
const int s_iIceRPCPushUnknownException = -20007;

struct ST_ICE_RPC_PUSH_CODE_MSG
{
	int iCode;                 // 错误码，0 表示成功，负数表示失败。
	const char* szCode;        // 错误码英文简称，便于调用方记录。
	const char* szMsg;         // 默认英文错误描述。
};

const ST_ICE_RPC_PUSH_CODE_MSG s_aIceRPCPushCodeMsg[] =
{
	{s_iIceRPCPushOk, "OK", "OK"},
	{s_iIceRPCPushInvalidParam, "INVALID_PARAM", "INVALID_PARAM: input parameter or handle is invalid"},
	{s_iIceRPCPushCreateFailed, "CREATE_FAILED", "CREATE_FAILED: IceRPCPush object creation failed"},
	{s_iIceRPCPushStateError, "STATE_ERROR", "STATE_ERROR: handle state does not allow this operation"},
	{s_iIceRPCPushCallFailed, "CALL_FAILED", "CALL_FAILED: IceRPCPush call failed"},
	{s_iIceRPCPushIceException, "ICE_EXCEPTION", "ICE_EXCEPTION: Ice runtime raised an exception"},
	{s_iIceRPCPushStdException, "STD_EXCEPTION", "STD_EXCEPTION: C++ standard exception was raised"},
	{s_iIceRPCPushUnknownException, "UNKNOWN_EXCEPTION", "UNKNOWN_EXCEPTION: unknown exception was raised"},
};

thread_local int s_iThreadLastErrorCode = s_iIceRPCPushOk;
thread_local std::string s_strThreadLastError = "OK";

const char* GetIceRPCPushErrorMsgInternal(int p_iErrorCode)
{
	for (size_t uIndex = 0; uIndex < sizeof(s_aIceRPCPushCodeMsg) / sizeof(s_aIceRPCPushCodeMsg[0]); ++uIndex)
	{
		if (s_aIceRPCPushCodeMsg[uIndex].iCode == p_iErrorCode)
		{
			return s_aIceRPCPushCodeMsg[uIndex].szMsg;
		}
	}
	return "UNKNOWN_ERROR: error code is not registered";
}

void WriteIceRPCPushErrorLog(const char* p_szFunction, int p_iErrorCode, const std::string& p_strDetail)
{
	if (p_iErrorCode == s_iIceRPCPushOk)
	{
		return;
	}
	CIceRPCPushLog::Instance().WriteLog("ERROR", p_szFunction != NULL ? p_szFunction : "IceRPCPush", "code=%d detail=%s", p_iErrorCode, p_strDetail.c_str());
}

void SetIceRPCPushLastError(HANDLE p_hHandle, int p_iErrorCode, const std::string& p_strDetail, const char* p_szFunction)
{
	std::string strDetail = p_strDetail.empty() ? GetIceRPCPushErrorMsgInternal(p_iErrorCode) : p_strDetail;
	s_iThreadLastErrorCode = p_iErrorCode;
	s_strThreadLastError = strDetail;
	JSONBINRPC::ST_JSON_BIN_HANDLE* pJh = reinterpret_cast<JSONBINRPC::ST_JSON_BIN_HANDLE*>(p_hHandle);
	if (pJh != NULL)
	{
		pJh->iLastErrorCode = p_iErrorCode;
		pJh->strLastError = strDetail;
	}
	WriteIceRPCPushErrorLog(p_szFunction, p_iErrorCode, strDetail);
}

std::string MakeIceExceptionDetail(const char* p_szFunction, const IceUtil::Exception& p_refException)
{
	char szWhat[1024] = {0};
	UTF82ASC(p_refException.what(), szWhat, sizeof(szWhat) - 1);
	std::string strDetail = "ICE_EXCEPTION: func=";
	strDetail += p_szFunction != NULL ? p_szFunction : "unknown";
	strDetail += ", ice=";
	strDetail += p_refException.ice_id();
	if (szWhat[0] != '\0')
	{
		strDetail += ", what=";
		strDetail += szWhat;
	}
	return strDetail;
}

std::string MakeStdExceptionDetail(const char* p_szFunction, const std::exception& p_refException)
{
	std::string strDetail = "STD_EXCEPTION: func=";
	strDetail += p_szFunction != NULL ? p_szFunction : "unknown";
	strDetail += ", what=";
	strDetail += p_refException.what();
	return strDetail;
}

std::string MakeUnknownExceptionDetail(const char* p_szFunction)
{
	std::string strDetail = "UNKNOWN_EXCEPTION: func=";
	strDetail += p_szFunction != NULL ? p_szFunction : "unknown";
	return strDetail;
}

void SetIceRPCPushOk(HANDLE p_hHandle, const char* p_szFunction)
{
	SetIceRPCPushLastError(p_hHandle, s_iIceRPCPushOk, GetIceRPCPushErrorMsgInternal(s_iIceRPCPushOk), p_szFunction);
}

void SetIceRPCPushInvalidParam(HANDLE p_hHandle, const char* p_szFunction, const char* p_szDetail)
{
	SetIceRPCPushLastError(p_hHandle, s_iIceRPCPushInvalidParam, p_szDetail != NULL ? p_szDetail : GetIceRPCPushErrorMsgInternal(s_iIceRPCPushInvalidParam), p_szFunction);
}

void SetIceRPCPushIceException(HANDLE p_hHandle, const char* p_szFunction, const IceUtil::Exception& p_refException)
{
	SetIceRPCPushLastError(p_hHandle, s_iIceRPCPushIceException, MakeIceExceptionDetail(p_szFunction, p_refException), p_szFunction);
}

void SetIceRPCPushStdException(HANDLE p_hHandle, const char* p_szFunction, const std::exception& p_refException)
{
	SetIceRPCPushLastError(p_hHandle, s_iIceRPCPushStdException, MakeStdExceptionDetail(p_szFunction, p_refException), p_szFunction);
}

void SetIceRPCPushUnknownException(HANDLE p_hHandle, const char* p_szFunction)
{
	SetIceRPCPushLastError(p_hHandle, s_iIceRPCPushUnknownException, MakeUnknownExceptionDetail(p_szFunction), p_szFunction);
}

void CopyErrInfo(char* p_szErrInfo, size_t p_uSize, const std::string& p_strDetail)
{
	if (p_szErrInfo == NULL || p_uSize == 0)
	{
		return;
	}
	p_szErrInfo[0] = '\0';
	strncpy(p_szErrInfo, p_strDetail.c_str(), p_uSize - 1);
	p_szErrInfo[p_uSize - 1] = '\0';
}
}
// 创建服务端实现并包装为对外 HANDLE，失败时释放初始化对象。
static HANDLE CreateJsonICEServerImpl(const char* p_szCfgFile, const char* p_szEndPointName, HANDLE& p_hSem, bool p_bSnappyCompress)
{
	JSONBINRPC::CJsonBinRPCImp* srv = new JSONBINRPC::CJsonBinRPCImp;
	if (srv->StartByServer(p_szCfgFile, p_szEndPointName, p_hSem, p_bSnappyCompress))
	{
		JSONBINRPC::ST_JSON_BIN_HANDLE* pJh = new JSONBINRPC::ST_JSON_BIN_HANDLE;
		pJh->iType = EN_JSON_HANDLE_RPC;
		pJh->pRpc = srv;
		return pJh;
	}
	else
		delete srv;
	return NULL;
}
// 注册为直接回调模式，适合低延迟场景；回调中必须尽快返回。
static void RegServerCallBackFuncImpl(HANDLE	p_hHandle, func_JsonICEServerCallBsack p_pfnCallback, void* p_pParam)
{
	JSONBINRPC::ST_JSON_BIN_HANDLE* pJh = (JSONBINRPC::ST_JSON_BIN_HANDLE*)p_hHandle;
	if (!pJh)
		return;
	pJh->pRpc->m_pfnServerCallback = p_pfnCallback;
	pJh->pRpc->m_pServerParam = p_pParam;
}
// 使用调用方传入的属性数组创建客户端，适合不落地配置文件的场景。
static HANDLE CreateJsonICEClient2Impl(int p_iNum, const char* p_pszPropertyKey[], const char* p_pszProperty[], const char* p_szProxyProperty, HANDLE& p_hSem, int	p_iThreadPool)
{
	JSONBINRPC::CJsonBinRPCImp* client = new JSONBINRPC::CJsonBinRPCImp;
	if (client->StartByClientWithProperty(p_iNum, p_pszPropertyKey, p_pszProperty, p_szProxyProperty, p_hSem))
	{
		JSONBINRPC::ST_JSON_BIN_HANDLE* pJh = new JSONBINRPC::ST_JSON_BIN_HANDLE;
		pJh->iType = EN_JSON_HANDLE_RPC;
		pJh->pRpc = client;
		return pJh;
	}
	else
		delete client;
	return NULL;
}
// 使用配置文件创建客户端，p_szProxyProperty 对应 Ice 的 Proxy 配置项。
static HANDLE CreateJsonICEClientImpl(const char* p_szCfgFile, const char* p_szProxyProperty, HANDLE& p_hSem, int	p_iThreadPool)
{
	JSONBINRPC::CJsonBinRPCImp* client = new JSONBINRPC::CJsonBinRPCImp;
	if (client->StartByClientWithLocator(p_szCfgFile, p_szProxyProperty, p_hSem))
	{
		JSONBINRPC::ST_JSON_BIN_HANDLE* pJh = new JSONBINRPC::ST_JSON_BIN_HANDLE;
		pJh->iType = EN_JSON_HANDLE_RPC;
		pJh->pRpc = client;
		return pJh;
	}
	else
		delete client;
	return NULL;
}
// 注册推送回调函数。
// 注册或注销客户端推送，客户端标识和回调由接口内部携带。
static HANDLE RegisterJsonICEClientImpl(HANDLE p_hHandle, const char* p_szGuid, func_JsonICEPushClientPack p_pfnCallback, int p_iIsReg/* =1 */, void* p_pParam)
{
	std::string	strret;
	JSONBINRPC::ST_JSON_BIN_HANDLE* pJh = (JSONBINRPC::ST_JSON_BIN_HANDLE*)p_hHandle;
	if (!pJh)
		return NULL;
	if (pJh->iType == EN_JSON_HANDLE_RPC)
	{
		JSONBINRPC::CJsonBinRPCImp* cli = pJh->pRpc->RegisterClient(strret, p_szGuid, NULL, p_pfnCallback, p_iIsReg, p_pParam);	// 返回原句柄，服务端通过该句柄继续 Pop 请求。
		return pJh;
	}
	else
		return NULL;	// 客户端子句柄不允许再次注册。		//((JSONBINRPC::CJsonBinRPCImp *)p_hHandle)->RegisterClient(p_szGuid,p_pfnCallback,p_iIsReg,p_pParam);
}
// 携带 subinfo 订阅信息，服务端可按订阅内容过滤推送。
// 注册或注销带订阅信息的客户端回调，返回服务端生成的推送配置。
static const char* RegisterJsonICEClient2Impl(HANDLE p_hHandle, const char* p_szGuid, const char* p_szSubInfo, func_JsonICEPushClientPack p_pfnCallback, int p_iIsReg, void* p_pParam)
{
	JSONBINRPC::ST_JSON_BIN_HANDLE* pJh = (JSONBINRPC::ST_JSON_BIN_HANDLE*)p_hHandle;
	if (!pJh)
		return NULL;
	std::string& strret = pJh->strRet;
	strret = "";
	if (pJh->iType == EN_JSON_HANDLE_RPC)
	{
		JSONBINRPC::CJsonBinRPCImp* cli = pJh->pRpc->RegisterClient(strret, p_szGuid, p_szSubInfo, p_pfnCallback, p_iIsReg, p_pParam);	// 返回原句柄，服务端通过该句柄继续 Pop 请求。
		return strret.c_str();
	}
	else
		return strret.c_str();
}
// 查询当前服务端记录的 endpoint 字符串，便于上层日志或诊断使用。
static const char* JsonGetEndPointImpl(HANDLE p_hHandle)
{
	JSONBINRPC::ST_JSON_BIN_HANDLE* pJh = (JSONBINRPC::ST_JSON_BIN_HANDLE*)p_hHandle;
	if (!pJh)
		return NULL;
	return pJh->pRpc->GetEndPoint();
}

// 服务端收到请求后从队列弹出节点，直回调模式不会进入队列。
// 上层处理完该节点后必须调用 JsonBinSrvComplete 完成回包和释放。
static ST_JSON_INPUT* JsonBinSrvPopfrontImpl(HANDLE p_hHandle)
{
	if (!p_hHandle)
		return NULL;
	ST_JSON_INPUT_EX* pNode = NULL;
	JSONBINRPC::ST_JSON_BIN_HANDLE* pJh = (JSONBINRPC::ST_JSON_BIN_HANDLE*)p_hHandle;
	pNode = pJh->pRpc->Req().PopFront();
	return pNode;
}
// 管理并释放队列节点 pNode，但不接管上层 result 内存。
// 没有返回数据时也必须走完成流程，保证内部节点被释放。
// 业务处理完成后发送回包并释放队列节点，result 生命周期仍归调用方。
static void JsonBinSrvCompleteImpl(ST_JSON_INPUT* p_pNodeOrg, ST_JSON_M_RESULT_TOP* p_pResult)
{
	//if ( !p_pNodeOrg || !result )	// 无结果时也需要避免节点泄漏。
	if (!p_pNodeOrg)
		return;
	ST_JSON_INPUT_EX* pNode = (ST_JSON_INPUT_EX*)p_pNodeOrg;
	::JSONBINRPC::AByte	stLParam, stWParam;
	// 回包前复制并压缩二进制参数，避免引用上层临时缓冲区。
	if (p_pResult && p_pResult->stLParam.lLen > 0)
	{
		stLParam.resize(p_pResult->stLParam.lLen);
		memcpy(&*stLParam.begin(), p_pResult->stLParam.pBuffer, p_pResult->stLParam.lLen);
		CompressAByte(stLParam, (const char*)&*stLParam.begin(), SafeSizeToLength<long>(stLParam.size()));
	}
	if (p_pResult && p_pResult->stWParam.lLen > 0)
	{
		stWParam.resize(p_pResult->stWParam.lLen);
		memcpy(&*stWParam.begin(), p_pResult->stWParam.pBuffer, p_pResult->stWParam.lLen);
		CompressAByte(stWParam, (const char*)&*stWParam.begin(), SafeSizeToLength<long>(stWParam.size()));
	}
	try
	{
		long long		lRetVal = (p_pResult ? p_pResult->lRetVal : 0);
		long long		lParam = (p_pResult ? p_pResult->lParam : 0);
		long long		wParam = (p_pResult ? p_pResult->wParam : 0);
		const char* szErrInfo = (p_pResult ? p_pResult->szErrInfo : "");
		if (pNode->chMode == EN_JSON_INPUT_RPC)
		{
			pNode->pRpcCallback->ice_response(lRetVal, lParam, stLParam, wParam, stWParam, szErrInfo);
		}
		else if (pNode->chMode == EN_JSON_INPUT_PUT)
		{
			pNode->pPutCallback->ice_response(lRetVal, lParam, stLParam, wParam, stWParam, szErrInfo);
		}
	}
	catch (const IceUtil::Exception& ex)
	{
		char	str[1024] = { 0 };
		UTF82ASC(ex.what(), str, 1023);
		SetIceRPCPushIceException(pNode != NULL ? pNode->hSelf : NULL, __FUNCTION__, ex);
	}
	if (pNode->stJsonReq.pBuffer)
	{
		delete[] pNode->stJsonReq.pBuffer;
		pNode->stJsonReq.pBuffer = NULL;
	}
	if (pNode->stLParam.pBuffer)
	{
		delete[] pNode->stLParam.pBuffer;
		pNode->stLParam.pBuffer = NULL;
	}
	if (pNode->stWParam.pBuffer)
	{
		delete[] pNode->stWParam.pBuffer;
		pNode->stWParam.pBuffer = NULL;
	}
	delete	pNode;
}
// 服务端主动推送数据，可按订阅过滤后发送给客户端。
// 返回当前推送拥塞计数，便于上层限流。
static long long PushJsonICEServerDataImpl(HANDLE p_hHandle, int p_lReqNo, const char* p_pBuf, long p_lBufLen, bool p_bAsync)
{
	if (!p_hHandle)
		return 0;
	JSONBINRPC::ST_JSON_BIN_HANDLE* pJh = (JSONBINRPC::ST_JSON_BIN_HANDLE*)p_hHandle;
	return pJh->pRpc->ProcessPackage(p_lReqNo, p_pBuf, p_lBufLen, p_bAsync);
}
// 释放客户端同步、异步或 Pre/End 返回的结果节点。
// 只释放 DLL 底层创建的结果节点，上层栈对象或自分配对象不能传入。
static void IJsonMutiResultFreeImpl(ST_JSON_M_RESULT_LEVEL* p_pResultOrg)
{
	if (!p_pResultOrg)
		return;
	ST_JSON_MULTI_RESULT_EX* pResult = (ST_JSON_MULTI_RESULT_EX*)p_pResultOrg;
	// 结果内部缓冲区由 AByte 成员托管，此处只释放结果对象。
// 	if ( pResult->stLParam.pBuffer )
// 	{
// 		delete [] pResult->stLParam.pBuffer;
// 		pResult->stLParam.pBuffer = NULL;
// 	}
// 	if ( pResult->stWParam.pBuffer )
// 	{
// 		delete [] pResult->stWParam.pBuffer;
// 		pResult->stWParam.pBuffer = NULL;
// 	}
	// 历史 ResultPtr 不再手动删除，避免破坏智能指针计数。
	//if ( pResult->pResult )
	//{
	//	delete ((::Ice::AsyncResultPtr *)pResult->pResult);	// 该写法会破坏智能指针生命周期，可能导致内存泄漏。
	//}
	delete pResult;
}

// 压缩 json 请求后调用远端 RPC，返回二进制参数再解压。
// 返回结果必须由 IJsonMutiResultFree 释放。
// 阻塞式 RPC 调用适合低频请求，高频请求建议使用异步或 Pre/End。
static ST_JSON_M_RESULT_LEVEL* JsonBinClientRPCImpl(HANDLE p_hHandle, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lBufLen)
{
	if (!p_hHandle)
		return NULL;
	JSONBINRPC::ST_JSON_BIN_HANDLE* pJh = (JSONBINRPC::ST_JSON_BIN_HANDLE*)p_hHandle;
	::std::string	errinf;
	//::JSONBINRPC::AByte	stLParam,stWParam;
	ST_JSON_MULTI_RESULT_EX* pResult = new ST_JSON_MULTI_RESULT_EX;
	pResult->lSynId = p_lSynId;
	pResult->lFuncId = p_lFuncId;
	//	pResult->aReqJson	= p_szJsonReq;
	//	pResult->stJsonReq		= pResult->aReqJson.c_str();

	CompressAByte(pResult->aReqJson, p_szJsonReq, p_lBufLen);

	//pResult->aReqJson.resize(p_lBufLen);
	//memcpy(&*pResult->aReqJson.begin(),p_szJsonReq,p_lBufLen);
	// 保存原始请求信息，便于回调时回传给上层识别。
	pResult->stJsonReq.lLen = SafeSizeToLength<int>(pResult->aReqJson.size());
	pResult->stJsonReq.pBuffer = (unsigned char*)&*pResult->aReqJson.begin();
	try
	{
		pResult->lRetVal = pJh->ClientIO()->JsonBinRPC(p_lSynId, p_lFuncId, p_lSetCode, pResult->aReqJson,
			pResult->lParam, pResult->aLParam,
			pResult->wParam, pResult->aWParam,
			errinf);
	}
	catch (const IceUtil::Exception& ex)
	{
		pResult->lRetVal = -1;
		char	str[1024] = { 0 };
		UTF82ASC(ex.what(), str, 1023);

		CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "%m_hSocket , %m_hSocket\r\n", ex.ice_id(), str);//ex.what());
		errinf = ex.ice_id();
		errinf += ex.what();
	}
	strncpy(pResult->szErrInfo, errinf.c_str(), sizeof(pResult->szErrInfo) - 1);

	size_t ulength;
	char* uncompressbuffer = NULL;
	if (pResult->aLParam.size() > 0)
	{
		ulength = UnCompressAByte(pResult->aLParam, (const char*)pResult->aLParam.data(), SafeSizeToLength<long>(pResult->aLParam.size()));

		pResult->stLParam.lLen = SafeSizeToLength<int>(ulength);
		pResult->stLParam.pBuffer = (unsigned char*)pResult->aLParam.data();//new char[pResult->stLParam.lLen];

		//pResult->stLParam.lLen		= pResult->aLParam.size();
		//pResult->stLParam.pBuffer	= (unsigned char*)pResult->aLParam.data();//new char[pResult->stLParam.lLen];
		//memcpy(pResult->stLParam.pBuffer,&*stLParam.begin(),pResult->stLParam.lLen);
	}
	if (pResult->aWParam.size() > 0)
	{
		ulength = UnCompressAByte(pResult->aWParam, (const char*)pResult->aWParam.data(), SafeSizeToLength<long>(pResult->aWParam.size()));

		pResult->stWParam.lLen = SafeSizeToLength<int>(pResult->aWParam.size());
		pResult->stWParam.pBuffer = (unsigned char*)pResult->aWParam.data();// char[pResult->stWParam.lLen];
		//memcpy(pResult->stWParam.pBuffer,&*stWParam.begin(),pResult->stWParam.lLen);
	}

	return pResult;
}


// p_pfnCallback(&result);
// 异步调用前复制请求缓冲区，避免上层提前释放。
// 异步 RPC 调用，结果通过调用方线程回调返回并由上层释放。
static long long JsonBinClientRPCAsyncImpl(HANDLE p_hHandle, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lBufLen, LPTHREAD_START_ROUTINE p_pfnCallbackClient, void* p_pParam)
{
	if (!p_hHandle)
		return 0;

	long long		iret = 0;
	JSONBINRPC::ST_JSON_BIN_HANDLE* pJh = (JSONBINRPC::ST_JSON_BIN_HANDLE*)p_hHandle;

	//LARGE_INTEGER				T[4];
	//memcpy(&T,p_szJsonReq,4*sizeof(LARGE_INTEGER));
	try
	{	// 构造异步调用的回调对象。
		JsonBinRPCCallBackPtr	cb = std::make_shared<CJsonBinRPCCallBack>();
		cb->m_dwCallTimes = 0;	// 调用次数。
		cb->m_pfnCallbackClient = p_pfnCallbackClient;	// 外部回调函数，原样保存。
		cb->m_pParam = p_pParam;
		cb->m_lSynId = p_lSynId;
		cb->m_lFuncId = p_lFuncId;
		//cb->m_stJsonReq		   = p_szJsonReq;
		//cb->m_stJsonReq.resize(p_lBufLen);
		//memcpy(&*cb->m_stJsonReq.begin(),p_szJsonReq,p_lBufLen);

		size_t  output_length = CompressAByte(cb->m_stJsonReq, p_szJsonReq, p_lBufLen);

		iret = InterlockedIncrement64(&g_lCrowded);

		JSONBINRPC::Callback_IJsonBinRPC_JsonBinRPCPtr<CJsonBinRPCCallBack>	ProcessCB =
			JSONBINRPC::newCallback_IJsonBinRPC_JsonBinRPC(cb,
				&CJsonBinRPCCallBack::ice_response,
				&CJsonBinRPCCallBack::exception,
				&CJsonBinRPCCallBack::sent);

		// 预留的微秒级计时探针。
		//QueryPerformanceCounter(&T[1]);
		//memcpy((char*)p_szJsonReq,&T,4*sizeof(LARGE_INTEGER));
		//memcpy(&*cb->m_stJsonReq.begin(),p_szJsonReq,p_lBufLen);
		// 不保存 AsyncResult 到回调对象，避免破坏回调智能指针生命周期。
		Ice::AsyncResultPtr r = icecompat::begin_JsonBinRPC(pJh->ClientIO(), p_lSynId, p_lFuncId, p_lSetCode, cb->m_stJsonReq, ProcessCB);
		// 等待完成会影响吞吐，这里保留旧行为以兼容调用方时序。
		// void waitForSent() This method m_chBlocks the calling thread until a request has been written to the client-side transport, or an exception occurs. After wait
		// ForSent returns, isSent returns true if the request was successfully written to the client-side transport, or false if an exception
		// occurred. In the case of a failure, you can call the corresponding end_ method or throwLocalException to obtain the exception.
		// r->waitForSent();
		// void waitForCompleted()
		// This method m_chBlocks the caller until the result of an invocation becomes available.
		if (pJh->pRpc->IsAsyncWaitCompleted())
		{
			r->waitForCompleted();
		}
		// ((JSONBINRPC::CJsonBinRPCImp *)p_hHandle)->ClientIO()->end_JsonBinRPC()
		// iret = (long long)cb->m_pResult.get();
	}
	catch (const IceUtil::Exception& ex)
	{
		std::string strDetail = MakeIceExceptionDetail(__FUNCTION__, ex);
		SetIceRPCPushLastError(p_hHandle, s_iIceRPCPushIceException, strDetail, __FUNCTION__);

		ST_JSON_MULTI_RESULT_EX* pResult = new ST_JSON_MULTI_RESULT_EX;
		pResult->lRetVal = -1;
		pResult->lSynId = p_lSynId;
		pResult->lFuncId = p_lFuncId;
		//pResult->aReqJson	= p_szJsonReq;
		//pResult->stJsonReq		= pResult->aReqJson.c_str();
		pResult->aReqJson.resize(p_lBufLen);
		memcpy(&*pResult->aReqJson.begin(), p_szJsonReq, p_lBufLen);
		// 保存原始请求信息，便于回调时回传给上层识别。
		pResult->stJsonReq.lLen = p_lBufLen;
		pResult->stJsonReq.pBuffer = (unsigned char*)&*pResult->aReqJson.begin();
		pResult->pParam = p_pParam;
		strncpy(pResult->szErrInfo, strDetail.c_str(), sizeof(pResult->szErrInfo) - 1);

		try
		{
			p_pfnCallbackClient(pResult);
		}
		catch (const std::exception& cbEx)
		{
			SetIceRPCPushStdException(p_hHandle, "JsonBinClientRPC_async.callback", cbEx);
		}
		catch (...)
		{
			SetIceRPCPushUnknownException(p_hHandle, "JsonBinClientRPC_async.callback");
		}

		InterlockedDecrement64(&g_lCrowded);
	}
	return iret;
}
/*
FileHandle file = open(...);
FileTransferPrx ft = ...;
const int chunkSize = ...;
Ice::Int offset = 0;
list<Ice::AsyncResultPtr> results;
const int numRequests = 5;
while (!file.eof()) {
	ByteSeq bs;
	bs = file.read(chunkSize);
	// Send up to numRequests + 1 chunks asynchronously.
	Ice::AsyncResultPtr r = ft->begin_send(offset, bs);
	offset += bs.size();
	// Wait until this request has been passed to the transport.
	r->waitForSent();
	results.push_back(r);
	// Once there are more than numRequests, wait for the least
	// recent one to complete.
	while (results.size() > numRequests) {
		Ice::AsyncResultPtr r = results.front();
		results.pop_front();
		r->waitForCompleted();
	}
}
// Wait for any remaining requests to complete.
while (!results.empty()) {
	Ice::AsyncResultPtr r = results.front();
	results.pop_front();
	r->waitForCompleted();
}
*/
// Pre/End RPC 的发送阶段，返回节点必须传给 EndPreJsonBinClientRPC。
static ST_JSON_M_RESULT_LEVEL* PreJsonBinClientRPCImpl(HANDLE p_hHandle, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lBufLen)
{
	if (!p_hHandle)
		return NULL;
	JSONBINRPC::ST_JSON_BIN_HANDLE* pJh = (JSONBINRPC::ST_JSON_BIN_HANDLE*)p_hHandle;
	ST_JSON_MULTI_RESULT_EX* pResult = new ST_JSON_MULTI_RESULT_EX;
	pResult->lSynId = p_lSynId;

	pResult->lSynId = p_lSynId;
	pResult->lFuncId = p_lFuncId;
	//pResult->aReqJson	= p_szJsonReq;
	//pResult->stJsonReq		= pResult->aReqJson.c_str();
	::JSONBINRPC::AByte							tmpreqjsonReq;
	size_t  output_length = CompressAByte(tmpreqjsonReq, p_szJsonReq, p_lBufLen);

	pResult->aReqJson.resize(p_lBufLen);
	memcpy(&*pResult->aReqJson.begin(), p_szJsonReq, p_lBufLen);

	pResult->stJsonReq.lLen = p_lBufLen;
	pResult->stJsonReq.pBuffer = (unsigned char*)&*pResult->aReqJson.begin();
	InterlockedIncrement64(&g_lCrowded);
	try
	{
		pResult->pResult = icecompat::begin_JsonBinRPC(pJh->ClientIO(), p_lSynId, p_lFuncId, p_lSetCode, tmpreqjsonReq);

	}
	catch (const IceUtil::Exception& ex)
	{
		pResult->lRetVal = -1;
		std::string strDetail = MakeIceExceptionDetail(__FUNCTION__, ex);
		SetIceRPCPushLastError(p_hHandle, s_iIceRPCPushIceException, strDetail, __FUNCTION__);
		strncpy(pResult->szErrInfo, strDetail.c_str(), sizeof(pResult->szErrInfo) - 1);
	}
	return pResult;
}
// Pre/End RPC 的收尾阶段，等待远端结果并填充同一个结果节点。
static long long EndPreJsonBinClientRPCImpl(HANDLE p_hHandle, ST_JSON_M_RESULT_LEVEL* p_pResultOrg)
{
	if (!p_hHandle)
		return 0;
	JSONBINRPC::ST_JSON_BIN_HANDLE* pJh = (JSONBINRPC::ST_JSON_BIN_HANDLE*)p_hHandle;
	ST_JSON_MULTI_RESULT_EX* pResult = (ST_JSON_MULTI_RESULT_EX*)p_pResultOrg;

	size_t	ulength = 0;
	InterlockedDecrement64(&g_lCrowded);
	try
	{
		//::JSONBINRPC::AByte	stLParam,stWParam;
		::std::string	errinf;
		pResult->lRetVal = icecompat::end_JsonBinRPC(pResult->lParam, pResult->aLParam, pResult->wParam, pResult->aWParam, errinf, pResult->pResult);
		strncpy(pResult->szErrInfo, errinf.c_str(), sizeof(pResult->szErrInfo) - 1);
		if (pResult->aLParam.size() > 0)
		{
			ulength = UnCompressAByte(pResult->aLParam, (const char*)pResult->aLParam.data(), SafeSizeToLength<long>(pResult->aLParam.size()));

			pResult->stLParam.lLen = SafeSizeToLength<int>(pResult->aLParam.size());
			pResult->stLParam.pBuffer = (unsigned char*)pResult->aLParam.data();////new char[pResult->stLParam.lLen];
			//memcpy(pResult->stLParam.pBuffer,&*stLParam.begin(),pResult->stLParam.lLen);
		}
		if (pResult->aWParam.size() > 0)
		{
			ulength = UnCompressAByte(pResult->aWParam, (const char*)pResult->aWParam.data(), SafeSizeToLength<long>(pResult->aWParam.size()));

			pResult->stWParam.lLen = SafeSizeToLength<int>(pResult->aWParam.size());
			pResult->stWParam.pBuffer = (unsigned char*)pResult->aWParam.data();//new char[pResult->stWParam.lLen];
			//memcpy(pResult->stWParam.pBuffer,&*stWParam.begin(),pResult->stWParam.lLen);
		}
	}
	catch (const IceUtil::Exception& ex)
	{
		pResult->lRetVal = -1;
		std::string strDetail = MakeIceExceptionDetail(__FUNCTION__, ex);
		SetIceRPCPushLastError(p_hHandle, s_iIceRPCPushIceException, strDetail, __FUNCTION__);
		strncpy(pResult->szErrInfo, strDetail.c_str(), sizeof(pResult->szErrInfo) - 1);
		return 0;
	}
	return pResult->lRetVal;
}
// 客户端上传后需要自行释放返回的 IJsonMutiResult 结果。
// 阻塞式 PUT 调用，会携带描述 json 和二进制参数上传到服务端。
static ST_JSON_M_RESULT_LEVEL* JsonBinClientPUTImpl(HANDLE p_hHandle, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lBufLen, const ST_JSON_M_RESULT_TOP* p_pPutData)
{
	if (!p_hHandle)
		return 0;
	::std::string			errinf;
	//::JSONBINRPC::AByte	oslparam,oswparam;
	JSONBINRPC::ST_JSON_BIN_HANDLE* pJh = (JSONBINRPC::ST_JSON_BIN_HANDLE*)p_hHandle;
	ST_JSON_MULTI_RESULT_EX* pResult = new ST_JSON_MULTI_RESULT_EX;
	pResult->lSynId = p_lSynId;
	pResult->lFuncId = p_lFuncId;

	size_t  output_length = CompressAByte(pResult->aReqJson, p_szJsonReq, p_lBufLen);

	//pResult->aReqJson.resize(p_lBufLen);
	//memcpy(&*pResult->aReqJson.begin(),p_szJsonReq,p_lBufLen);
	// 保存原始请求信息，便于回调时回传给上层识别。
	pResult->stJsonReq.lLen = p_lBufLen;
	pResult->stJsonReq.pBuffer = (unsigned char*)&*pResult->aReqJson.begin();

	// 	p_pPutData->lSynId	= p_lSynId;
	// 	p_pPutData->lFuncId= p_lFuncId;
	// 	p_pPutData->stJsonReq= pResult->stJsonReq;
	::JSONBINRPC::AByte	stLParam, stWParam;
	if (p_pPutData->stLParam.lLen > 0)
	{
		stLParam.resize(p_pPutData->stLParam.lLen);
		memcpy(&*stLParam.begin(), p_pPutData->stLParam.pBuffer, p_pPutData->stLParam.lLen);

		CompressAByte(stLParam, (const char*)&*stLParam.begin(), SafeSizeToLength<long>(stLParam.size()));
	}
	if (p_pPutData->stWParam.lLen > 0)
	{
		stWParam.resize(p_pPutData->stWParam.lLen);
		memcpy(&*stWParam.begin(), p_pPutData->stWParam.pBuffer, p_pPutData->stWParam.lLen);

		CompressAByte(stWParam, (const char*)&*stWParam.begin(), SafeSizeToLength<long>(stWParam.size()));
	}
	try
	{
		//pResult->lRetVal = pJh->ClientIO()->JsonBinPUT(p_lSynId,p_lFuncId,p_lSetCode,pResult->aReqJson,p_pPutData->lParam,stLParam,p_pPutData->wParam,stWParam,
		//	pResult->lParam,oslparam,pResult->wParam,oswparam,errinf);
		pResult->lRetVal = pJh->ClientIO()->JsonBinPUT(p_lSynId, p_lFuncId, p_lSetCode, pResult->aReqJson, p_pPutData->lParam, stLParam, p_pPutData->wParam, stWParam,
			pResult->lParam, pResult->aLParam, pResult->wParam, pResult->aWParam, errinf);
	}
	catch (const IceUtil::Exception& ex)
	{
		pResult->lRetVal = -1;
		std::string strDetail = MakeIceExceptionDetail(__FUNCTION__, ex);
		SetIceRPCPushLastError(p_hHandle, s_iIceRPCPushIceException, strDetail, __FUNCTION__);
		errinf = strDetail;
		strncpy(pResult->szErrInfo, strDetail.c_str(), sizeof(pResult->szErrInfo) - 1);
		//return -1;
	}
	strncpy(pResult->szErrInfo, errinf.c_str(), sizeof(pResult->szErrInfo) - 1);
	if (pResult->aLParam.size() > 0)
	{
		UnCompressAByte(pResult->aLParam, (const char*)pResult->aLParam.data(), SafeSizeToLength<long>(pResult->aLParam.size()));
		pResult->stLParam.lLen = SafeSizeToLength<int>(pResult->aLParam.size());
		pResult->stLParam.pBuffer = (unsigned char*)pResult->aLParam.data();//new char[pResult->stLParam.lLen];
		//memcpy(pResult->stLParam.pBuffer,&*oslparam.begin(),pResult->stLParam.lLen);
	}
	if (pResult->aWParam.size() > 0)
	{
		UnCompressAByte(pResult->aWParam, (const char*)pResult->aWParam.data(), SafeSizeToLength<long>(pResult->aWParam.size()));
		pResult->stWParam.lLen = SafeSizeToLength<int>(pResult->aWParam.size());
		pResult->stWParam.pBuffer = (unsigned char*)pResult->aWParam.data();//new char[pResult->stWParam.lLen];
		//memcpy(pResult->stWParam.pBuffer,&*oswparam.begin(),pResult->stWParam.lLen);
	}
	return pResult;//p_pPutData->lRetVal;
}
// 客户端上传后需要自行释放返回的 IJsonMutiResult 结果。
// 异步 PUT 调用，发送前会复制并压缩输入缓冲区，上层可提前释放原始数据。
static long long JsonBinClientPUTAsyncImpl(HANDLE p_hHandle, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lBufLen, const ST_JSON_M_RESULT_TOP* p_pPutData, LPTHREAD_START_ROUTINE p_pfnPutCallback, void* p_pParam)
{
	if (!p_hHandle)
		return 0;

	long long		iret = 0;
	JSONBINRPC::ST_JSON_BIN_HANDLE* pJh = (JSONBINRPC::ST_JSON_BIN_HANDLE*)p_hHandle;
	try
	{
		JsonBinPUTCallBackPtr	cb = std::make_shared<CJsonBinRPCCallBack>();
		cb->m_dwCallTimes = 0;	// 调用次数。
		cb->m_pfnCallbackClient = p_pfnPutCallback;
		cb->m_pParam = p_pParam;
		cb->m_lSynId = p_lSynId;
		cb->m_lFuncId = p_lFuncId;

		size_t  output_length = CompressAByte(cb->m_stJsonReq, p_szJsonReq, p_lBufLen);
		//cb->m_stJsonReq.resize(p_lBufLen);
		//memcpy(&*cb->m_stJsonReq.begin(),p_szJsonReq,p_lBufLen);
		// p_pPutData 可能指向临时对象，不能修改其内部指针，避免异步期间失效。
		//p_pPutData->stJsonReq.lLen	= p_lBufLen;
		//p_pPutData->stJsonReq.pBuffer	= (char*)cb->m_stJsonReq.begin();
		iret = InterlockedIncrement64(&g_lCrowded);

		::JSONBINRPC::AByte	stLParam, stWParam;
		if (p_pPutData->stLParam.lLen > 0)
		{
			stLParam.resize(p_pPutData->stLParam.lLen);
			memcpy(&*stLParam.begin(), p_pPutData->stLParam.pBuffer, p_pPutData->stLParam.lLen);
			CompressAByte(stLParam, (const char*)&*stLParam.begin(), SafeSizeToLength<long>(stLParam.size()));
		}
		if (p_pPutData->stWParam.lLen > 0)
		{
			stWParam.resize(p_pPutData->stWParam.lLen);
			memcpy(&*stWParam.begin(), p_pPutData->stWParam.pBuffer, p_pPutData->stWParam.lLen);
			CompressAByte(stWParam, (const char*)&*stWParam.begin(), SafeSizeToLength<long>(stWParam.size()));
		}

		JSONBINRPC::Callback_IJsonBinRPC_JsonBinPUTPtr<CJsonBinRPCCallBack>	ProcessCB =
			JSONBINRPC::newCallback_IJsonBinRPC_JsonBinPUT(cb,
				&CJsonBinRPCCallBack::ice_response,
				&CJsonBinRPCCallBack::exception,
				&CJsonBinRPCCallBack::sent);

		// 不保存 AsyncResult 到回调对象，避免破坏回调智能指针生命周期。
		icecompat::begin_JsonBinPUT(pJh->ClientIO(),
			p_lSynId, p_lFuncId, p_lSetCode, cb->m_stJsonReq, p_pPutData->lParam, stLParam, p_pPutData->wParam, stWParam, ProcessCB);

		// iret = (long long)cb->m_pResult.get();
		// ((JSONBINRPC::CJsonBinRPCImp *)p_hHandle)->ClientIO()->end_JsonBinRPC()
	}
	catch (const IceUtil::Exception& ex)
	{
		std::string strDetail = MakeIceExceptionDetail(__FUNCTION__, ex);
		SetIceRPCPushLastError(p_hHandle, s_iIceRPCPushIceException, strDetail, __FUNCTION__);
		// strncpy(p_pPutData->szErrInfo,ex.ice_id(),sizeof(p_pPutData->szErrInfo)-1);
		// p_pfnPutCallback(p_pPutData);
		ST_JSON_MULTI_RESULT_EX* pResult = new ST_JSON_MULTI_RESULT_EX;
		pResult->lRetVal = -1;
		pResult->lSynId = p_lSynId;
		pResult->lFuncId = p_lFuncId;
		//pResult->aReqJson	= p_szJsonReq;
		//pResult->stJsonReq		= pResult->aReqJson.c_str();
		pResult->aReqJson.resize(p_lBufLen);
		memcpy(&*pResult->aReqJson.begin(), p_szJsonReq, p_lBufLen);
		// 保存原始请求信息，便于回调时回传给上层识别。
		pResult->stJsonReq.lLen = p_lBufLen;
		pResult->stJsonReq.pBuffer = (unsigned char*)&*pResult->aReqJson.begin();
		pResult->pParam = p_pParam;
		strncpy(pResult->szErrInfo, strDetail.c_str(), sizeof(pResult->szErrInfo) - 1);

		try
		{
			p_pfnPutCallback(pResult);
		}
		catch (const std::exception& cbEx)
		{
			SetIceRPCPushStdException(p_hHandle, "JsonBinClientPUT_async.callback", cbEx);
		}
		catch (...)
		{
			SetIceRPCPushUnknownException(p_hHandle, "JsonBinClientPUT_async.callback");
		}

		InterlockedDecrement64(&g_lCrowded);
	}
	return iret;
}
// Pre/End PUT 的发送阶段，p_pPutData 内部参数在发送前会被复制并压缩。
static ST_JSON_M_RESULT_LEVEL* PreJsonBinClientPUTImpl(HANDLE p_hHandle, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lBufLen, const ST_JSON_M_RESULT_TOP* p_pPutData)
{
	if (!p_hHandle)
		return NULL;

	HANDLE		hresult = NULL;
	JSONBINRPC::ST_JSON_BIN_HANDLE* pJh = (JSONBINRPC::ST_JSON_BIN_HANDLE*)p_hHandle;
	// tag_ICEHRESULT	*	hice = new tag_ICEHRESULT;

// 	p_pPutData->lSynId	= p_lSynId;
// 	p_pPutData->lFuncId= p_lFuncId;
// 	p_pPutData->stJsonReq= p_szJsonReq;
	ST_JSON_MULTI_RESULT_EX* pResult = new ST_JSON_MULTI_RESULT_EX;
	pResult->lSynId = p_lSynId;

	pResult->lSynId = p_lSynId;
	pResult->lFuncId = p_lFuncId;
	//pResult->aReqJson	= p_szJsonReq;
	//pResult->stJsonReq		= pResult->aReqJson.c_str();
	::JSONBINRPC::AByte							tmpreqjsonReq;
	size_t  output_length = CompressAByte(tmpreqjsonReq, p_szJsonReq, p_lBufLen);
	pResult->aReqJson.resize(p_lBufLen);
	memcpy(&*pResult->aReqJson.begin(), p_szJsonReq, p_lBufLen);
	pResult->stJsonReq.lLen = p_lBufLen;
	pResult->stJsonReq.pBuffer = (unsigned char*)&*pResult->aReqJson.begin();
	InterlockedIncrement64(&g_lCrowded);
	try
	{
		::JSONBINRPC::AByte	stLParam, stWParam;
		if (p_pPutData->stLParam.lLen > 0)
		{
			stLParam.resize(p_pPutData->stLParam.lLen);
			memcpy(&*stLParam.begin(), p_pPutData->stLParam.pBuffer, p_pPutData->stLParam.lLen);
			CompressAByte(stLParam, (const char*)&*stLParam.begin(), SafeSizeToLength<long>(stLParam.size()));
		}
		if (p_pPutData->stWParam.lLen > 0)
		{
			stWParam.resize(p_pPutData->stWParam.lLen);
			memcpy(&*stWParam.begin(), p_pPutData->stWParam.pBuffer, p_pPutData->stWParam.lLen);
			CompressAByte(stWParam, (const char*)&*stWParam.begin(), SafeSizeToLength<long>(stWParam.size()));
		}
		pResult->pResult = icecompat::begin_JsonBinPUT(pJh->ClientIO(),
			p_lSynId, p_lFuncId, p_lSetCode, tmpreqjsonReq, p_pPutData->lParam, stLParam, p_pPutData->wParam, stWParam);
	}
	catch (const IceUtil::Exception& ex)
	{
		pResult->lRetVal = -1;
		std::string strDetail = MakeIceExceptionDetail(__FUNCTION__, ex);
		SetIceRPCPushLastError(p_hHandle, s_iIceRPCPushIceException, strDetail, __FUNCTION__);
		strncpy(pResult->szErrInfo, strDetail.c_str(), sizeof(pResult->szErrInfo) - 1);
	}
	return pResult;
}
// Pre/End PUT 的收尾阶段，解压返回参数并填充结果节点。
static long long EndPreJsonBinClientPUTImpl(HANDLE p_hHandle, ST_JSON_M_RESULT_LEVEL* p_pResultOrg)
{
	if (!p_hHandle)
		return 0;
	InterlockedDecrement64(&g_lCrowded);
	JSONBINRPC::ST_JSON_BIN_HANDLE* pJh = (JSONBINRPC::ST_JSON_BIN_HANDLE*)p_hHandle;
	ST_JSON_MULTI_RESULT_EX* pResult = (ST_JSON_MULTI_RESULT_EX*)p_pResultOrg;
	//::Ice::AsyncResult * art = (::Ice::AsyncResult *)hresult;
	//::Ice::AsyncResultPtr	newptr(art);
	try
	{
		//ret = ((JSONBINRPCU::CJsonBinRPCImp *)p_hHandle)->ClientIO()->end_JsonBinPUT(hice->pResult);
		//::JSONBINRPC::AByte	stLParam,stWParam;
		::std::string	errinf;
		pResult->lRetVal = icecompat::end_JsonBinPUT(pResult->lParam, pResult->aLParam, pResult->wParam, pResult->aWParam, errinf, pResult->pResult);
		strncpy(pResult->szErrInfo, errinf.c_str(), sizeof(pResult->szErrInfo) - 1);
		if (pResult->aLParam.size() > 0)
		{
			UnCompressAByte(pResult->aLParam, (const char*)pResult->aLParam.data(), SafeSizeToLength<long>(pResult->aLParam.size()));
			pResult->stLParam.lLen = SafeSizeToLength<int>(pResult->aLParam.size());
			pResult->stLParam.pBuffer = (unsigned char*)pResult->aLParam.data();//new char[pResult->stLParam.lLen];
			// memcpy(pResult->stLParam.pBuffer,&*stLParam.begin(),pResult->stLParam.lLen);
		}
		if (pResult->aWParam.size() > 0)
		{
			UnCompressAByte(pResult->aWParam, (const char*)pResult->aWParam.data(), SafeSizeToLength<long>(pResult->aWParam.size()));
			pResult->stWParam.lLen = SafeSizeToLength<int>(pResult->aWParam.size());
			pResult->stWParam.pBuffer = (unsigned char*)pResult->aWParam.data();//new char[pResult->stWParam.lLen];
			// memcpy(pResult->stWParam.pBuffer,&*stWParam.begin(),pResult->stWParam.lLen);
		}
	}
	catch (const IceUtil::Exception& ex)
	{
		pResult->lRetVal = -1;
		std::string strDetail = MakeIceExceptionDetail(__FUNCTION__, ex);
		SetIceRPCPushLastError(p_hHandle, s_iIceRPCPushIceException, strDetail, __FUNCTION__);
		strncpy(pResult->szErrInfo, strDetail.c_str(), sizeof(pResult->szErrInfo) - 1);
	}
	return pResult->lRetVal;
}

// 删除 CreateJsonICE* 返回的句柄，并释放内部服务端或客户端状态资源。
static void DeleteJsonICERPCImpl(HANDLE p_hHandle)
{
	JSONBINRPC::ST_JSON_BIN_HANDLE* pJh = (JSONBINRPC::ST_JSON_BIN_HANDLE*)p_hHandle;
	if (!p_hHandle)
		return;

	if (pJh->pRpc->IsClientMode())
	{
	}
	JSONBINRPC::CJsonBinRPCImp::DeleteIt(pJh->pRpc);

	//delete	jh;
	pJh->ReleaseIt();
}

// 直回调模式下发送 Ice AMD 响应，并释放内部回调结果节点。
static void JsonICEResponseDataImpl(HANDLE p_hHandle, ST_JSON_M_RESULT_TOP* p_pResultCallback)
{
	if (!p_pResultCallback)
		return;
	ST_JSON_MULTI_RESULT_DIRECT_CALLBACK* pResult = (ST_JSON_MULTI_RESULT_DIRECT_CALLBACK*)p_pResultCallback;
	::JSONBINRPC::AByte	stLParam, stWParam;
	if (pResult->stLParam.lLen > 0)
	{
		CompressAByte(stLParam, reinterpret_cast<const char*>(pResult->stLParam.pBuffer), pResult->stLParam.lLen);
	}
	if (pResult->stWParam.lLen > 0)
	{
		CompressAByte(stWParam, reinterpret_cast<const char*>(pResult->stWParam.pBuffer), pResult->stWParam.lLen);
	}
	try
	{
		if (pResult->chMode == EN_JSON_INPUT_RPC)
			pResult->pRpcCallback->ice_response(pResult->lRetVal, pResult->lParam, stLParam, pResult->wParam, stWParam, pResult->szErrInfo);
		else
			pResult->pPutCallback->ice_response(pResult->lRetVal, pResult->lParam, stLParam, pResult->wParam, stWParam, pResult->szErrInfo);
	}
	catch (const IceUtil::Exception& ex)
	{
		char	str[1024] = { 0 };
		UTF82ASC(ex.what(), str, 1023);
		SetIceRPCPushIceException(p_hHandle, __FUNCTION__, ex);
	}
	delete pResult;
}



// 导出函数只做参数校验、异常隔离和错误上报，具体业务仍走 Impl 旧实现。
HANDLE CreateJsonICEServer(const char* p_szCfgFile, const char* p_szEndPointName, HANDLE& p_hSem, bool p_bSnappyCompress)
{
	try
	{
		if (p_szCfgFile == NULL || p_szEndPointName == NULL)
		{
			SetIceRPCPushInvalidParam(NULL, __FUNCTION__, "INVALID_PARAM: cfg file or endpoint name is null");
			return NULL;
		}
		HANDLE hHandle = CreateJsonICEServerImpl(p_szCfgFile, p_szEndPointName, p_hSem, p_bSnappyCompress);
		if (hHandle == NULL)
		{
			SetIceRPCPushLastError(NULL, s_iIceRPCPushCreateFailed, "CREATE_FAILED: CreateJsonICEServer failed to start server", __FUNCTION__);
			return NULL;
		}
		SetIceRPCPushOk(hHandle, __FUNCTION__);
		return hHandle;
	}
	catch (const IceUtil::Exception& ex)
	{
		SetIceRPCPushIceException(NULL, __FUNCTION__, ex);
	}
	catch (const std::exception& ex)
	{
		SetIceRPCPushStdException(NULL, __FUNCTION__, ex);
	}
	catch (...)
	{
		SetIceRPCPushUnknownException(NULL, __FUNCTION__);
	}
	return NULL;
}

void RegServerCallBackFunc(HANDLE p_hHandle, func_JsonICEServerCallBsack p_pfnCallback, void* p_pParam)
{
	try
	{
		if (p_hHandle == NULL || p_pfnCallback == NULL)
		{
			SetIceRPCPushInvalidParam(p_hHandle, __FUNCTION__, "INVALID_PARAM: handle or callback is null");
			return;
		}
		RegServerCallBackFuncImpl(p_hHandle, p_pfnCallback, p_pParam);
		SetIceRPCPushOk(p_hHandle, __FUNCTION__);
	}
	catch (const IceUtil::Exception& ex)
	{
		SetIceRPCPushIceException(p_hHandle, __FUNCTION__, ex);
	}
	catch (const std::exception& ex)
	{
		SetIceRPCPushStdException(p_hHandle, __FUNCTION__, ex);
	}
	catch (...)
	{
		SetIceRPCPushUnknownException(p_hHandle, __FUNCTION__);
	}
}

HANDLE CreateJsonICEClient2(int p_iNum, const char* p_pszPropertyKey[], const char* p_pszProperty[], const char* p_szProxyProperty, HANDLE& p_hSem, int p_iThreadPool)
{
	try
	{
		if (p_iNum <= 0 || p_pszPropertyKey == NULL || p_pszProperty == NULL || p_szProxyProperty == NULL)
		{
			SetIceRPCPushInvalidParam(NULL, __FUNCTION__, "INVALID_PARAM: property list or proxy property is invalid");
			return NULL;
		}
		HANDLE hHandle = CreateJsonICEClient2Impl(p_iNum, p_pszPropertyKey, p_pszProperty, p_szProxyProperty, p_hSem, p_iThreadPool);
		if (hHandle == NULL)
		{
			SetIceRPCPushLastError(NULL, s_iIceRPCPushCreateFailed, "CREATE_FAILED: CreateJsonICEClient2 failed to start client", __FUNCTION__);
			return NULL;
		}
		SetIceRPCPushOk(hHandle, __FUNCTION__);
		return hHandle;
	}
	catch (const IceUtil::Exception& ex)
	{
		SetIceRPCPushIceException(NULL, __FUNCTION__, ex);
	}
	catch (const std::exception& ex)
	{
		SetIceRPCPushStdException(NULL, __FUNCTION__, ex);
	}
	catch (...)
	{
		SetIceRPCPushUnknownException(NULL, __FUNCTION__);
	}
	return NULL;
}

HANDLE CreateJsonICEClient(const char* p_szCfgFile, const char* p_szProxyProperty, HANDLE& p_hSem, int p_iThreadPool)
{
	try
	{
		if (p_szCfgFile == NULL || p_szProxyProperty == NULL)
		{
			SetIceRPCPushInvalidParam(NULL, __FUNCTION__, "INVALID_PARAM: cfg file or proxy property is null");
			return NULL;
		}
		HANDLE hHandle = CreateJsonICEClientImpl(p_szCfgFile, p_szProxyProperty, p_hSem, p_iThreadPool);
		if (hHandle == NULL)
		{
			SetIceRPCPushLastError(NULL, s_iIceRPCPushCreateFailed, "CREATE_FAILED: CreateJsonICEClient failed to start client", __FUNCTION__);
			return NULL;
		}
		SetIceRPCPushOk(hHandle, __FUNCTION__);
		return hHandle;
	}
	catch (const IceUtil::Exception& ex)
	{
		SetIceRPCPushIceException(NULL, __FUNCTION__, ex);
	}
	catch (const std::exception& ex)
	{
		SetIceRPCPushStdException(NULL, __FUNCTION__, ex);
	}
	catch (...)
	{
		SetIceRPCPushUnknownException(NULL, __FUNCTION__);
	}
	return NULL;
}

HANDLE RegisterJsonICEClient(HANDLE p_hHandle, const char* p_szGuid, func_JsonICEPushClientPack p_pfnCallback, int p_iIsReg, void* p_pParam)
{
	try
	{
		if (p_hHandle == NULL || p_szGuid == NULL || (p_iIsReg != 0 && p_pfnCallback == NULL))
		{
			SetIceRPCPushInvalidParam(p_hHandle, __FUNCTION__, "INVALID_PARAM: handle, guid or callback is invalid");
			return NULL;
		}
		HANDLE hRet = RegisterJsonICEClientImpl(p_hHandle, p_szGuid, p_pfnCallback, p_iIsReg, p_pParam);
		if (hRet == NULL)
		{
			SetIceRPCPushLastError(p_hHandle, s_iIceRPCPushCallFailed, "CALL_FAILED: RegisterJsonICEClient returned null", __FUNCTION__);
		}
		else
		{
			SetIceRPCPushOk(p_hHandle, __FUNCTION__);
		}
		return hRet;
	}
	catch (const IceUtil::Exception& ex)
	{
		SetIceRPCPushIceException(p_hHandle, __FUNCTION__, ex);
	}
	catch (const std::exception& ex)
	{
		SetIceRPCPushStdException(p_hHandle, __FUNCTION__, ex);
	}
	catch (...)
	{
		SetIceRPCPushUnknownException(p_hHandle, __FUNCTION__);
	}
	return NULL;
}

const char* RegisterJsonICEClient2(HANDLE p_hHandle, const char* p_szGuid, const char* p_szSubInfo, func_JsonICEPushClientPack p_pfnCallback, int p_iIsReg, void* p_pParam)
{
	try
	{
		if (p_hHandle == NULL || p_szGuid == NULL || (p_iIsReg != 0 && p_pfnCallback == NULL))
		{
			SetIceRPCPushInvalidParam(p_hHandle, __FUNCTION__, "INVALID_PARAM: handle, guid or callback is invalid");
			return NULL;
		}
		const char* pRet = RegisterJsonICEClient2Impl(p_hHandle, p_szGuid, p_szSubInfo, p_pfnCallback, p_iIsReg, p_pParam);
		if (pRet == NULL)
		{
			SetIceRPCPushLastError(p_hHandle, s_iIceRPCPushCallFailed, "CALL_FAILED: RegisterJsonICEClient2 returned null", __FUNCTION__);
		}
		else
		{
			SetIceRPCPushOk(p_hHandle, __FUNCTION__);
		}
		return pRet;
	}
	catch (const IceUtil::Exception& ex)
	{
		SetIceRPCPushIceException(p_hHandle, __FUNCTION__, ex);
	}
	catch (const std::exception& ex)
	{
		SetIceRPCPushStdException(p_hHandle, __FUNCTION__, ex);
	}
	catch (...)
	{
		SetIceRPCPushUnknownException(p_hHandle, __FUNCTION__);
	}
	return NULL;
}

const char* JsonGetEndPoint(HANDLE p_hHandle)
{
	try
	{
		if (p_hHandle == NULL)
		{
			SetIceRPCPushInvalidParam(p_hHandle, __FUNCTION__, "INVALID_HANDLE: handle is null");
			return NULL;
		}
		return JsonGetEndPointImpl(p_hHandle);
	}
	catch (const IceUtil::Exception& ex)
	{
		SetIceRPCPushIceException(p_hHandle, __FUNCTION__, ex);
	}
	catch (const std::exception& ex)
	{
		SetIceRPCPushStdException(p_hHandle, __FUNCTION__, ex);
	}
	catch (...)
	{
		SetIceRPCPushUnknownException(p_hHandle, __FUNCTION__);
	}
	return NULL;
}

ST_JSON_INPUT* JsonBinSrvPopfront(HANDLE p_hHandle)
{
	try
	{
		if (p_hHandle == NULL)
		{
			SetIceRPCPushInvalidParam(p_hHandle, __FUNCTION__, "INVALID_HANDLE: handle is null");
			return NULL;
		}
		return JsonBinSrvPopfrontImpl(p_hHandle);
	}
	catch (const IceUtil::Exception& ex)
	{
		SetIceRPCPushIceException(p_hHandle, __FUNCTION__, ex);
	}
	catch (const std::exception& ex)
	{
		SetIceRPCPushStdException(p_hHandle, __FUNCTION__, ex);
	}
	catch (...)
	{
		SetIceRPCPushUnknownException(p_hHandle, __FUNCTION__);
	}
	return NULL;
}

void JsonBinSrvComplete(ST_JSON_INPUT* p_pNode, ST_JSON_M_RESULT_TOP* p_pResult)
{
	try
	{
		if (p_pNode == NULL)
		{
			SetIceRPCPushInvalidParam(NULL, __FUNCTION__, "INVALID_PARAM: server request node is null");
			return;
		}
		JsonBinSrvCompleteImpl(p_pNode, p_pResult);
	}
	catch (const IceUtil::Exception& ex)
	{
		SetIceRPCPushIceException(p_pNode != NULL ? p_pNode->hSelf : NULL, __FUNCTION__, ex);
	}
	catch (const std::exception& ex)
	{
		SetIceRPCPushStdException(p_pNode != NULL ? p_pNode->hSelf : NULL, __FUNCTION__, ex);
	}
	catch (...)
	{
		SetIceRPCPushUnknownException(p_pNode != NULL ? p_pNode->hSelf : NULL, __FUNCTION__);
	}
}

long long PushJsonICEServerData(HANDLE p_hHandle, int p_lReqNo, const char* p_pBuf, long p_lBufLen, bool p_bAsync)
{
	try
	{
		if (p_hHandle == NULL || p_lBufLen < 0 || (p_lBufLen > 0 && p_pBuf == NULL))
		{
			SetIceRPCPushInvalidParam(p_hHandle, __FUNCTION__, "INVALID_PARAM: handle or push buffer is invalid");
			return s_iIceRPCPushInvalidParam;
		}
		long long lRet = PushJsonICEServerDataImpl(p_hHandle, p_lReqNo, p_pBuf, p_lBufLen, p_bAsync);
		SetIceRPCPushOk(p_hHandle, __FUNCTION__);
		return lRet;
	}
	catch (const IceUtil::Exception& ex)
	{
		SetIceRPCPushIceException(p_hHandle, __FUNCTION__, ex);
	}
	catch (const std::exception& ex)
	{
		SetIceRPCPushStdException(p_hHandle, __FUNCTION__, ex);
	}
	catch (...)
	{
		SetIceRPCPushUnknownException(p_hHandle, __FUNCTION__);
	}
	return s_iIceRPCPushUnknownException;
}

void IJsonMutiResultFree(ST_JSON_M_RESULT_LEVEL* p_pResult)
{
	try
	{
		IJsonMutiResultFreeImpl(p_pResult);
		SetIceRPCPushOk(NULL, __FUNCTION__);
	}
	catch (const IceUtil::Exception& ex)
	{
		SetIceRPCPushIceException(NULL, __FUNCTION__, ex);
	}
	catch (const std::exception& ex)
	{
		SetIceRPCPushStdException(NULL, __FUNCTION__, ex);
	}
	catch (...)
	{
		SetIceRPCPushUnknownException(NULL, __FUNCTION__);
	}
}

ST_JSON_M_RESULT_LEVEL* JsonBinClientRPC(HANDLE p_hHandle, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lBufLen)
{
	try
	{
		if (p_hHandle == NULL || p_lBufLen < 0 || (p_lBufLen > 0 && p_szJsonReq == NULL))
		{
			SetIceRPCPushInvalidParam(p_hHandle, __FUNCTION__, "INVALID_PARAM: handle or json request is invalid");
			return NULL;
		}
		ST_JSON_M_RESULT_LEVEL* pResult = JsonBinClientRPCImpl(p_hHandle, p_lSynId, p_lFuncId, p_lSetCode, p_szJsonReq != NULL ? p_szJsonReq : "", p_lBufLen);
		if (pResult == NULL)
		{
			SetIceRPCPushLastError(p_hHandle, s_iIceRPCPushCallFailed, "CALL_FAILED: JsonBinClientRPC returned null", __FUNCTION__);
		}
		return pResult;
	}
	catch (const IceUtil::Exception& ex)
	{
		SetIceRPCPushIceException(p_hHandle, __FUNCTION__, ex);
	}
	catch (const std::exception& ex)
	{
		SetIceRPCPushStdException(p_hHandle, __FUNCTION__, ex);
	}
	catch (...)
	{
		SetIceRPCPushUnknownException(p_hHandle, __FUNCTION__);
	}
	return NULL;
}

long long JsonBinClientRPC_async(HANDLE p_hHandle, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lBufLen, LPTHREAD_START_ROUTINE p_pfnCallback, void* p_pParam)
{
	try
	{
		if (p_hHandle == NULL || p_pfnCallback == NULL || p_lBufLen < 0 || (p_lBufLen > 0 && p_szJsonReq == NULL))
		{
			SetIceRPCPushInvalidParam(p_hHandle, __FUNCTION__, "INVALID_PARAM: handle, json request or callback is invalid");
			return s_iIceRPCPushInvalidParam;
		}
		return JsonBinClientRPCAsyncImpl(p_hHandle, p_lSynId, p_lFuncId, p_lSetCode, p_szJsonReq != NULL ? p_szJsonReq : "", p_lBufLen, p_pfnCallback, p_pParam);
	}
	catch (const IceUtil::Exception& ex)
	{
		SetIceRPCPushIceException(p_hHandle, __FUNCTION__, ex);
	}
	catch (const std::exception& ex)
	{
		SetIceRPCPushStdException(p_hHandle, __FUNCTION__, ex);
	}
	catch (...)
	{
		SetIceRPCPushUnknownException(p_hHandle, __FUNCTION__);
	}
	return s_iIceRPCPushUnknownException;
}

ST_JSON_M_RESULT_LEVEL* PreJsonBinClientRPC(HANDLE p_hHandle, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lBufLen)
{
	try
	{
		if (p_hHandle == NULL || p_lBufLen < 0 || (p_lBufLen > 0 && p_szJsonReq == NULL))
		{
			SetIceRPCPushInvalidParam(p_hHandle, __FUNCTION__, "INVALID_PARAM: handle or json request is invalid");
			return NULL;
		}
		return PreJsonBinClientRPCImpl(p_hHandle, p_lSynId, p_lFuncId, p_lSetCode, p_szJsonReq != NULL ? p_szJsonReq : "", p_lBufLen);
	}
	catch (const IceUtil::Exception& ex)
	{
		SetIceRPCPushIceException(p_hHandle, __FUNCTION__, ex);
	}
	catch (const std::exception& ex)
	{
		SetIceRPCPushStdException(p_hHandle, __FUNCTION__, ex);
	}
	catch (...)
	{
		SetIceRPCPushUnknownException(p_hHandle, __FUNCTION__);
	}
	return NULL;
}

long long EndPreJsonBinClientRPC(HANDLE p_hHandle, ST_JSON_M_RESULT_LEVEL* p_pResult)
{
	try
	{
		if (p_hHandle == NULL || p_pResult == NULL)
		{
			SetIceRPCPushInvalidParam(p_hHandle, __FUNCTION__, "INVALID_PARAM: handle or pre result is null");
			return s_iIceRPCPushInvalidParam;
		}
		return EndPreJsonBinClientRPCImpl(p_hHandle, p_pResult);
	}
	catch (const IceUtil::Exception& ex)
	{
		SetIceRPCPushIceException(p_hHandle, __FUNCTION__, ex);
	}
	catch (const std::exception& ex)
	{
		SetIceRPCPushStdException(p_hHandle, __FUNCTION__, ex);
	}
	catch (...)
	{
		SetIceRPCPushUnknownException(p_hHandle, __FUNCTION__);
	}
	return s_iIceRPCPushUnknownException;
}

ST_JSON_M_RESULT_LEVEL* JsonBinClientPUT(HANDLE p_hHandle, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lBufLen, const ST_JSON_M_RESULT_TOP* p_pPutData)
{
	try
	{
		if (p_hHandle == NULL || p_pPutData == NULL || p_lBufLen < 0 || (p_lBufLen > 0 && p_szJsonReq == NULL))
		{
			SetIceRPCPushInvalidParam(p_hHandle, __FUNCTION__, "INVALID_PARAM: handle, json request or put data is invalid");
			return NULL;
		}
		return JsonBinClientPUTImpl(p_hHandle, p_lSynId, p_lFuncId, p_lSetCode, p_szJsonReq != NULL ? p_szJsonReq : "", p_lBufLen, p_pPutData);
	}
	catch (const IceUtil::Exception& ex)
	{
		SetIceRPCPushIceException(p_hHandle, __FUNCTION__, ex);
	}
	catch (const std::exception& ex)
	{
		SetIceRPCPushStdException(p_hHandle, __FUNCTION__, ex);
	}
	catch (...)
	{
		SetIceRPCPushUnknownException(p_hHandle, __FUNCTION__);
	}
	return NULL;
}

long long JsonBinClientPUT_async(HANDLE p_hHandle, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lBufLen, const ST_JSON_M_RESULT_TOP* p_pPutData, LPTHREAD_START_ROUTINE p_pfnPutCallback, void* p_pParam)
{
	try
	{
		if (p_hHandle == NULL || p_pPutData == NULL || p_pfnPutCallback == NULL || p_lBufLen < 0 || (p_lBufLen > 0 && p_szJsonReq == NULL))
		{
			SetIceRPCPushInvalidParam(p_hHandle, __FUNCTION__, "INVALID_PARAM: handle, json request, put data or callback is invalid");
			return s_iIceRPCPushInvalidParam;
		}
		return JsonBinClientPUTAsyncImpl(p_hHandle, p_lSynId, p_lFuncId, p_lSetCode, p_szJsonReq != NULL ? p_szJsonReq : "", p_lBufLen, p_pPutData, p_pfnPutCallback, p_pParam);
	}
	catch (const IceUtil::Exception& ex)
	{
		SetIceRPCPushIceException(p_hHandle, __FUNCTION__, ex);
	}
	catch (const std::exception& ex)
	{
		SetIceRPCPushStdException(p_hHandle, __FUNCTION__, ex);
	}
	catch (...)
	{
		SetIceRPCPushUnknownException(p_hHandle, __FUNCTION__);
	}
	return s_iIceRPCPushUnknownException;
}

ST_JSON_M_RESULT_LEVEL* PreJsonBinClientPUT(HANDLE p_hHandle, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lBufLen, const ST_JSON_M_RESULT_TOP* p_pPutData)
{
	try
	{
		if (p_hHandle == NULL || p_pPutData == NULL || p_lBufLen < 0 || (p_lBufLen > 0 && p_szJsonReq == NULL))
		{
			SetIceRPCPushInvalidParam(p_hHandle, __FUNCTION__, "INVALID_PARAM: handle, json request or put data is invalid");
			return NULL;
		}
		return PreJsonBinClientPUTImpl(p_hHandle, p_lSynId, p_lFuncId, p_lSetCode, p_szJsonReq != NULL ? p_szJsonReq : "", p_lBufLen, p_pPutData);
	}
	catch (const IceUtil::Exception& ex)
	{
		SetIceRPCPushIceException(p_hHandle, __FUNCTION__, ex);
	}
	catch (const std::exception& ex)
	{
		SetIceRPCPushStdException(p_hHandle, __FUNCTION__, ex);
	}
	catch (...)
	{
		SetIceRPCPushUnknownException(p_hHandle, __FUNCTION__);
	}
	return NULL;
}

long long EndPreJsonBinClientPUT(HANDLE p_hHandle, ST_JSON_M_RESULT_LEVEL* p_pResult)
{
	try
	{
		if (p_hHandle == NULL || p_pResult == NULL)
		{
			SetIceRPCPushInvalidParam(p_hHandle, __FUNCTION__, "INVALID_PARAM: handle or pre result is null");
			return s_iIceRPCPushInvalidParam;
		}
		return EndPreJsonBinClientPUTImpl(p_hHandle, p_pResult);
	}
	catch (const IceUtil::Exception& ex)
	{
		SetIceRPCPushIceException(p_hHandle, __FUNCTION__, ex);
	}
	catch (const std::exception& ex)
	{
		SetIceRPCPushStdException(p_hHandle, __FUNCTION__, ex);
	}
	catch (...)
	{
		SetIceRPCPushUnknownException(p_hHandle, __FUNCTION__);
	}
	return s_iIceRPCPushUnknownException;
}

void DeleteJsonICERPC(HANDLE p_hHandle)
{
	try
	{
		if (p_hHandle == NULL)
		{
			SetIceRPCPushInvalidParam(p_hHandle, __FUNCTION__, "INVALID_HANDLE: handle is null");
			return;
		}
		DeleteJsonICERPCImpl(p_hHandle);
		SetIceRPCPushOk(NULL, __FUNCTION__);
	}
	catch (const IceUtil::Exception& ex)
	{
		SetIceRPCPushIceException(NULL, __FUNCTION__, ex);
	}
	catch (const std::exception& ex)
	{
		SetIceRPCPushStdException(NULL, __FUNCTION__, ex);
	}
	catch (...)
	{
		SetIceRPCPushUnknownException(NULL, __FUNCTION__);
	}
}

void JsonICEResponseData(HANDLE p_hHandle, ST_JSON_M_RESULT_TOP* p_pResultCallback)
{
	try
	{
		if (p_pResultCallback == NULL)
		{
			SetIceRPCPushInvalidParam(p_hHandle, __FUNCTION__, "INVALID_PARAM: response result is null");
			return;
		}
		JsonICEResponseDataImpl(p_hHandle, p_pResultCallback);
		SetIceRPCPushOk(p_hHandle, __FUNCTION__);
	}
	catch (const IceUtil::Exception& ex)
	{
		SetIceRPCPushIceException(p_hHandle, __FUNCTION__, ex);
	}
	catch (const std::exception& ex)
	{
		SetIceRPCPushStdException(p_hHandle, __FUNCTION__, ex);
	}
	catch (...)
	{
		SetIceRPCPushUnknownException(p_hHandle, __FUNCTION__);
	}
}



int IceRPCPushGetLastErrorCode(HANDLE p_hHandle)
{
	JSONBINRPC::ST_JSON_BIN_HANDLE* pJh = reinterpret_cast<JSONBINRPC::ST_JSON_BIN_HANDLE*>(p_hHandle);
	if (pJh == NULL)
	{
		return s_iThreadLastErrorCode;
	}
	return pJh->iLastErrorCode;
}

const char* IceRPCPushGetLastError(HANDLE p_hHandle)
{
	JSONBINRPC::ST_JSON_BIN_HANDLE* pJh = reinterpret_cast<JSONBINRPC::ST_JSON_BIN_HANDLE*>(p_hHandle);
	if (pJh == NULL)
	{
		return s_strThreadLastError.c_str();
	}
	return pJh->strLastError.c_str();
}

const char* IceRPCPushGetErrorMsg(int p_iErrorCode)
{
	return GetIceRPCPushErrorMsgInternal(p_iErrorCode);
}
