#include "CodeMsg.h"

const ST_MT_CODE_MSG g_arrMtCodeMsg[] = {
	{BUSINESS_DEAL, "主站业务处理异常"}, {FRAME_DEAL,"插件业务处理异常"}, 
	{UNKNOW_ABNORM, "未知错误"}, {USER_ACCEPT_FAIL,"接收失败"},
	{USER_NOTICE_DISCONNET,"通知断开"}, {USER_NET_BREAK,"主站断开连接"},
	{USER_NOT_EXIST,"用户不存在"}, {USER_REQUEST_OEVERFLOW,"累计请求超过上限"},
	{USER_OUTQUEUE_TOPLIMIT, "最大输出队列异常"}, {USER_GETREQUEST_ABNORM,"获取请求错误"},
	{USER_RECV_OEVERFLOW, "接收请求缓存溢出"},{USER_REQLEN_ABNORM, "请求包长度错误"},
	{USER_REQBODY_ABNORM, "请求包体错误"}, {USER_COMPREEEID_ABNORM,"压缩编号错误"},
	{USER_REQ_VERSION, "协议版本错误"}, {USER_COUNT_TOPLIMIT,"连接数量超过上限"},
	{USER_NETWORK_TYPE, "通信协议类型错误"}, {USER_MULTIPLE_ABNORM,"版本|压缩|大小错误"},
	{USER_REQREMAIN_FAIL, "请求剩余长度错误"}, {USER_REQBUF_OEVERFLOW, "请求缓存区溢出"},
	{USER_RESBUF_OEVERFLOW,"应答缓存区溢出"}, {BUSINESS_NODE_NOTEXIST,"插件不存在"},
	{BUSINESS_OVERTIME, "协程超时"}, {FRAME_REQUEST_OEVERFLOW, "队列堆积"},
	{FRAME_CORO_OEVERFLOW,"协程过多"},{FRAME_COMPRESS_FAIL, "压缩失败"},
	{FRAME_SERVER_STOP, "主站服务停止"},
};

CCodeMsg *CCodeMsg::m_pThis = nullptr;

CCodeMsg::CCodeMsg()
{
	Init();
}

CCodeMsg::~CCodeMsg()
{
	
}

CCodeMsg* CCodeMsg::GetInstance()
{
	if (m_pThis == NULL) {
		if (m_pThis == NULL) {
			m_pThis = new CCodeMsg;
		}
	}
	return m_pThis;
}

void CCodeMsg::Release()
{
	if (NULL == m_pThis)
		return;

	delete m_pThis;
	m_pThis = NULL;
}

void CCodeMsg::Init()
{
	for(int i = 0; i < MT_CODE_NUM; i++)
	{
		m_mapMtCodeMsg[g_arrMtCodeMsg[i].nCode] = g_arrMtCodeMsg[i].strMsg;
	}
}

std::string CCodeMsg::GetCodeMsg(short p_nErrCode)
{
	if(m_mapMtCodeMsg.find(p_nErrCode) != m_mapMtCodeMsg.end())
	{
		return m_mapMtCodeMsg[p_nErrCode];
	}

	return "";
}
