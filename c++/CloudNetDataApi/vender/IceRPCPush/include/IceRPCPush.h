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

#pragma pack(pop)

// 服务端直回调模式使用，绕过队列以降低延迟，但回调内必须尽快返回。
typedef void(*func_JsonICEServerCallBsack)(void* p_pParam, short p_chMode, long long p_lSetCode, ST_JSON_M_RESULT_TOP* p_pResult);

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
ICERPCPUSH_API long long JsonBinClientPUT_async(HANDLE p_hHandle, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lBufLen, const ST_JSON_M_RESULT_TOP* p_pPutData, LPTHREAD_START_ROUTINE p_pfnPutCallback, void* p_pParam);
// 预提交 PUT 请求，必须和 EndPreJsonBinClientPUT 成对调用。
ICERPCPUSH_API ST_JSON_M_RESULT_LEVEL* PreJsonBinClientPUT(HANDLE p_hHandle, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lBufLen, const ST_JSON_M_RESULT_TOP* p_pPutData);
// 等待预提交 PUT 结果，返回值写入 p_pResultOrg。
ICERPCPUSH_API long long EndPreJsonBinClientPUT(HANDLE p_hHandle, ST_JSON_M_RESULT_LEVEL* p_pResultOrg);

// 服务端、客户端共用的销毁入口，必须与 CreateJsonICE* 返回的 HANDLE 配对。
ICERPCPUSH_API void DeleteJsonICERPC(HANDLE p_hHandle);
// 服务端完成异步请求时主动回包，p_pResultCallback 内部缓冲区仍由调用方维护生命周期。
ICERPCPUSH_API void JsonICEResponseData(HANDLE p_hHandle, ST_JSON_M_RESULT_TOP* p_pResultCallback);

#endif // H_INTERFACE_ICE_RPC_PUSH
