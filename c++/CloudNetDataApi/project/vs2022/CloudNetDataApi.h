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

// 二进制缓冲区描述，调用方传入时不转移所有权，返回结果中由 FreeResult 释放。
typedef struct ST_CLOUD_NET_BUFFER
{
	long lLen;                    // 缓冲区有效字节数，单位为字节，允许为 0。
	unsigned char* pBuffer;       // 缓冲区起始地址，返回结果中归 CloudNetDataApi 管理。
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
} ST_CLOUD_NET_REQUEST;

// 服务端请求回调：调用方在回调内填充 p_pResult，返回后库内部立即回包。
typedef void (*PFN_CLOUD_NET_REQUEST)(const ST_CLOUD_NET_REQUEST* p_pRequest, ST_CLOUD_NET_RESULT* p_pResult, void* p_pParam);
// 异步 RPC/PUT 回调：p_pResult 只在回调期间有效，若需长期保存应自行拷贝。
typedef void (*PFN_CLOUD_NET_ASYNC)(const ST_CLOUD_NET_RESULT* p_pResult, void* p_pParam);
// 推送回调：p_pBuf 只在回调期间有效，调用方如需异步处理应自行拷贝。
typedef void (*PFN_CLOUD_NET_PUSH)(long long p_lReqNo, const char* p_pBuf, int p_iLen, void* p_pParam);

// 创建 CloudNetDataApi 实例，返回句柄由 Destroy 释放。
CLOUDNETDATA_API HCLOUD_NET_API Create();
// 销毁 CloudNetDataApi 实例，并释放其内部服务端和客户端连接。
CLOUDNETDATA_API void Destroy(HCLOUD_NET_API p_hApi);
// 启动 Ice 服务端，并注册同步请求回调，p_iSnappyCompress 非 0 表示启用压缩。
CLOUDNETDATA_API int StartService(HCLOUD_NET_API p_hApi, const char* p_szXmlPath, const char* p_szServiceName, int p_iSnappyCompress, PFN_CLOUD_NET_REQUEST p_pfnCallback, void* p_pParam);
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
// 释放 CallSync/PutSync 返回的结果对象。
CLOUDNETDATA_API void FreeResult(ST_CLOUD_NET_RESULT* p_pResult);
// 获取当前实例最后一次错误信息，返回指针由实例维护。
CLOUDNETDATA_API const char* GetLastError(HCLOUD_NET_API p_hApi);

#ifdef __cplusplus
}
#endif

#endif // H_CLOUDNETDATA_API