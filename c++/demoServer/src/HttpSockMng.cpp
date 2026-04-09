#include "HttpSockMng.h"
#include <vector>
#include <string>
#include <math.h>

#include "MtApiUrls.h"
#include "nsdk_atomic.h"
#include "json/json.h"
#include "Log.h"

//启动
typedef CSocketServer *(*pfnCreateHttpSockInstance)();
pfnCreateHttpSockInstance fnCreateHttpSockInstance = nullptr;

//释放
typedef void(*pfnDelHttpSockInstance)(CSocketServer *&);
pfnDelHttpSockInstance fnDelHttpSockInstance = nullptr;

pfnDelHttpSockInstance g_fnDelHttpSockInstance = nullptr;

CHttpSockMng* CHttpSockMng::m_pThis = nullptr;

void HttpNotifyHandle(CHttpAsynReq *p_refReq)
{
	if (nullptr == p_refReq)
		return;

	const char * szUrl = p_refReq->GetUrl();
	if (!szUrl)
	{
		CHttpSockMng::SendResponse(p_refReq, "url is empty", strlen("url is empty"));
		return;
	}
	else if (CHttpSockMng::GetInstance()->IsHttpUrl(szUrl))
	{
		CHttpSockMng::GetInstance()->HttpProcess(p_refReq);
	}
	else
	{
		CHttpSockMng::SendResponse(p_refReq, "url not support", strlen("url not support"));
		return;
	}
}

CHttpSockMng::CHttpSockMng() 
{
	m_bStatus = false;
	m_pHttpServerHandle = nullptr;
	m_pclLibraryOp = nullptr;
}

CHttpSockMng::~CHttpSockMng()
{
}

CHttpSockMng* CHttpSockMng::GetInstance()
{
	if (!m_pThis)
	{
		m_pThis = new CHttpSockMng;
	}
	return m_pThis;
}

void CHttpSockMng::Release()
{
	if (m_pThis)
	{
		delete m_pThis;
		m_pThis = nullptr;
	}
}

int CHttpSockMng::SendResponse(CHttpAsynReq *p_refReq, const char* p_szData, int p_iDataLen)
{
	if (!m_pThis || !p_refReq)
	{
		HTTP_WARN("实例=%d,请求包是否有效=%d", m_pThis == nullptr, p_refReq == nullptr);
		return -1;
	}

	CSocketServer* pHttpHandle = m_pThis->HttpHandle();
	if (!pHttpHandle)
	{
		HTTP_WARN("[HTTP服务] Http服务句柄为空");
		return -2;
	}

	// 只有底层确认响应已提交发送后，才允许释放请求对象；否则上层还能决定是否重试或转错误处理。
	if (!p_refReq->SendResponse(p_szData, p_iDataLen))
	{
		HTTP_WARN("[HTTP服务] 发送应答失败,ReqId=%llu,DataLen=%d",
			p_refReq->GetConnAsyId(), p_iDataLen);
		return -3;
	}

	if (!pHttpHandle->DelHttpAsynReq(p_refReq->GetConnAsyId()))
	{
		HTTP_WARN("[HTTP服务] 释放请求对象失败,ReqId=%llu", p_refReq->GetConnAsyId());
		return -4;
	}

	return 0;
}

bool CHttpSockMng::IsHttpUrl(const char *p_szUrl)
{
	return m_mapHttpUrl.find(p_szUrl) != m_mapHttpUrl.end();
}

void CHttpSockMng::RegisterUrl()
{
	for (int i = 0; i < g_nMtUrlMsgCount; ++i)
	{
		m_mapHttpUrl[g_arrMtUrlMsg[i].strUrl] = true;
	}

	for (auto it = m_mapHttpUrl.begin(); it != m_mapHttpUrl.end(); ++it)
	{
		HTTP_INFO("[HTTP服务] 已注册的URL: %s", it->first.c_str());
	}
}
bool CHttpSockMng::InitHttpServerInfo(const char* p_sHomePath)
{
	if (nullptr == p_sHomePath || strlen(p_sHomePath) == 0)
	{
		HTTP_WARN("[HTTP服务] 路径是空");
		return false;
	}
	string strDllPath = p_sHomePath;
	strDllPath.append("\\");
	strDllPath.append(HTTP_DLL_NAME);

	//TODO 读取websocket服务开关

	//TODO 读取websocket服务日志路径

	m_pclLibraryOp = new CLibraryOp;
	if (!m_pclLibraryOp)
	{
		HTTP_WARN("[HTTP服务] 装载三方库类失败...");
		return false;
	}

	//加载监控动态库
	const auto fileAttributes = ::GetFileAttributes(strDllPath.c_str());
	if (INVALID_FILE_ATTRIBUTES == fileAttributes || 0 != (fileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		HTTP_WARN("[HTTP服务] 动态库文件不存在[%s]...", strDllPath.c_str());
		delete m_pclLibraryOp;
		m_pclLibraryOp = nullptr;
		return false;
	}

	//加载动态库
	if (!m_pclLibraryOp->Load(strDllPath.c_str()))
	{
		HTTP_WARN("[HTTP服务] 动态库加载失败[%s]...", strDllPath.c_str());
		delete m_pclLibraryOp;
		m_pclLibraryOp = nullptr;
		return false;
	}

	// 创建函数
	pfnCreateHttpSockInstance fnCreateHttpSockInstance = nullptr;

	if (!m_pclLibraryOp->GetFuncAddress((void**)&fnCreateHttpSockInstance, "CreateHttpSockInstance") || nullptr == fnCreateHttpSockInstance)
	{
		HTTP_WARN("[HTTP服务] 获取方法[HttpSockIns]失败...");
		delete m_pclLibraryOp;
		m_pclLibraryOp = nullptr;
		return false;
	}

	if (nullptr == m_pHttpServerHandle)
	{
		m_pHttpServerHandle = fnCreateHttpSockInstance();
		if (nullptr == m_pHttpServerHandle)
		{
			HTTP_WARN("[HTTP服务] 获取监控方法失败");
			delete m_pclLibraryOp;
			m_pclLibraryOp = nullptr;
			return false;
		}
	}

	if (!m_pclLibraryOp->GetFuncAddress((void**)&g_fnDelHttpSockInstance, "DelHttpSockInstance") || nullptr == g_fnDelHttpSockInstance)
	{
		HTTP_WARN("[HTTP服务] 获取方法[DelHttpSockIns]失败...");
		delete m_pclLibraryOp;
		m_pclLibraryOp = nullptr;
		return false;
	}

	//TODO 初始化日志
	m_bStatus = true;

	//注册url
	RegisterUrl();

	HTTP_INFO("[HTTP服务] 初始化状态: %d", m_bStatus);

	return m_bStatus;
}

bool CHttpSockMng::Start(const char *p_szIp, unsigned short p_nPort, int p_iThreadNum, int p_iQueueNum, int p_iRBufLen, int p_iMaxConnectNum, int p_iMaxAcceptNum,
	bool p_bSSL, const char *p_szPemCertFile, const char *p_szPemKeyFile,
	const char *p_szKeyPassword, const char *p_szCAPemCertFileOrPath, char *p_szLogFold)
{
	if (nullptr == m_pHttpServerHandle)
	{
		HTTP_WARN("[HTTP服务] 创建http服务失败: 服务句柄是空");
		return false;
	}

	p_iThreadNum = p_iThreadNum <= 0 ? HTTP_THREAD_NUM : p_iThreadNum;
	p_iQueueNum = p_iQueueNum <= 0 ? HTTP_QUEUE_NUM : p_iQueueNum;
	p_iRBufLen = p_iRBufLen <= 0 ? HTTP_RECVBUF_LEN : p_iRBufLen;
	p_iMaxConnectNum = p_iMaxConnectNum <= 0 ? HTTP_CONNECT_NUM : p_iMaxConnectNum;
	p_iMaxAcceptNum = p_iMaxAcceptNum <= 0 ? HTTP_ACCEPT_NUM : p_iMaxAcceptNum;

	char szBuf[1024] = { 0 };
	char *pLogFold = (nullptr == p_szLogFold || '\0' == *p_szLogFold) ? nullptr : p_szLogFold;
	const char* pSafeLogFold = (nullptr == pLogFold) ? "" : pLogFold;
	const char* pSafePemCertFile = (nullptr == p_szPemCertFile) ? "" : p_szPemCertFile;
	const char* pSafePemKeyFile = (nullptr == p_szPemKeyFile) ? "" : p_szPemKeyFile;
	const char* pSafeKeyPassword = (nullptr == p_szKeyPassword) ? "" : p_szKeyPassword;
	const char* pSafeCAPemCertFileOrPath = (nullptr == p_szCAPemCertFileOrPath) ? "" : p_szCAPemCertFileOrPath;

	HTTP_INFO("[HTTP服务] 服务IP: %s", p_szIp);
	HTTP_INFO("[HTTP服务] 服务端口: %d", p_nPort);
	HTTP_INFO("[HTTP服务] 线程数: %d", p_iThreadNum);
	HTTP_INFO("[HTTP服务] 队列数: %d", p_iQueueNum);
	HTTP_INFO("[HTTP服务] 缓存大小: %d", p_iRBufLen);
	HTTP_INFO("[HTTP服务] 最大Accept: %d", p_iMaxConnectNum);
	HTTP_INFO("[HTTP服务] 最大Connect: %d", p_iMaxAcceptNum);
	HTTP_INFO("[HTTP服务] 底层日志路径:%s", pSafeLogFold);
	HTTP_INFO("[HTTP服务] 是否开启https:%d", p_bSSL);

	if (p_bSSL)
	{
		HTTP_INFO("[HTTP服务] 证书文件路径:%s", pSafePemCertFile);
		HTTP_INFO("[HTTP服务] 私钥文件路径:%s", pSafePemKeyFile);
		HTTP_INFO("[HTTP服务] 私钥密码:%s", pSafeKeyPassword);
		HTTP_INFO("[HTTP服务] CA证书文件路径:%s", pSafeCAPemCertFileOrPath);

		//启动服务
		if (!m_pHttpServerHandle->CreateHttpsSock(p_szIp, p_nPort, p_iRBufLen, p_iMaxConnectNum, p_iMaxAcceptNum, HttpNotifyHandle, p_iThreadNum, p_iQueueNum, szBuf,
			p_szPemCertFile, p_szPemKeyFile, p_szKeyPassword, p_szCAPemCertFileOrPath, pLogFold))
		{
			m_bStatus = false;
			HTTP_WARN("[HTTP服务] 创建https服务失败: %s", szBuf);
		}
	}
	else
	{
		//启动服务
		if (!m_pHttpServerHandle->CreateHttpSock(p_szIp, p_nPort, p_iRBufLen, p_iMaxConnectNum, p_iMaxAcceptNum, HttpNotifyHandle, p_iThreadNum, p_iQueueNum, szBuf, pLogFold))
		{
			m_bStatus = false;
			HTTP_WARN("[HTTP服务] 创建http服务失败: %s", szBuf);
		}
	}
	return m_bStatus;
}

void CHttpSockMng::Stop()
{
	m_bStatus = false;

	if (nullptr == m_pHttpServerHandle)
		return;

	m_pHttpServerHandle->StopHttpSock();

	if (nullptr == g_fnDelHttpSockInstance)
		return;

	g_fnDelHttpSockInstance(m_pHttpServerHandle);

	if (nullptr == m_pclLibraryOp)
		return;

	delete m_pclLibraryOp;
	m_pclLibraryOp = nullptr;
}

bool CHttpSockMng::HttpProcess(CHttpAsynReq *p_refReq)
{
	if (!m_bStatus || !p_refReq)
	{
		HTTP_WARN("[HTTP服务] 状态=%d,请求包是否有效=%d", m_bStatus, p_refReq == nullptr);
		return false;
	}

	//应答
	CHttpSockMng::SendResponse(p_refReq, "Done", 4);
	return true;
}




