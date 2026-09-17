#ifndef H_CLOUDNETDATA_API
#define H_CLOUDNETDATA_API

#if defined(OS_IS_WINDOWS) || defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN64) || defined(_INC_WINDOWS)
#if defined(CLOUDNETDATAAPI_EXPORTS)
// DLL 内部通过 .def 控制导出名，避免额外导出 C++ 装饰名。
#define CLOUDNETDATA_API
#else
// 调用方包含本头文件时，使用导入声明匹配 libCloudNetDataApi.lib。
#define CLOUDNETDATA_API __declspec(dllimport)
#endif
#else
#define CLOUDNETDATA_API
#endif

// Windows.h 会把 StartService 映射为 StartServiceA/W，本库导出必须保留无前缀原名。
#ifdef StartService
#undef StartService
#endif

#ifdef __cplusplus
extern "C"
{
#endif

// CloudNetDataApi 对外只暴露不透明句柄，内部实现不进入业务工程源码。
typedef void* HCLOUD_NET_API;
// 延迟二进制应答句柄只用于 CompleteBinaryResponse/CancelBinaryResponse。
typedef void* HCLOUD_NET_BINARY_RESPONSE;

// 二进制缓冲区描述，调用方传入时不转移所有权，返回结果中由 FreeResult 释放。
typedef struct ST_CLOUD_NET_BUFFER
{
	long lLen;                    // 缓冲区有效字节数，单位为字节，允许为 0。
	unsigned char* pBuffer;       // 缓冲区起始地址，返回结果中归 CloudNetDataApi 管理。
#ifdef __cplusplus
	ST_CLOUD_NET_BUFFER()
	{
		lLen = 0;
		pBuffer = 0;
	}
#endif
} ST_CLOUD_NET_BUFFER;

// RPC/PUT 调用和服务端处理返回的统一结果。
typedef struct ST_CLOUD_NET_RESULT
{
	long long lRetVal;            // 主返回值，0 或正数表示业务成功，负数表示网络或业务异常。
	long long lParam;             // 第一组业务整型返回参数，具体含义由功能号约定。
	long long wParam;             // 第二组业务整型返回参数，具体含义由功能号约定。
	long long lSynId;             // 请求同步序号，用于调用方匹配请求和响应。
	long long lFuncId;            // 业务功能号，用于服务端分发处理逻辑。
	ST_CLOUD_NET_BUFFER stLParam; // 第一组二进制返回参数，内存由 FreeResult 释放。
	ST_CLOUD_NET_BUFFER stWParam; // 第二组二进制返回参数，内存由 FreeResult 释放。
	ST_CLOUD_NET_BUFFER stJsonReq;// 原始请求 JSON 备份，异步回调中用于调用方识别请求。
	char szErrInfo[64];           // 错误信息，最多保存 63 字节并以 0 结尾。
	void* pParam;                 // 调用方透传上下文，异步回调时原样返回。
#ifdef __cplusplus
	ST_CLOUD_NET_RESULT()
	{
		lRetVal = 0;
		lParam = 0;
		wParam = 0;
		lSynId = 0;
		lFuncId = 0;
		for (int iIndex = 0; iIndex < 64; ++iIndex)
		{
			szErrInfo[iIndex] = 0;
		}
		pParam = 0;
	}
#endif
} ST_CLOUD_NET_RESULT;

// 服务端收到的统一请求对象，仅在请求回调期间有效。
typedef struct ST_CLOUD_NET_REQUEST
{
	long long lSynId;             // 请求同步序号，回包时应原样返回。
	long long lFuncId;            // 业务功能号，服务端按该字段选择处理流程。
	long long lSetCode;           // 业务集合或市场编码，含义由调用双方约定。
	long long lParam;             // PUT 请求携带的第一组业务整型参数。
	long long wParam;             // PUT 请求携带的第二组业务整型参数。
	short chMode;                 // 请求模式，0 表示 RPC，1 表示 PUT。
	ST_CLOUD_NET_BUFFER stJsonReq;// 请求 JSON 数据，只在请求回调期间有效。
	ST_CLOUD_NET_BUFFER stLParam; // PUT 请求携带的第一组二进制参数，只在回调期间有效。
	ST_CLOUD_NET_BUFFER stWParam; // PUT 请求携带的第二组二进制参数，只在回调期间有效。
	void* pParam;                 // StartService 传入的调用方上下文。
#ifdef __cplusplus
	ST_CLOUD_NET_REQUEST()
	{
		lSynId = 0;
		lFuncId = 0;
		lSetCode = 0;
		lParam = 0;
		wParam = 0;
		chMode = 0;
		pParam = 0;
	}
#endif
} ST_CLOUD_NET_REQUEST;

// 二进制只读视图；调用期间由调用方持有，CloudNetDataApi 不接管内存。
typedef struct ST_CLOUD_NET_BINARY_VIEW
{
	int iLen;                         // pBuffer 的有效字节数，单位为字节，允许为 0。
	const unsigned char* pBuffer;     // 二进制数据起始地址，iLen 大于 0 时不能为空。
#ifdef __cplusplus
	ST_CLOUD_NET_BINARY_VIEW()
	{
		iLen = 0;
		pBuffer = 0;
	}
#endif
} ST_CLOUD_NET_BINARY_VIEW;

// 二进制结果缓冲区；同步结果中的内存由 FreeBinaryResult 统一释放。
typedef struct ST_CLOUD_NET_BINARY_BUFFER
{
	int iLen;                         // pBuffer 的有效字节数，单位为字节，允许为 0。
	unsigned char* pBuffer;           // 二进制数据起始地址，所有权由所属结果对象持有。
#ifdef __cplusplus
	ST_CLOUD_NET_BINARY_BUFFER()
	{
		iLen = 0;
		pBuffer = 0;
	}
#endif
} ST_CLOUD_NET_BINARY_BUFFER;

// BinaryPayload 编码视图；正文保持原始或 Snappy 状态，所有权由所属编码结果持有。
typedef struct ST_CLOUD_NET_BINARY_ENCODED_BUFFER
{
	int iVersion;                     // BinaryPayload 协议版本，当前固定为 1。
	int iCompression;                 // 0 表示原始二进制，1 表示 Snappy。
	int iRawSize;                     // 解压后的原始字节数。
	int iWireSize;                    // pBuffer 的有效编码字节数。
	int iMaxPayloadBytes;             // 接收连接允许的最大原始载荷，用于延迟解压校验。
	const unsigned char* pBuffer;     // 编码正文，只在所属结果释放前有效。
#ifdef __cplusplus
	ST_CLOUD_NET_BINARY_ENCODED_BUFFER()
	{
		iVersion = 0;
		iCompression = 0;
		iRawSize = 0;
		iWireSize = 0;
		iMaxPayloadBytes = 0;
		pBuffer = 0;
	}
#endif
} ST_CLOUD_NET_BINARY_ENCODED_BUFFER;

// 客户端二进制调用参数；RPC 使用主载荷，PUT 还可携带扩展载荷和整型参数。
typedef struct ST_CLOUD_NET_BINARY_CALL
{
	int iVersion;                         // 调用结构版本，当前必须为 1。
	long long lSynId;                     // 请求序号，用于调用链跟踪和异步响应匹配。
	long long lFuncId;                    // 业务功能号，由六插件共享二进制协议定义。
	long long lRouteCode;                 // 路由编码，可表示 MT 服务器、市场或业务分片。
	long long lParam;                     // 第一组业务整型参数，不使用时设置为 0。
	ST_CLOUD_NET_BINARY_VIEW stPayload;   // 主请求载荷，允许包含零字节和任意二进制内容。
	long long wParam;                     // 第二组业务整型参数，不使用时设置为 0。
	ST_CLOUD_NET_BINARY_VIEW stExtra;     // 可选扩展请求载荷，主要供 PUT 使用。
#ifdef __cplusplus
	ST_CLOUD_NET_BINARY_CALL()
	{
		iVersion = 1;
		lSynId = 0;
		lFuncId = 0;
		lRouteCode = 0;
		lParam = 0;
		wParam = 0;
	}
#endif
} ST_CLOUD_NET_BINARY_CALL;

// 服务端收到的二进制请求；所有缓冲区只在请求回调期间有效。
typedef struct ST_CLOUD_NET_BINARY_REQUEST
{
	int iVersion;                         // 请求结构版本，当前为 1。
	long long lSynId;                     // 客户端请求序号，服务端无需自行生成。
	long long lFuncId;                    // 业务功能号，服务端按该字段分发处理器。
	long long lRouteCode;                 // 客户端传入的路由编码。
	long long lParam;                     // 第一组业务整型输入参数。
	ST_CLOUD_NET_BINARY_VIEW stPayload;   // 已完成 Snappy 解压的主请求载荷。
	long long wParam;                     // 第二组业务整型输入参数。
	ST_CLOUD_NET_BINARY_VIEW stExtra;     // 已完成 Snappy 解压的扩展请求载荷。
	short chMode;                         // 请求模式，0 表示 Binary RPC，1 表示 Binary PUT。
	void* pParam;                         // StartBinaryService/StartBinaryNetwork 传入的上层上下文。
	HCLOUD_NET_BINARY_RESPONSE hResponse; // Ex 回调可保存的不透明引用上下文；完成后不得解引用或复用。
#ifdef __cplusplus
	ST_CLOUD_NET_BINARY_REQUEST()
	{
		iVersion = 1;
		lSynId = 0;
		lFuncId = 0;
		lRouteCode = 0;
		lParam = 0;
		wParam = 0;
		chMode = 0;
		pParam = 0;
		hResponse = 0;
	}
#endif
} ST_CLOUD_NET_BINARY_REQUEST;

// 二进制调用结果；主载荷和扩展载荷均为解压后的原始数据。
typedef struct ST_CLOUD_NET_BINARY_RESULT
{
	int iVersion;                         // 结果结构版本，当前为 1。
	long long lRetVal;                    // 业务返回值，0 或正数表示业务成功，负数表示失败。
	int iErrorCode;                       // 网络层详细错误码，0 表示传输和协议处理成功。
	long long lSynId;                     // 对应请求序号，便于异步回调匹配调用。
	long long lFuncId;                    // 对应业务功能号，便于异步回调分发结果。
	long long lParam;                     // 第一组业务整型返回参数。
	ST_CLOUD_NET_BINARY_BUFFER stPayload; // 解压后的主返回载荷。
	long long wParam;                     // 第二组业务整型返回参数。
	ST_CLOUD_NET_BINARY_BUFFER stExtra;   // 解压后的扩展返回载荷。
	char szErrInfo[256];                  // 详细英文错误描述，最多保存 255 字节并以 0 结尾。
	void* pParam;                         // 异步调用方透传上下文，回调时原样返回。
#ifdef __cplusplus
	ST_CLOUD_NET_BINARY_RESULT()
	{
		iVersion = 1;
		lRetVal = 0;
		iErrorCode = 0;
		lSynId = 0;
		lFuncId = 0;
		lParam = 0;
		wParam = 0;
		for (int iIndex = 0; iIndex < 256; ++iIndex)
		{
			szErrInfo[iIndex] = 0;
		}
		pParam = 0;
	}
#endif
} ST_CLOUD_NET_BINARY_RESULT;

// 拥有型编码异步结果；回调取得所有权，必须交给 FreeBinaryEncodedResult 释放。
// CloudNetDataApi 只运输 BinaryPayload，不解释插件业务协议，也不提前解压正文。
typedef struct ST_CLOUD_NET_BINARY_ENCODED_RESULT
{
	int iVersion;                         // 结果结构版本，当前为 1。
	long long lRetVal;                    // 业务返回值，语义与 ST_CLOUD_NET_BINARY_RESULT 一致。
	int iErrorCode;                       // 网络层错误码，0 表示运输成功。
	long long lSynId;                     // 对应请求序号，供异步上下文核对。
	long long lFuncId;                    // 对应业务功能号，供异步上下文核对。
	long long lParam;                     // 第一组业务整型返回参数。
	ST_CLOUD_NET_BINARY_ENCODED_BUFFER stPayload; // 仍保持编码状态的主返回载荷。
	long long wParam;                     // 第二组业务整型返回参数。
	ST_CLOUD_NET_BINARY_ENCODED_BUFFER stExtra; // 仍保持编码状态的扩展返回载荷。
	char szErrInfo[256];                  // 详细英文错误描述，最多保存 255 字节并以 0 结尾。
	void* pParam;                         // 异步调用方透传上下文，回调时原样返回。
	void* pInternalOwner;                 // DLL 私有所有权句柄，调用方禁止访问。
#ifdef __cplusplus
	ST_CLOUD_NET_BINARY_ENCODED_RESULT()
	{
		iVersion = 1;
		lRetVal = 0;
		iErrorCode = 0;
		lSynId = 0;
		lFuncId = 0;
		lParam = 0;
		wParam = 0;
		for (int iIndex = 0; iIndex < 256; ++iIndex)
		{
			szErrInfo[iIndex] = 0;
		}
		pParam = 0;
		pInternalOwner = 0;
	}
#endif
} ST_CLOUD_NET_BINARY_ENCODED_RESULT;

// 服务端请求回调：调用方在回调内填充 p_pResult，返回后库内部立即回包。
typedef void (*PFN_CLOUD_NET_REQUEST)(const ST_CLOUD_NET_REQUEST* p_pRequest, ST_CLOUD_NET_RESULT* p_pResult, void* p_pParam);
// 异步 RPC/PUT 回调：p_pResult 只在回调期间有效，若需长期保存应自行拷贝。
typedef void (*PFN_CLOUD_NET_ASYNC)(const ST_CLOUD_NET_RESULT* p_pResult, void* p_pParam);
// 推送回调：p_pBuf 只在回调期间有效，调用方如需异步处理应自行拷贝。
typedef void (*PFN_CLOUD_NET_PUSH)(long long p_lReqNo, const char* p_pBuf, int p_iLen, void* p_pParam);
// 二进制服务端请求回调：在回调内填充结果，缓冲区只需保持到回调返回。
typedef void (*PFN_CLOUD_NET_BINARY_REQUEST)(const ST_CLOUD_NET_BINARY_REQUEST* p_pRequest, ST_CLOUD_NET_BINARY_RESULT* p_pResult, void* p_pParam);
// Ex 服务端回调结果；COMPLETED 由库使用 p_pResult 立即应答，DEFERRED 由调用方保存 hResponse 后异步应答。
typedef enum EN_CLOUD_NET_BINARY_SERVER_CALLBACK_RESULT
{
	EN_CLOUD_NET_BINARY_SERVER_CALLBACK_COMPLETED = 0,
	EN_CLOUD_NET_BINARY_SERVER_CALLBACK_DEFERRED = 1
} EN_CLOUD_NET_BINARY_SERVER_CALLBACK_RESULT;
// 支持延迟应答的二进制服务端回调；异步阶段必须深拷贝请求字段和缓冲区。
typedef int (*PFN_CLOUD_NET_BINARY_REQUEST_EX)(const ST_CLOUD_NET_BINARY_REQUEST* p_pRequest, ST_CLOUD_NET_BINARY_RESULT* p_pResult, void* p_pParam);
// 控制面优先分类器；返回非零表示控制请求，CloudNetDataApi 不解释 FuncId。
typedef int (*PFN_CLOUD_NET_BINARY_PRIORITY_CLASSIFIER)(long long p_lFuncId, void* p_pParam);
// 二进制异步结果回调：结果和缓冲区仅在回调期间有效，长期保存时必须自行深拷贝。
typedef void (*PFN_CLOUD_NET_BINARY_ASYNC)(const ST_CLOUD_NET_BINARY_RESULT* p_pResult, void* p_pParam);
// 拥有型编码结果回调：回调取得 p_pResult 所有权，可跨回调保存并异步释放。
typedef void (*PFN_CLOUD_NET_BINARY_ENCODED_ASYNC)(ST_CLOUD_NET_BINARY_ENCODED_RESULT* p_pResult, void* p_pParam);

CLOUDNETDATA_API HCLOUD_NET_API Create();
// 销毁 CloudNetDataApi 实例，并释放其内部服务端和客户端连接。
CLOUDNETDATA_API void Destroy(HCLOUD_NET_API p_hApi);
// 加载 CloudNetDataApi 组网配置，p_szXmlPath 为空时默认读取当前目录下的 IceRPCPush.xml。
CLOUDNETDATA_API int LoadConfig(HCLOUD_NET_API p_hApi, const char* p_szXmlPath, const char* p_szPluginId);
// 重新加载上一次 LoadConfig 使用的配置文件，并让后台维护线程按新服务表继续工作。
CLOUDNETDATA_API int ReloadConfig(HCLOUD_NET_API p_hApi);
// 按 XML 配置启动本地服务、自动连接远端服务，并启动后台重连和推送续约线程。
CLOUDNETDATA_API int StartNetwork(HCLOUD_NET_API p_hApi, PFN_CLOUD_NET_REQUEST p_pfnRequest, PFN_CLOUD_NET_PUSH p_pfnPush, void* p_pParam);
// 按 XML 配置启动二进制本地服务、远端连接和后台重连；推送订阅仍使用现有 Push 回调。
CLOUDNETDATA_API int StartBinaryNetwork(HCLOUD_NET_API p_hApi, PFN_CLOUD_NET_BINARY_REQUEST p_pfnRequest, PFN_CLOUD_NET_PUSH p_pfnPush, void* p_pParam);
// 启动支持延迟应答和控制优先级的二进制组网，旧 StartBinaryNetwork 保持同步回调语义。
CLOUDNETDATA_API int StartBinaryNetworkEx(HCLOUD_NET_API p_hApi, PFN_CLOUD_NET_BINARY_REQUEST_EX p_pfnRequest, PFN_CLOUD_NET_BINARY_PRIORITY_CLASSIFIER p_pfnPriorityClassifier, PFN_CLOUD_NET_PUSH p_pfnPush, void* p_pParam);
// 停止组网维护线程，注销推送订阅，断开连接并停止本地服务。
CLOUDNETDATA_API int StopNetwork(HCLOUD_NET_API p_hApi);
// 通过本地服务端句柄向已注册客户端发布推送数据，p_iAsync 非 0 表示异步推送。
CLOUDNETDATA_API long long Publish(HCLOUD_NET_API p_hApi, long long p_lReqNo, const char* p_pBuf, long p_lBufLen, int p_iAsync);
// 启动 Ice 服务端，并注册同步请求回调，p_iSnappyCompress 非 0 表示启用压缩。
CLOUDNETDATA_API int StartService(HCLOUD_NET_API p_hApi, const char* p_szXmlPath, const char* p_szServiceName, int p_iSnappyCompress, PFN_CLOUD_NET_REQUEST p_pfnCallback, void* p_pParam);
// 启动二进制 Ice 服务端；业务回调不接触 IceRPCPush 句柄或生成代码。
CLOUDNETDATA_API int StartBinaryService(HCLOUD_NET_API p_hApi, const char* p_szXmlPath, const char* p_szServiceName, int p_iSnappyCompress, PFN_CLOUD_NET_BINARY_REQUEST p_pfnCallback, void* p_pParam);
// 启动支持延迟应答和控制优先队列的二进制服务端。
CLOUDNETDATA_API int StartBinaryServiceEx(HCLOUD_NET_API p_hApi, const char* p_szXmlPath, const char* p_szServiceName, int p_iSnappyCompress, PFN_CLOUD_NET_BINARY_REQUEST_EX p_pfnCallback, PFN_CLOUD_NET_BINARY_PRIORITY_CLASSIFIER p_pfnPriorityClassifier, void* p_pParam);
// 停止当前实例中的 Ice 服务端。
CLOUDNETDATA_API int StopService(HCLOUD_NET_API p_hApi);
// 创建到远端 Ice 服务的客户端连接，p_szConnName 是本库内部连接名。
CLOUDNETDATA_API int Connect(HCLOUD_NET_API p_hApi, const char* p_szXmlPath, const char* p_szProxyName, const char* p_szConnName, int p_iThreadPool);
// 断开指定客户端连接。
CLOUDNETDATA_API int Disconnect(HCLOUD_NET_API p_hApi, const char* p_szConnName);
// 注册推送回调和订阅信息。
CLOUDNETDATA_API int RegisterPush(HCLOUD_NET_API p_hApi, const char* p_szConnName, const char* p_szPluginId, const char* p_szSubInfo, PFN_CLOUD_NET_PUSH p_pfnCallback, void* p_pParam);
// 注销推送订阅。
CLOUDNETDATA_API int UnregisterPush(HCLOUD_NET_API p_hApi, const char* p_szConnName, const char* p_szPluginId, const char* p_szSubInfo);
// 同步 RPC 调用，返回对象必须由 FreeResult 释放。
CLOUDNETDATA_API ST_CLOUD_NET_RESULT* CallSync(HCLOUD_NET_API p_hApi, const char* p_szConnName, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lJsonLen);
// 异步 RPC 调用，返回值为底层拥塞计数或负数错误码。
CLOUDNETDATA_API long long CallAsync(HCLOUD_NET_API p_hApi, const char* p_szConnName, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lJsonLen, PFN_CLOUD_NET_ASYNC p_pfnCallback, void* p_pParam);
// 同步 PUT 调用，p_pPutData 只读取其中 lParam/wParam/stLParam/stWParam。
CLOUDNETDATA_API ST_CLOUD_NET_RESULT* PutSync(HCLOUD_NET_API p_hApi, const char* p_szConnName, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lJsonLen, const ST_CLOUD_NET_RESULT* p_pPutData);
// 异步 PUT 调用，返回值为底层拥塞计数或负数错误码。
CLOUDNETDATA_API long long PutAsync(HCLOUD_NET_API p_hApi, const char* p_szConnName, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lJsonLen, const ST_CLOUD_NET_RESULT* p_pPutData, PFN_CLOUD_NET_ASYNC p_pfnCallback, void* p_pParam);
// 同步二进制 RPC，返回对象必须使用 FreeBinaryResult 释放。
CLOUDNETDATA_API ST_CLOUD_NET_BINARY_RESULT* CallBinarySync(HCLOUD_NET_API p_hApi, const char* p_szConnName, const ST_CLOUD_NET_BINARY_CALL* p_pCall);
// 异步二进制 RPC；提交成功返回正数，提交失败返回负数且不会触发回调。
CLOUDNETDATA_API long long CallBinaryAsync(HCLOUD_NET_API p_hApi, const char* p_szConnName, const ST_CLOUD_NET_BINARY_CALL* p_pCall, PFN_CLOUD_NET_BINARY_ASYNC p_pfnCallback, void* p_pParam);
// 带单次超时覆盖的异步二进制 RPC；0 表示无限等待，不修改连接的全局超时配置。
CLOUDNETDATA_API long long CallBinaryAsyncEx(HCLOUD_NET_API p_hApi, const char* p_szConnName, const ST_CLOUD_NET_BINARY_CALL* p_pCall, int p_iTimeoutMs, PFN_CLOUD_NET_BINARY_ASYNC p_pfnCallback, void* p_pParam);
// 同步二进制 PUT，返回对象必须使用 FreeBinaryResult 释放。
CLOUDNETDATA_API ST_CLOUD_NET_BINARY_RESULT* PutBinarySync(HCLOUD_NET_API p_hApi, const char* p_szConnName, const ST_CLOUD_NET_BINARY_CALL* p_pCall);
// 异步二进制 PUT；提交成功返回正数，异步结果只在回调期间有效。
CLOUDNETDATA_API long long PutBinaryAsync(HCLOUD_NET_API p_hApi, const char* p_szConnName, const ST_CLOUD_NET_BINARY_CALL* p_pCall, PFN_CLOUD_NET_BINARY_ASYNC p_pfnCallback, void* p_pParam);
// 带单次超时覆盖的异步二进制 PUT；0 表示无限等待。
CLOUDNETDATA_API long long PutBinaryAsyncEx(HCLOUD_NET_API p_hApi, const char* p_szConnName, const ST_CLOUD_NET_BINARY_CALL* p_pCall, int p_iTimeoutMs, PFN_CLOUD_NET_BINARY_ASYNC p_pfnCallback, void* p_pParam);
// 拥有型编码异步 RPC；回调直接取得未解压 BinaryPayload，0 表示无限等待。
CLOUDNETDATA_API long long CallBinaryEncodedAsyncEx(HCLOUD_NET_API p_hApi, const char* p_szConnName, const ST_CLOUD_NET_BINARY_CALL* p_pCall, int p_iTimeoutMs, PFN_CLOUD_NET_BINARY_ENCODED_ASYNC p_pfnCallback, void* p_pParam);
// 拥有型编码异步 PUT；所有权和超时语义与 CallBinaryEncodedAsyncEx 一致。
CLOUDNETDATA_API long long PutBinaryEncodedAsyncEx(HCLOUD_NET_API p_hApi, const char* p_szConnName, const ST_CLOUD_NET_BINARY_CALL* p_pCall, int p_iTimeoutMs, PFN_CLOUD_NET_BINARY_ENCODED_ASYNC p_pfnCallback, void* p_pParam);
// 完成延迟二进制应答；成功返回 1，重复、过期或非法句柄返回 0。
CLOUDNETDATA_API int CompleteBinaryResponse(HCLOUD_NET_BINARY_RESPONSE p_hResponse, const ST_CLOUD_NET_BINARY_RESULT* p_pResult);
// 使用指定错误完成延迟应答，供超时、取消和停机的上层收口路径使用。
CLOUDNETDATA_API int CancelBinaryResponse(HCLOUD_NET_BINARY_RESPONSE p_hResponse, int p_iErrorCode, const char* p_szError);
// 释放 CallSync/PutSync 返回的结果对象。
CLOUDNETDATA_API void FreeResult(ST_CLOUD_NET_RESULT* p_pResult);
// 释放 CallBinarySync/PutBinarySync 返回的二进制结果对象。
CLOUDNETDATA_API void FreeBinaryResult(ST_CLOUD_NET_BINARY_RESULT* p_pResult);
// 在最终消费线程校验并解码一个 BinaryPayload；成功后缓冲由 FreeBinaryBuffer 释放。
CLOUDNETDATA_API int DecodeBinaryEncodedBuffer(const ST_CLOUD_NET_BINARY_ENCODED_BUFFER* p_pEncoded, ST_CLOUD_NET_BINARY_BUFFER* p_pDecoded, char* p_szError, int p_iErrorCapacity);
// 释放 DecodeBinaryEncodedBuffer 分配的原始缓冲并清空视图。
CLOUDNETDATA_API void FreeBinaryBuffer(ST_CLOUD_NET_BINARY_BUFFER* p_pBuffer);
// 释放编码异步回调取得的结果及 payload/extra 底层所有权。
CLOUDNETDATA_API void FreeBinaryEncodedResult(ST_CLOUD_NET_BINARY_ENCODED_RESULT* p_pResult);
// 获取当前实例最后一次错误信息；p_hApi 为空时读取当前线程错误。
CLOUDNETDATA_API const char* GetLastErrorDetail(HCLOUD_NET_API p_hApi);
// 获取当前实例最后一次错误码；p_hApi 为空时读取当前线程错误。
CLOUDNETDATA_API int GetLastErrorCode(HCLOUD_NET_API p_hApi);
// 根据错误码获取默认英文错误描述；不依赖实例句柄。
CLOUDNETDATA_API const char* GetErrorMsg(int p_iErrorCode);

#ifdef __cplusplus
}
#endif

#endif // H_CLOUDNETDATA_API
