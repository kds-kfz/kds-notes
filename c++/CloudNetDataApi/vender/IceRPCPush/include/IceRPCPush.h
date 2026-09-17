#ifndef H_INTERFACE_ICE_RPC_PUSH
#define H_INTERFACE_ICE_RPC_PUSH

#if defined(OS_IS_WINDOWS) || defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN64)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#else
#ifndef NULL
#define NULL 0
#endif
typedef void* HANDLE;
typedef unsigned long (*LPTHREAD_START_ROUTINE)(void*);
#endif

#include <cstring>
#include <string>

#if defined(OS_IS_WINDOWS) || defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN64) || defined(_INC_WINDOWS)
#if defined(ICERPCPUSH_EXPORTS)
// DLL 内部通过 .def 控制导出名，避免额外导出 C++ 装饰名。
#define ICERPCPUSH_API
#else
// 调用方包含本头文件时，使用导入声明匹配 libIceRPCPush.lib。
#define ICERPCPUSH_API __declspec(dllimport)
#endif
#else
#define ICERPCPUSH_API
#endif

/*
	IceRPCPush 是 Ice RPC/PUT 的 DLL 封装，服务端使用异步派发把请求放入队列，上层自行开线程处理业务。
	客户端推荐使用异步或 Pre/End 半异步模式，阻塞模式仅用于简单调用或兼容旧代码。
	RPC 以请求服务端数据为主，PUT 以向服务端上传二进制数据为主；结构体采用 pack(1) 保持旧 ABI 二进制布局。
	异步调用前，调用方必须保证 ST_JSON_M_RESULT_TOP 以及其中指针在回调返回前持续有效。
*/

#pragma pack(push,1)

// 服务端通过特殊功能号把客户端注册、订阅、断开事件回传给上层。
const long long NOTIFY_ADD_CLIENT = 0x1FFFFFFFFFFFFFF0;
const long long NOTIFY_SUB_CLIENT = 0x1FFFFFFFFFFFFFF1;
const long long NOTIFY_UNSUB_CLIENT = 0x1FFFFFFFFFFFFFF2;
const long long NOTIFY_DEL_CLIENT = 0x1FFFFFFFFFFFFFFF;

// 推送注册通知携带客户端句柄和订阅内容，尾部 szData 是变长缓冲区。
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4200)
#endif
struct ST_NOTIFY_CLIENT_INFO
{
	HANDLE hClient;              // 客户端句柄，服务端后续主动推送或断开通知时使用。
	long long lReserve;          // 保留字段，兼容旧协议扩展，当前保持为 0。
	long lLen;                   // szData 的有效字节数，单位为字节。
	char szData[0];              // 变长订阅数据起始地址，实际空间跟随结构体一起分配。
	ST_NOTIFY_CLIENT_INFO()
	{
		memset(this, 0, sizeof(ST_NOTIFY_CLIENT_INFO));
	}
};
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

// 客户端收到服务端主动推送时触发，p_pParam 原样回传注册时的调用方上下文。
typedef void(*func_JsonICEPushClientPack)(long long p_lReqNo, const char* p_pBuf, int p_iLen, void* p_pParam);

// 请求类型，保持旧协议整数值不变。
enum EN_JSON_INPUT_TYPE
{
	EN_JSON_INPUT_RPC = 0,
	EN_JSON_INPUT_PUT = 1,
};

// 二进制请求类型，和旧 JSON 请求类型使用不同取值，避免服务端回调误判。
enum EN_BINARY_INPUT_TYPE
{
	EN_BINARY_INPUT_RPC = 2,
	EN_BINARY_INPUT_PUT = 3,
};

// DLL 内部句柄类型，区分 RPC 对象和服务端保存的客户端对象。
enum EN_JSON_HANDLE_TYPE
{
	EN_JSON_HANDLE_RPC = 0,
	EN_JSON_HANDLE_CLIENT = 1,
};

// DLL 对外暴露的二进制缓冲区描述，只借用 pBuffer 指针，不隐含所有权。
struct ST_RESULT_BUF
{
	int lLen;                    // pBuffer 指向缓冲区的有效字节数，允许为 0。
	unsigned char* pBuffer;      // 二进制缓冲区指针，所有权由具体接口说明决定。
	ST_RESULT_BUF()
	{
		memset(this, 0, sizeof(ST_RESULT_BUF));
	}
};

// 多返回值结构用于 RPC/PUT 结果和回调上下文，上层负责保存业务含义。
struct ST_JSON_M_RESULT_TOP
{
	long long lRetVal;           // RPC/PUT 主返回值，0 或正数表示业务成功，负数表示异常。
	long long lParam;            // 第一组业务整型返回参数，具体含义由 FuncId 约定。
	long long wParam;            // 第二组业务整型返回参数，具体含义由 FuncId 约定。
	ST_RESULT_BUF stLParam;      // 第一组二进制返回参数，通常为压缩后的业务数据。
	ST_RESULT_BUF stWParam;      // 第二组二进制返回参数，通常为附加业务数据。
	char szErrInfo[64];          // 错误信息缓冲区，最多保存 63 字节并以 0 结尾。
	void* pParam;                // 调用方透传上下文，异步回调时原样返回。
	long long lSynId;            // 调用方传入的同步序号，用于匹配请求和响应。
	long long lFuncId;           // 业务功能号，用于服务端分发处理逻辑。
	ST_RESULT_BUF stJsonReq;     // 原始或压缩后的请求 JSON 缓冲，生命周期由结果对象维护。
	ST_JSON_M_RESULT_TOP()
	{
		memset(this, 0, sizeof(ST_JSON_M_RESULT_TOP));
	}
};

// 底层分配的结果节点，调用方必须使用 IJsonMutiResultFree 释放。
struct ST_JSON_M_RESULT_LEVEL : ST_JSON_M_RESULT_TOP
{
	ST_JSON_M_RESULT_LEVEL()
	{
		memset(this, 0, sizeof(ST_JSON_M_RESULT_LEVEL));
	}
};

// 服务端队列节点，保存一次 RPC/PUT 请求及回包所需的句柄上下文。
struct ST_JSON_INPUT
{
	long long lSynId;            // 调用方传入的同步序号，回包时原样返回。
	long long lFuncId;           // 业务功能号，服务端按该字段选择处理流程。
	long long lSetCode;          // 业务集合或市场编码，旧接口沿用该名称。
	ST_RESULT_BUF stJsonReq;     // 请求 JSON 数据缓冲，节点释放时由 DLL 内部回收。
	HANDLE hSelf;                // 当前客户端句柄，服务端可用它向指定客户端反向推送。
	char chMode;                 // 请求模式，取值为 EN_JSON_INPUT_RPC 或 EN_JSON_INPUT_PUT。
	long long lParam;            // PUT 请求携带的第一组业务整型参数。
	long long wParam;            // PUT 请求携带的第二组业务整型参数。
	ST_RESULT_BUF stLParam;      // PUT 请求携带的第一组二进制参数。
	ST_RESULT_BUF stWParam;      // PUT 请求携带的第二组二进制参数。
	ST_JSON_INPUT()
	{
		memset(this, 0, sizeof(ST_JSON_INPUT));
	}
	~ST_JSON_INPUT()
	{
	}
};

// 二进制只读缓冲区；调用期间由调用方持有，DLL 不接管其内存。
struct ST_BINARY_VIEW
{
	int lLen;                         // pBuffer 的有效字节数，允许为 0。
	const unsigned char* pBuffer;     // 二进制数据起始地址，长度大于 0 时不能为空。
	ST_BINARY_VIEW()
	{
		memset(this, 0, sizeof(ST_BINARY_VIEW));
	}
};

// 二进制返回缓冲区；同步结果中的内存统一由 BinaryResultFree 释放。
struct ST_BINARY_BUFFER
{
	int lLen;                         // pBuffer 的有效字节数，允许为 0。
	unsigned char* pBuffer;           // 二进制数据起始地址，所有权由结果对象持有。
	ST_BINARY_BUFFER()
	{
		memset(this, 0, sizeof(ST_BINARY_BUFFER));
	}
};

// 客户端通用二进制调用参数；RPC 使用主载荷，PUT 还可以使用扩展载荷和整型参数。
struct ST_BINARY_CALL
{
	int iVersion;                     // 调用结构版本，当前固定为 1。
	long long lSynId;                 // 调用方生成的请求序号，用于日志跟踪和响应匹配。
	long long lFuncId;                // 业务功能号，由共享业务协议定义。
	long long lRouteCode;             // 路由编码，可表示服务器、市场或业务分片。
	long long lParam;                 // 第一组业务整型参数，RPC 不需要时保持为 0。
	ST_BINARY_VIEW stPayload;         // 主请求载荷，允许包含任意二进制字节。
	long long wParam;                 // 第二组业务整型参数，RPC 不需要时保持为 0。
	ST_BINARY_VIEW stExtra;           // 可选扩展载荷，主要供 PUT 使用。
	ST_BINARY_CALL()
	{
		memset(this, 0, sizeof(ST_BINARY_CALL));
		iVersion = 1;
	}
};

// 二进制调用结果；主载荷和扩展载荷均为解压后的原始数据。
struct ST_BINARY_RESULT
{
	int iVersion;                     // 结果结构版本，当前固定为 1。
	long long lRetVal;                // 业务返回值，0 或正数表示业务成功，负数表示失败。
	int iErrorCode;                   // 网络库错误码，0 表示网络层处理成功。
	long long lParam;                 // 第一组业务整型返回参数。
	ST_BINARY_BUFFER stPayload;       // 解压后的主返回载荷。
	long long wParam;                 // 第二组业务整型返回参数。
	ST_BINARY_BUFFER stExtra;         // 解压后的扩展返回载荷。
	char szErrInfo[256];              // 详细英文错误描述，最多保存 255 字节。
	void* pParam;                     // 异步调用方透传上下文，回调时原样返回。
	ST_BINARY_RESULT()
	{
		memset(this, 0, sizeof(ST_BINARY_RESULT));
		iVersion = 1;
	}
};

// BinaryPayload 编码缓冲；正文仍保持 Ice 收到的原始或 Snappy 状态。
// pBuffer 由所属 ST_BINARY_ENCODED_RESULT 持有，调用方只能读取且不得单独释放。
struct ST_BINARY_ENCODED_BUFFER
{
	int iVersion;                     // BinaryPayload 协议版本，当前固定为 1。
	int iCompression;                 // 0 表示原始二进制，1 表示 Snappy。
	int iRawSize;                     // 解压后的原始字节数。
	int iWireSize;                    // pBuffer 的有效编码字节数。
	int iMaxPayloadBytes;             // 接收连接允许的最大原始载荷，用于延迟解压校验。
	const unsigned char* pBuffer;     // 编码正文，只在所属结果释放前有效。
	ST_BINARY_ENCODED_BUFFER()
	{
		memset(this, 0, sizeof(ST_BINARY_ENCODED_BUFFER));
	}
};

// 异步拥有型编码结果；回调取得所有权，必须交给 BinaryEncodedResultFree 释放。
// 该结构只运输 BinaryPayload，不解释任何插件业务协议。
struct ST_BINARY_ENCODED_RESULT
{
	int iVersion;                     // 结果结构版本，当前固定为 1。
	long long lRetVal;                // 业务返回值，语义与 ST_BINARY_RESULT 一致。
	int iErrorCode;                   // 网络层错误码，0 表示运输成功。
	long long lParam;                 // 第一组业务整型返回参数。
	ST_BINARY_ENCODED_BUFFER stPayload; // 仍保持编码状态的主返回载荷。
	long long wParam;                 // 第二组业务整型返回参数。
	ST_BINARY_ENCODED_BUFFER stExtra; // 仍保持编码状态的扩展返回载荷。
	char szErrInfo[256];              // 详细英文错误描述。
	void* pParam;                     // 异步调用方透传上下文。
	void* pInternalOwner;             // DLL 私有所有权对象，调用方禁止访问。
	ST_BINARY_ENCODED_RESULT()
	{
		memset(this, 0, sizeof(ST_BINARY_ENCODED_RESULT));
		iVersion = 1;
	}
};

// 服务端收到的二进制请求；所有缓冲仅在服务端回调期间有效。
struct ST_BINARY_REQUEST
{
	int iVersion;                     // 请求结构版本，当前固定为 1。
	long long lSynId;                 // 客户端请求序号，服务端回包时由底层自动保持。
	long long lFuncId;                // 业务功能号，服务端据此分发业务处理器。
	long long lRouteCode;             // 客户端传入的路由编码。
	long long lParam;                 // 第一组业务整型输入参数。
	ST_BINARY_VIEW stPayload;         // 已解压的主请求载荷。
	long long wParam;                 // 第二组业务整型输入参数。
	ST_BINARY_VIEW stExtra;           // 已解压的扩展请求载荷。
	short chMode;                     // 请求类型，取值为 EN_BINARY_INPUT_RPC 或 EN_BINARY_INPUT_PUT。
	HANDLE hResponse;                 // DLL 内部引用计数上下文，只能原样传给 BinaryResponseData，完成后不得复用。
	ST_BINARY_REQUEST()
	{
		memset(this, 0, sizeof(ST_BINARY_REQUEST));
		iVersion = 1;
	}
};

#pragma pack(pop)

// 服务端直回调模式使用，绕过队列以降低延迟，但回调内必须尽快返回。
typedef void(*func_JsonICEServerCallBsack)(void* p_pParam, short p_chMode, long long p_lSetCode, ST_JSON_M_RESULT_TOP* p_pResult);
// 二进制服务端回调；处理完成前必须调用 BinaryResponseData，回调返回后请求缓冲立即失效。
typedef void(*func_IceBinaryServerCallback)(void* p_pParam, const ST_BINARY_REQUEST* p_pRequest);

// Ex 服务端回调返回值；COMPLETED 要求回调内已经应答，DEFERRED 允许回调返回后使用句柄应答。
enum EN_BINARY_SERVER_CALLBACK_RESULT
{
	EN_BINARY_SERVER_CALLBACK_COMPLETED = 0,
	EN_BINARY_SERVER_CALLBACK_DEFERRED = 1
};

// 延迟应答服务端回调。请求缓冲仍只在回调期间有效，hResponse 可保存到异步上下文。
typedef int(*func_IceBinaryServerCallbackEx)(void* p_pParam,
	const ST_BINARY_REQUEST* p_pRequest);
// 传输优先级分类器；返回非零表示控制请求，底层不解释业务 FuncId。
typedef int(*func_IceBinaryPriorityClassifier)(void* p_pParam,
	long long p_lFuncId);

// 返回服务端实际 Endpoint 字符串，主要用于启动后确认配置解析结果。
ICERPCPUSH_API const char* JsonGetEndPoint(HANDLE p_hHandle);

// 服务端按配置创建 Ice RPC 服务，返回 HANDLE 由 DeleteJsonICERPC 释放。
ICERPCPUSH_API HANDLE CreateJsonICEServer(const char* p_szCfgFile, const char* p_szEndPointName, HANDLE& p_hSem, bool p_bSnappyCompress);
// 可以直接注册服务端回调函数，不采用消息队列。
ICERPCPUSH_API void RegServerCallBackFunc(HANDLE p_hHandle, func_JsonICEServerCallBsack p_pfnCallback, void* p_pParam);
// 从服务端队列中弹出请求，调用方处理后必须调用 JsonBinSrvComplete。
ICERPCPUSH_API ST_JSON_INPUT* JsonBinSrvPopfront(HANDLE p_hHandle);
// 服务端处理完成后发送结果并释放队列节点，不释放调用方传入的 p_pResult。
ICERPCPUSH_API void JsonBinSrvComplete(ST_JSON_INPUT* p_pNode, ST_JSON_M_RESULT_TOP* p_pResult);
// 服务端主动推送数据到已注册客户端，可按 p_bAsync 选择异步推送。
ICERPCPUSH_API long long PushJsonICEServerData(HANDLE p_hHandle, int p_lReqNo, const char* p_pBuf, long p_lBufLen, bool p_bAsync = true);
// 客户端按配置文件创建 Ice RPC 客户端。
ICERPCPUSH_API HANDLE CreateJsonICEClient(const char* p_szCfgFile, const char* p_szProxyProperty, HANDLE& p_hSem, int p_iThreadPool = 0);
// 客户端按调用方传入的属性数组创建 Ice RPC 客户端。
ICERPCPUSH_API HANDLE CreateJsonICEClient2(int p_iNum, const char* p_pszPropertyKey[], const char* p_pszProperty[], const char* p_szProxyProperty, HANDLE& p_hSem, int p_iThreadPool = 0);
// 注册或注销客户端推送回调，p_iIsReg 为 0 时注销。
ICERPCPUSH_API HANDLE RegisterJsonICEClient(HANDLE p_hHandle, const char* p_szGuid, func_JsonICEPushClientPack p_pfnCallback, int p_iIsReg = 1, void* p_pParam = NULL);
// 带订阅字符串的注册接口，返回字符串由句柄内部缓存维护，下次调用可能被覆盖。
ICERPCPUSH_API const char* RegisterJsonICEClient2(HANDLE p_hHandle, const char* p_szGuid, const char* p_szSubInfo, func_JsonICEPushClientPack p_pfnCallback, int p_iIsReg = 1, void* p_pParam = NULL);

// 同步 RPC 调用，返回结果必须由 IJsonMutiResultFree 释放。
ICERPCPUSH_API ST_JSON_M_RESULT_LEVEL* JsonBinClientRPC(HANDLE p_hHandle, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lBufLen);
// 异步 RPC 调用，回调参数为 ST_JSON_M_RESULT_LEVEL*，回调内或之后必须释放结果。
ICERPCPUSH_API long long JsonBinClientRPCAsync(HANDLE p_hHandle, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lBufLen, LPTHREAD_START_ROUTINE p_pfnCallback, void* p_pParam);
// 兼容旧版 ABI 的异步 RPC 导出名，行为与 JsonBinClientRPCAsync 完全一致。
ICERPCPUSH_API long long JsonBinClientRPC_async(HANDLE p_hHandle, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lBufLen, LPTHREAD_START_ROUTINE p_pfnCallback, void* p_pParam);
// 预提交 RPC 请求，必须和 EndPreJsonBinClientRPC 成对调用。
ICERPCPUSH_API ST_JSON_M_RESULT_LEVEL* PreJsonBinClientRPC(HANDLE p_hHandle, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lBufLen);
// 等待预提交 RPC 结果，返回值写入 p_pResultOrg。
ICERPCPUSH_API long long EndPreJsonBinClientRPC(HANDLE p_hHandle, ST_JSON_M_RESULT_LEVEL* p_pResultOrg);
// 释放底层分配的 RPC/PUT 结果。
ICERPCPUSH_API void IJsonMutiResultFree(ST_JSON_M_RESULT_LEVEL* p_pResult);

// 同步 PUT 调用，p_pPutData 中的内部缓冲区由调用方管理生命周期。
ICERPCPUSH_API ST_JSON_M_RESULT_LEVEL* JsonBinClientPUT(HANDLE p_hHandle, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lBufLen, const ST_JSON_M_RESULT_TOP* p_pPutData);
// 异步 PUT 调用，回调参数为 ST_JSON_M_RESULT_TOP*。
ICERPCPUSH_API long long JsonBinClientPUTAsync(HANDLE p_hHandle, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lBufLen, const ST_JSON_M_RESULT_TOP* p_pPutData, LPTHREAD_START_ROUTINE p_pfnPutCallback, void* p_pParam);
// 兼容旧版 ABI 的异步 PUT 导出名，行为与 JsonBinClientPUTAsync 完全一致。
ICERPCPUSH_API long long JsonBinClientPUT_async(HANDLE p_hHandle, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lBufLen, const ST_JSON_M_RESULT_TOP* p_pPutData, LPTHREAD_START_ROUTINE p_pfnPutCallback, void* p_pParam);
// 预提交 PUT 请求，必须和 EndPreJsonBinClientPUT 成对调用。
ICERPCPUSH_API ST_JSON_M_RESULT_LEVEL* PreJsonBinClientPUT(HANDLE p_hHandle, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lBufLen, const ST_JSON_M_RESULT_TOP* p_pPutData);
// 等待预提交 PUT 结果，返回值写入 p_pResultOrg。
ICERPCPUSH_API long long EndPreJsonBinClientPUT(HANDLE p_hHandle, ST_JSON_M_RESULT_LEVEL* p_pResultOrg);

// 服务端、客户端共用的销毁入口，必须与 CreateJsonICE* 返回的 HANDLE 配对。
ICERPCPUSH_API void DeleteJsonICERPC(HANDLE p_hHandle);
// 服务端完成异步请求时主动回包，p_pResultCallback 内部缓冲区仍由调用方维护生命周期。
ICERPCPUSH_API void JsonICEResponseData(HANDLE p_hHandle, ST_JSON_M_RESULT_TOP* p_pResultCallback);
// 注册独立二进制服务端回调，并按 XML 配置启动有界工作线程。
ICERPCPUSH_API void RegBinaryServerCallBackFunc(HANDLE p_hHandle, func_IceBinaryServerCallback p_pfnCallback, void* p_pParam);
// 注册支持延迟应答和控制优先级的服务端回调；与旧注册接口互斥。
ICERPCPUSH_API void RegBinaryServerCallBackFuncEx(HANDLE p_hHandle,
	func_IceBinaryServerCallbackEx p_pfnCallback,
	func_IceBinaryPriorityClassifier p_pfnPriorityClassifier,
	void* p_pParam);
// 同步二进制 RPC；返回结果必须使用 BinaryResultFree 释放。
ICERPCPUSH_API ST_BINARY_RESULT* BinaryClientRPC(HANDLE p_hHandle, const ST_BINARY_CALL* p_pCall);
// 异步二进制 RPC；提交成功后回调参数为 ST_BINARY_RESULT*，回调方必须释放结果。
ICERPCPUSH_API long long BinaryClientRPCAsync(HANDLE p_hHandle, const ST_BINARY_CALL* p_pCall, LPTHREAD_START_ROUTINE p_pfnCallback, void* p_pParam);
// 单次超时覆盖的异步 RPC；0 表示无限等待，正数为毫秒，不修改句柄全局配置。
ICERPCPUSH_API long long BinaryClientRPCAsyncEx(HANDLE p_hHandle,
	const ST_BINARY_CALL* p_pCall, int p_iTimeoutMs,
	LPTHREAD_START_ROUTINE p_pfnCallback, void* p_pParam);
// 拥有型编码异步 RPC；回调取得 ST_BINARY_ENCODED_RESULT 所有权，不提前解压正文。
ICERPCPUSH_API long long BinaryClientRPCAsyncEncodedEx(HANDLE p_hHandle,
	const ST_BINARY_CALL* p_pCall, int p_iTimeoutMs,
	LPTHREAD_START_ROUTINE p_pfnCallback, void* p_pParam);
// 同步二进制 PUT；返回结果必须使用 BinaryResultFree 释放。
ICERPCPUSH_API ST_BINARY_RESULT* BinaryClientPUT(HANDLE p_hHandle, const ST_BINARY_CALL* p_pCall);
// 异步二进制 PUT；提交成功后回调参数为 ST_BINARY_RESULT*，回调方必须释放结果。
ICERPCPUSH_API long long BinaryClientPUTAsync(HANDLE p_hHandle, const ST_BINARY_CALL* p_pCall, LPTHREAD_START_ROUTINE p_pfnCallback, void* p_pParam);
// 单次超时覆盖的异步 PUT；0 表示无限等待。
ICERPCPUSH_API long long BinaryClientPUTAsyncEx(HANDLE p_hHandle,
	const ST_BINARY_CALL* p_pCall, int p_iTimeoutMs,
	LPTHREAD_START_ROUTINE p_pfnCallback, void* p_pParam);
// 拥有型编码异步 PUT；行为与编码 RPC 一致，额外保留 PUT 参数和 extra。
ICERPCPUSH_API long long BinaryClientPUTAsyncEncodedEx(HANDLE p_hHandle,
	const ST_BINARY_CALL* p_pCall, int p_iTimeoutMs,
	LPTHREAD_START_ROUTINE p_pfnCallback, void* p_pParam);
// 服务端二进制回包入口；同步和延迟回调都只能对同一句柄应答一次。
ICERPCPUSH_API void BinaryResponseData(HANDLE p_hResponse, const ST_BINARY_RESULT* p_pResult);
// 同步或延迟应答的可检查入口；成功返回 1，重复、过期或非法句柄返回 0。
ICERPCPUSH_API int BinaryResponseDataEx(HANDLE p_hResponse,
	const ST_BINARY_RESULT* p_pResult);
// 释放 BinaryClientRPC/BinaryClientPUT 以及异步回调返回的结果。
ICERPCPUSH_API void BinaryResultFree(ST_BINARY_RESULT* p_pResult);
// 在最终消费线程校验并解码一个拥有型 BinaryPayload；成功后缓冲由 BinaryBufferFree 释放。
ICERPCPUSH_API int BinaryDecodeEncodedBuffer(
	const ST_BINARY_ENCODED_BUFFER* p_pEncoded,
	ST_BINARY_BUFFER* p_pDecoded, char* p_szError, int p_iErrorCapacity);
// 释放 BinaryDecodeEncodedBuffer 分配的原始缓冲并清空视图。
ICERPCPUSH_API void BinaryBufferFree(ST_BINARY_BUFFER* p_pBuffer);
// 释放编码异步回调取得的结果及其 payload/extra 所有权。
ICERPCPUSH_API void BinaryEncodedResultFree(
	ST_BINARY_ENCODED_RESULT* p_pResult);
// 获取指定句柄最后一次错误码；句柄为空时读取当前线程错误。
ICERPCPUSH_API int IceRPCPushGetLastErrorCode(HANDLE p_hHandle);
// 获取指定句柄最后一次详细英文错误描述；句柄为空时读取当前线程错误。
ICERPCPUSH_API const char* IceRPCPushGetLastError(HANDLE p_hHandle);
// 根据错误码获取默认英文错误描述；不依赖句柄。
ICERPCPUSH_API const char* IceRPCPushGetErrorMsg(int p_iErrorCode);

#endif // H_INTERFACE_ICE_RPC_PUSH
