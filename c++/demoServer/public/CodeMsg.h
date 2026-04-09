#ifndef __MT_CODE_MSG_H__
#define __MT_CODE_MSG_H__

//MT服务 错误码

#include <string>
#include <map>

#define MT_CODE_NUM 27

//错误日志约定：函数+行数+日志级别+错误码+模块+错误描述;
//错误码(short 类型),错误码定义: 模块+错误描述,描述中不要出现'ERROR';
const short BUSINESS_DEAL			= -2; //业务处理异常，插件返回-2，或者主站自身处理错误，具体错误在pViewBuf，插件处理错误、应答客户端;
const short FRAME_DEAL				= -1; //业务处理异常，插件返回-1，iceGrid层错误,处理时间在指定范围，具体错误在errinfo，节点不存在，应答客户端;
const short UNKNOW_ABNORM			= 0; //未知异常;

//用户模块;
const short USER_ACCEPT_FAIL		= 1; //用户accpet失败;
const short USER_NOTICE_DISCONNET	= 2; //通知用户断开;
const short USER_NET_BREAK			= 3; //用户主动连接断开、网络出错用户断开;
const short USER_NOT_EXIST			= 4; //用户查询异常、不存在;
const short USER_REQUEST_OEVERFLOW	= 5; //单个用户累计请求数异常、溢出；单个人队列超过了上限;
const short USER_OUTQUEUE_TOPLIMIT	= 6; //用户最大输出队列深度异常;
const short USER_GETREQUEST_ABNORM	= 7; //获取用户请求获取异常、不正确的请求格式;
const short USER_RECV_OEVERFLOW		= 8; //用户接收请求缓存溢出，大于预设值;
const short USER_REQLEN_ABNORM		= 9; //用户请求包体长度异常;
const short USER_REQBODY_ABNORM		= 10; //用户请求包体异常;
const short USER_COMPREEEID_ABNORM	= 11; //用户请求压缩编号错误;
const short USER_REQ_VERSION		= 12; //用户请求协议版本错误;
const short USER_COUNT_TOPLIMIT		= 13; //用户连接数量超过上限;
const short USER_NETWORK_TYPE		= 14; //用户通信协议类型异常;
const short USER_MULTIPLE_ABNORM	= 15; //用户多种异常、版本、压缩、大小等;
const short USER_REQREMAIN_FAIL		= 16; //用户请求剩余长度错误;
const short USER_REQBUF_OEVERFLOW	= 17; //用户请求缓存区异常、溢出;
const short USER_RESBUF_OEVERFLOW	= 18; //用户应答缓存区异常、溢出;
//业务模块;
const short BUSINESS_NODE_NOTEXIST	= 19; //插件异常、插件不存在、插件不返回功能号，处理时间在指定范围;
const short BUSINESS_OVERTIME		= 20; //业务处理超时,协程超时;
//框架模块;
const short FRAME_REQUEST_OEVERFLOW = 21; //队列堆积、溢出；组合包情况则删除、应答客户端;
const short FRAME_CORO_OEVERFLOW	= 22; //协程过多、溢出;
const short FRAME_COMPRESS_FAIL		= 23; //压缩失败;
const short FRAME_SERVER_STOP		= 24; //主站服务停止、删除所有用户;

struct ST_MT_CODE_MSG
{
	short nCode;
	std::string strMsg;
};

//MT 错误码;
class CCodeMsg
{
private:
	static CCodeMsg* m_pThis;
	std::map<short, std::string> m_mapMtCodeMsg;
private:
	void Init();
public:
	CCodeMsg();
	~CCodeMsg();

	static CCodeMsg* GetInstance();
	static void Release();

	std::string GetCodeMsg(short p_nErrCode);
};

#endif