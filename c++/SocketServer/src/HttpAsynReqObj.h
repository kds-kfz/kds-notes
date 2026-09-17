#ifndef _HTTP_ASYN_REQ_OBJ_H_
#define _HTTP_ASYN_REQ_OBJ_H_

#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include "SocketServer.h"
#include "SocketInterface.h"

struct ST_HTTP_SERVER_RUNTIME;

// HTTP query 解析结果只描述网络层错误，不包含任何业务参数含义。
enum EN_HTTP_QUERY_PARSE_RESULT
{
	HTTP_QUERY_PARSE_OK = 0,
	HTTP_QUERY_PARSE_RAW_URL_TOO_LONG,
	HTTP_QUERY_PARSE_PARAM_TOO_MANY,
	HTTP_QUERY_PARSE_DECODED_NUL
};

class CHttpAsynReqObj : public CHttpAsynReq
{
public:
	// 构造一个空的 HTTP 异步请求对象，初始化默认请求和响应状态。
	// 创建并绑定所属 HTTP 实例；异步回包始终回到同一运行上下文。
	explicit CHttpAsynReqObj(ST_HTTP_SERVER_RUNTIME* p_pRuntime);
	// 销毁请求对象；实际资源回收主要依赖成员对象和上层释放流程。
	virtual ~CHttpAsynReqObj();

public:
	// 返回请求 URL；如果尚未设置则返回 nullptr。
	virtual const char* GetUrl() override;
	// 返回请求行原始 URL；兼容 GET 参数读取，未设置时返回 nullptr。
	virtual const char* GetRawUrl() override;
	// 返回 URL 中的原始 query；没有 query 时返回 nullptr。
	virtual const char* GetQueryString() override;
	// 返回 URL 解码后的 query 参数值；不存在时返回 nullptr。
	virtual const char* GetParam(const char* p_szName) override;
	// 返回 HTTP 请求方法；如果尚未设置则返回 nullptr。
	virtual const char* GetMethodType() override;
	// 返回当前已累计的请求 BODY 长度。
	virtual const int GetContentLen() override;
	// 返回请求 BODY 缓冲区；如果 BODY 为空则返回 nullptr。
	virtual const void* GetContent() override;
	// 按头名查询请求头，内部会先把头名归一化后再查找。
	virtual const char* GetHead(const char* p_szName) override;
	// 返回客户端 IP 和端口信息。
	virtual void GetAddress(char* p_szClientIp, int p_iIpLen, unsigned short& p_nClientPort) override;
	// 返回当前请求所属连接 ID。
	virtual unsigned long long GetConnId() override;
	// 返回当前请求的异步请求号。
	virtual unsigned long long GetConnAsyId() override;

	// 设置待发送的 HTTP 响应状态码。
	virtual void SetResponseStatus(HttpStatusType p_enStatus) override;
	// 追加一个响应头，发送响应时会按追加顺序一起带出。
	virtual void AddResponseHead(const char* p_szName, const char* p_szValue) override;
	// 通过底层 sender 发送 HTTP 响应；成功后会更新连接状态并收口生命周期。
	virtual bool SendResponse(const void* p_szData, int p_iLen) override;

public:
	// 绑定底层 HTTP 服务对象，供后续回包或断开连接使用。
	void SetSender(IHttpServer* p_pSender);
	// 记录当前请求所属连接 ID。
	void SetConnId(CONNID p_dwConnID);
	// 设置库内分配的异步请求号。
	void SetConnAsyId(unsigned long long p_ullReqID);
	// 标记当前请求完成后是否允许保持连接复用。
	void SetKeepAlive(bool p_bKeepAlive);
	// 写入 HTTP 请求方法。
	void SetMethod(const char* p_szMethod);
	// 写入请求 URL / PATH。
	void SetUrl(const char* p_szUrl);
	// 写入请求行原始 URL，并优先使用 HP-Socket 已拆分的 query 做一次通用解析。
	EN_HTTP_QUERY_PARSE_RESULT SetRequestUrl(const char* p_szRawUrl, const char* p_szQuery,
		size_t p_uiMaxRawUrlBytes, size_t p_uiMaxQueryParamCount,
		size_t& p_refUiObservedRawUrlBytes, size_t& p_refUiObservedQueryParamCount);
	// 写入客户端 IP 和端口信息。
	void SetAddress(const char* p_szClientIp, unsigned short p_unClientPort);
	// 标记网络层拒绝原因；继续完成 HTTP 解析后统一回包，避免 HPR_ERROR 提前丢弃响应。
	void SetRequestReject(HttpStatusType p_enStatus, const char* p_szReason);
	// 查询当前请求是否已被网络层标记拒绝。
	bool HasRequestReject() const;
	// 返回网络层拒绝状态码。
	HttpStatusType GetRequestRejectStatus() const;
	// 返回网络层拒绝文本。
	const char* GetRequestRejectReason() const;
	// 追加请求头，并按数量和字节数上限做限制校验。
	bool AddRequestHead(const char* p_szName, const char* p_szValue, size_t p_uiMaxHeadCount, size_t p_uiMaxHeadBytes);
	// 追加一段请求 BODY，并按累计总大小上限做限制校验。
	bool AppendContent(const unsigned char* p_pData, int p_iLen, size_t p_uiMaxBodyBytes);
	// wyl 2026-04-25：按 Content-Length 预留 BODY 缓冲，减少大包分片追加时的重复扩容拷贝。
	bool ReserveContent(size_t p_uiContentLen, size_t p_uiMaxBodyBytes);
	// 业务层放弃请求时的兜底收口：清理活动状态并主动断开连接。
	void AbortRequest();
	// 断开请求对象与底层 transport 的关联，防止后续继续回包到失效连接。
	void DetachTransport();
	// 标记该请求已经从解析阶段派发给上层异步处理。
	void MarkDispatched();
	// 判断该请求是否已经派发到上层。
	bool IsDispatched() const;
	// 判断当前请求是否是 keep-alive 连接。
	bool IsKeepAlive() const;
	// 判断当前是否仍持有可用的底层 transport。
	bool HasTransport() const;
	// 判断该请求是否已经成功提交过响应。
	bool HasSentResponse() const;

private:
	struct HttpHeaderItem
	{
		std::string strName;	// 响应头名称
		std::string strValue;	// 响应头值
	};
	// query 参数按 URL 原始顺序连续保存，避免 std::map 的逐节点分配。
	struct ST_HTTP_QUERY_PARAM
	{
		std::string strName;	// URL 解码后的参数名
		std::string strValue;	// URL 解码后的参数值
	};

	// 统一把头名转换成小写，避免请求头大小写差异导致查找失败。
	static std::string NormalizeHeaderName(const char* p_szName);
	// 使用单遍状态机解析 query；同名参数保留原顺序，由 GetParam 返回第一个值。
	EN_HTTP_QUERY_PARSE_RESULT ParseQueryString(size_t p_uiMaxQueryParamCount,
		size_t& p_refUiObservedQueryParamCount);

private:
	ST_HTTP_SERVER_RUNTIME* m_pRuntime;              // 不拥有；停服先解除 transport 并等待回调退出。
	IHttpServer* m_pSender;							// 当前请求绑定的底层 HTTP 服务对象，断连或停服后会被置空
	CONNID m_dwConnID;								// 当前请求所属连接 ID
	unsigned long long m_ullReqID;					// 库内分配的异步请求号
	unsigned short m_unClientPort;					// 客户端源端口
	HttpStatusType m_enHttpStatus;					// 待发送的 HTTP 响应状态码
	bool m_bDispatched;								// 是否已经从解析线程切换到上层异步处理阶段
	bool m_bKeepAlive;								// 当前请求完成后是否允许复用连接
	bool m_bResponseSent;							// 是否已经成功向底层提交过响应
	bool m_bRequestRejected;						// 是否由网络层在业务派发前拒绝
	char m_szClientIp[STR_IP_LEN];					// 客户端 IP 字符串缓存
	std::string m_strUrl;							// 请求 URL 或 PATH
	std::string m_strRawUrl;						// 请求行原始 URL，保留 query 供业务层兼容旧 GET 入参
	std::string m_strQueryString;					// 原始 query 字符串，不包含问号
	std::string m_strMethod;						// HTTP 请求方法
	std::string m_strContent;						// 按片段累计后的请求 BODY
	std::string m_strRequestRejectReason;			// 网络层拒绝原因，不包含请求参数内容
	HttpStatusType m_enRequestRejectStatus;			// 网络层拒绝使用的 HTTP 状态码
	size_t m_uiRequestHeadCount;					// 已接收请求头数量
	size_t m_uiRequestHeadBytes;					// 已接收请求头累计字节数
	std::map<std::string, std::string> m_mapRequestHead;	// 请求头表，key 为归一化后的小写头名
	std::vector<ST_HTTP_QUERY_PARAM> m_vecQueryParam;		// URL query 参数表，保持原始顺序且只在派发前写入
	std::vector<HttpHeaderItem> m_vecResponseHead;	// 待发送的响应头列表，保持添加顺序
};

#endif
