#include "StdAfx.h"
//#include "HttpSockMng.h"
#include <vector>
#include <string>
#include "json/json.h"
#include <math.h>

#include "ProtoPack.h"
#include "DeriveDataMng.h"

//启动
typedef CSocketServer *(*pfnCreateHttpSockInstance)();
pfnCreateHttpSockInstance fnCreateHttpSockInstance = NULL;

//释放
typedef void(*pfnDelHttpSockInstance)(CSocketServer *&);
pfnDelHttpSockInstance fnDelHttpSockInstance = NULL;

pfnDelHttpSockInstance g_fnDelHttpSockInstance = NULL;

CHttpSockMng* CHttpSockMng::m_pThis = NULL;

void HttpNotifyHandle(CHttpAsynReq *p_refReq)
{
	if (NULL == p_refReq)
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
	m_pHttpServerHandle = NULL;
	m_pclLibraryOp = NULL;
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
		m_pThis = NULL;
	}
}

int CHttpSockMng::SendResponse(CHttpAsynReq *p_refReq, const char* p_szData, int p_iDataLen)
{
	if (!m_pThis || !p_refReq)
	{
		WARN("[HTTP服务] 实例=%d,请求包是否有效=%d", m_pThis == NULL, p_refReq == NULL);
		return -1;
	}

	CSocketServer* pHttpHandle = m_pThis->HttpHandle();
	if (!pHttpHandle)
	{
		WARN("[HTTP服务] Http服务句柄为空");
		return -2;
	}

	// 只有底层确认响应已提交发送后，才允许释放请求对象；否则上层还能决定是否重试或转错误处理。
	if (!p_refReq->SendResponse(p_szData, p_iDataLen))
	{
		WARN("[HTTP服务] 发送应答失败,ReqId=%llu,DataLen=%d",
			p_refReq->GetConnAsyId(), p_iDataLen);
		return -3;
	}

	if (!pHttpHandle->DelHttpAsynReq(p_refReq->GetConnAsyId()))
	{
		WARN("[HTTP服务] 释放请求对象失败,ReqId=%llu", p_refReq->GetConnAsyId());
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
	m_mapHttpUrl[URL_STOCK_QUERY] = true;

	for(auto it = m_mapHttpUrl.begin(); it != m_mapHttpUrl.end(); it++)
		INFO("[HTTP服务] 已注册的URL: %s", it->first.c_str());
}

bool CHttpSockMng::InitHttpServerInfo(const char* p_sHomePath)
{
	if (NULL == p_sHomePath || strlen(p_sHomePath) == 0)
	{
		WARN("[HTTP服务] 路径是空");
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
		WARN("[HTTP服务] 装载三方库类失败...");
		return FALSE;
	}

	//加载监控动态库
	DWORD attr = ::GetFileAttributes(strDllPath.c_str());
	if (INVALID_FILE_ATTRIBUTES == attr || 0 != (attr & FILE_ATTRIBUTE_DIRECTORY))
	{
		WARN("[HTTP服务] 动态库文件不存在[%s]...", strDllPath.c_str());
		delete m_pclLibraryOp;
		m_pclLibraryOp = NULL;
		return FALSE;
	}

	//加载动态库
	if (!m_pclLibraryOp->Load(strDllPath.c_str()))
	{
		WARN("[HTTP服务] 动态库加载失败[%s]...", strDllPath.c_str());
		delete m_pclLibraryOp;
		m_pclLibraryOp = NULL;
		return FALSE;
	}

	// 创建函数
	pfnCreateHttpSockInstance fnCreateHttpSockInstance = NULL;

	if (!m_pclLibraryOp->GetFuncAddress((void**)&fnCreateHttpSockInstance, "CreateHttpSockInstance") || NULL == fnCreateHttpSockInstance)
	{
		WARN("[HTTP服务] 获取方法[HttpSockIns]失败...");
		delete m_pclLibraryOp;
		m_pclLibraryOp = NULL;
		return FALSE;
	}

	if (NULL == m_pHttpServerHandle)
	{
		m_pHttpServerHandle = fnCreateHttpSockInstance();
		if (NULL == m_pHttpServerHandle)
		{
			WARN("[HTTP服务] 获取监控方法失败");
			delete m_pclLibraryOp;
			m_pclLibraryOp = NULL;
			return FALSE;
		}
	}

	if (!m_pclLibraryOp->GetFuncAddress((void**)&g_fnDelHttpSockInstance, "DelHttpSockInstance") || NULL == g_fnDelHttpSockInstance)
	{
		WARN("[HTTP服务] 获取方法[DelHttpSockIns]失败...");
		delete m_pclLibraryOp;
		m_pclLibraryOp = NULL;
		return FALSE;
	}

	//TODO 初始化日志
	m_bStatus = true;

	//注册url
	RegisterUrl();

	INFO("[HTTP服务] 初始化状态: %d", m_bStatus);

	return m_bStatus;
}

bool CHttpSockMng::Start(const char *p_szIp, unsigned short p_nPort, int p_iThreadNum, int p_iQueueNum, int p_iRBufLen, int p_iMaxConnectNum, int p_iMaxAcceptNum,
	bool p_bSSL, const char *p_szPemCertFile, const char *p_szPemKeyFile,
	const char *p_szKeyPassword, const char *p_szCAPemCertFileOrPath, char *p_szLogFold)
{
	if (NULL == m_pHttpServerHandle)
	{
		WARN("[HTTP服务] 创建http服务失败: 服务句柄是空");
		return false;
	}

	p_iThreadNum = p_iThreadNum <= 0 ? HTTP_THREAD_NUM : p_iThreadNum;
	p_iQueueNum = p_iQueueNum <= 0 ? HTTP_QUEUE_NUM : p_iQueueNum;
	p_iRBufLen = p_iRBufLen <= 0 ? HTTP_RECVBUF_LEN : p_iRBufLen;
	p_iMaxConnectNum = p_iMaxConnectNum <= 0 ? HTTP_CONNECT_NUM : p_iMaxConnectNum;
	p_iMaxAcceptNum = p_iMaxAcceptNum <= 0 ? HTTP_ACCEPT_NUM : p_iMaxAcceptNum;

	char szBuf[1024] = { 0 };
	char *pLogFold = (NULL == p_szLogFold || '\0' == *p_szLogFold) ? nullptr : p_szLogFold;
	const char* pSafeLogFold = (NULL == pLogFold) ? "" : pLogFold;
	const char* pSafePemCertFile = (NULL == p_szPemCertFile) ? "" : p_szPemCertFile;
	const char* pSafePemKeyFile = (NULL == p_szPemKeyFile) ? "" : p_szPemKeyFile;
	const char* pSafeKeyPassword = (NULL == p_szKeyPassword) ? "" : p_szKeyPassword;
	const char* pSafeCAPemCertFileOrPath = (NULL == p_szCAPemCertFileOrPath) ? "" : p_szCAPemCertFileOrPath;

	INFO("[HTTP服务] 服务IP: %s", p_szIp);
	INFO("[HTTP服务] 服务端口: %d", p_nPort);
	INFO("[HTTP服务] 线程数: %d", p_iThreadNum);
	INFO("[HTTP服务] 队列数: %d", p_iQueueNum);
	INFO("[HTTP服务] 缓存大小: %d", p_iRBufLen);
	INFO("[HTTP服务] 最大Accept: %d", p_iMaxConnectNum);
	INFO("[HTTP服务] 最大Connect: %d", p_iMaxAcceptNum);
	INFO("[HTTP服务] 底层日志路径:%s", pSafeLogFold);
	INFO("[HTTP服务] 是否开启https:%d", p_bSSL);

	if (p_bSSL)
	{
		INFO("[HTTP服务] 证书文件路径:%s", pSafePemCertFile);
		INFO("[HTTP服务] 私钥文件路径:%s", pSafePemKeyFile);
		INFO("[HTTP服务] 私钥密码:%s", pSafeKeyPassword);
		INFO("[HTTP服务] CA证书文件路径:%s", pSafeCAPemCertFileOrPath);

		//启动服务
		if (!m_pHttpServerHandle->CreateHttpsSock(p_szIp, p_nPort, p_iRBufLen, p_iMaxConnectNum, p_iMaxAcceptNum, HttpNotifyHandle, p_iThreadNum, p_iQueueNum, szBuf,
			p_szPemCertFile, p_szPemKeyFile, p_szKeyPassword, p_szCAPemCertFileOrPath, pLogFold))
		{
			m_bStatus = false;
			WARN("[HTTP服务] 创建https服务失败: %s", szBuf);
		}
	}
	else
	{
		//启动服务
		if (!m_pHttpServerHandle->CreateHttpSock(p_szIp, p_nPort, p_iRBufLen, p_iMaxConnectNum, p_iMaxAcceptNum, HttpNotifyHandle, p_iThreadNum, p_iQueueNum, szBuf, pLogFold))
		{
			m_bStatus = false;
			WARN("[HTTP服务] 创建http服务失败: %s", szBuf);
		}
	}
	return m_bStatus;
}

void CHttpSockMng::Stop()
{
	m_bStatus = FALSE;

	if (NULL == m_pHttpServerHandle)
		return;

	m_pHttpServerHandle->StopHttpSock();

	if (NULL == g_fnDelHttpSockInstance)
		return;

	g_fnDelHttpSockInstance(m_pHttpServerHandle);

	if (NULL == m_pclLibraryOp)
		return;

	delete m_pclLibraryOp;
	m_pclLibraryOp = NULL;
}

bool CHttpSockMng::HttpProcess(CHttpAsynReq *p_refReq)
{
	if (!m_bStatus || !p_refReq)
	{
		WARN("[HTTP服务] 状态=%d,请求包是否有效=%d", m_bStatus, p_refReq == NULL);
		return false;
	}

	//定义应答
	Jzt::QuotePackage	ProrobufPack; // 应答包
	ProrobufPack.set_version(PROTOBUF_VERSION);
	std::string	ProtobufResult = ""; // 应答结果

	bool bJson = false;
	Jzt::QuotePackage ReqProrobufPack;
	std::string strErrMsg;
	bool bRet = AnyPackToProto((char *)p_refReq->GetContent(), p_refReq->GetContentLen(), ReqProrobufPack, bJson, strErrMsg);//还原pb

	if (!bRet)
	{
		WARN("[HTTP服务] 还原PB协议失败,包体=%s,大小=%d,是否json=%d,信息=%s",
			p_refReq->GetContent(), p_refReq->GetContentLen(), bJson, strErrMsg.c_str());

		Jzt::ErrorAns ProtoBufErrAns;
		ProrobufPack.set_reqtype(Jzt::REQ_ERROR);// 错误功能号
		ProtoBufErrAns.set_errtype(Jzt::ERR_REQ_BODY_DATA);
		ProtoBufErrAns.set_info(strErrMsg);// 错误信息
		MakePackage(ProtoBufErrAns, bJson, ProrobufPack, ProtobufResult);//处理错误返回以bJson为准

		CHttpSockMng::SendResponse(p_refReq, ProtobufResult._Myptr(), ProtobufResult.size());
		return false;
	}

	ProrobufPack.set_reqtype(ReqProrobufPack.reqtype());
	ProrobufPack.set_assisid(ReqProrobufPack.assisid());
	ProrobufPack.set_cookie(ReqProrobufPack.cookie());
	ProrobufPack.set_mainid(ReqProrobufPack.mainid());

	Jzt::QuoteReqType emReqType = ReqProrobufPack.reqtype();

	switch (emReqType)
	{
	case Jzt::REQ_STOCK_QUERY:
	{
		//TODO 正常业务处理
		Jzt::StockQueryReq ProtoBufReq;
		//p_refProrobufPack.packdata().UnpackTo(&ProtoBufReq);
		if (!PasrePackToReq(ReqProrobufPack, ProtoBufReq, strErrMsg))
		{
			WARN("[HTTP服务] 功能号=%d,解包失败,是JSON包=%d,信息=%s",
				emReqType, bJson, strErrMsg.c_str());

			Jzt::ErrorAns ProtoBufErrAns;
			ProrobufPack.set_reqtype(Jzt::REQ_ERROR);// 错误功能号
			ProtoBufErrAns.set_reqtype(emReqType);
			ProtoBufErrAns.set_errtype(Jzt::ERR_PARSE_DATA);
			ProtoBufErrAns.set_info(strErrMsg);// 错误信息
			MakePackage(ProtoBufErrAns, bJson, ProrobufPack, ProtobufResult);//处理错误返回以bJson为准
			break;
		}

		string strKeyword = ProtoBufReq.keyword();
		if (strKeyword.empty())
		{
			WARN("[HTTP服务] 功能号=%d,关键字是空", emReqType);

			Jzt::ErrorAns ProtoBufErrAns;
			ProrobufPack.set_reqtype(Jzt::REQ_ERROR);// 错误功能号
			ProtoBufErrAns.set_reqtype(emReqType);
			ProtoBufErrAns.set_errtype(Jzt::ERR_PARSE_DATA);
			ProtoBufErrAns.set_info("keyword is empty");// 错误信息
			MakePackage(ProtoBufErrAns, bJson, ProrobufPack, ProtobufResult);//处理错误返回以bJson为准
			break;
		}

		char	szUtf8StrBuf[STR_KEYWORD_LEN] = { 0 };
		UTF82ASC(strKeyword.data(), szUtf8StrBuf);
		INFO("[HTTP服务] 功能号=%d,关键字=%s, 解析完成", emReqType, szUtf8StrBuf);//注意打印编码，utf8会奔溃

		strTolower(strKeyword);
		wstring wstrKeyword = string2wstring(strKeyword);
		wstrKeyword = TrimStr(wstrKeyword);
		vector<TagCodeWithNames> vecMatch;
		if (CDataCenterMng::GetInstance()->GetMatchCache(wstrKeyword, vecMatch))
		{
			CDataCenterMng::GetInstance()->GetMatchStock(wstrKeyword, vecMatch);
			CDataCenterMng::GetInstance()->SetMatchCache(wstrKeyword, vecMatch);
		}
		Jzt::StockQueryAns ProtoBufAns;

		memset(szUtf8StrBuf, 0, STR_KEYWORD_LEN);
		for (int i = 0; i < vecMatch.size(); i++)
		{
			Jzt::TagCodeWithName *tcode = ProtoBufAns.add_matchs();
			tcode->set_setcode(SetCodeTypeConvert(vecMatch[i].setcode));
			tcode->set_code(vecMatch[i].code);
			ASC2UTF8(vecMatch[i].name, szUtf8StrBuf, STR_KEYWORD_LEN);
			tcode->set_name(szUtf8StrBuf);
		}

		MakePackage(ProtoBufAns, bJson, ProrobufPack, ProtobufResult);
	}
	break;
	default:
	{
		ProrobufPack.set_reqtype(Jzt::REQ_ERROR);

		Jzt::ErrorAns ProtoBufErrAns;
		ProtoBufErrAns.set_reqtype(emReqType);
		ProtoBufErrAns.set_errtype(Jzt::ERR_NOT_SUPPORT);
		ProtoBufErrAns.set_info("FuncID Not Support");
		MakePackage(ProtoBufErrAns, bJson, ProrobufPack, ProtobufResult);
	}
	break;
	}

#if 0
	//获取头协议类型 "Content-Type: application/x-protobuf"
	if (0 == strcmp(p_refReq->GetHead("Content-Type"), "application/protobuf"))//proto协议
	{
		iType = 1;
		//解析包体
		//业务处理
		//应答
	}
	else if (0 == strcmp(p_refReq->GetHead("Content-Type"), "application/json"))//json协议
	{
		iType = 2;
		//解析包体
		//业务处理
		
	}
	else//协议错误
	{
		CHttpSockMng::SendResponse(p_refReq, "protocol err", strlen("protocol err"));
		return false;
	}
#endif
	INFO("[HTTP服务] 应答 cookie=%d,mainid=%d,reqid=%d,len=%d",
		ReqProrobufPack.cookie(), ReqProrobufPack.mainid(), emReqType, ProtobufResult.size());
	//应答
	CHttpSockMng::SendResponse(p_refReq, ProtobufResult._Myptr(), ProtobufResult.size());
	return true;
}

Jzt::SetCodeType CHttpSockMng::SetCodeTypeConvert(short p_nSetcodeType)
{
	switch (p_nSetcodeType)
	{
	case SZ:
		return Jzt::SZ;
	case SH:
		return Jzt::SH;
	case GZ:
		return Jzt::GZ;
	case BJ:
		return Jzt::BJ;
	default:
		return Jzt::SZ;
	}
}

