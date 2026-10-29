#ifndef KDS_NETWORK_FACADE_H
#define KDS_NETWORK_FACADE_H

#include "FacadeAbi.h"

#if defined(_WIN32)
#if defined(NETWORK_FACADE_EXPORTS)
#define NETWORK_FACADE_API __declspec(dllexport)
#else
#define NETWORK_FACADE_API __declspec(dllimport)
#endif
#else
#define NETWORK_FACADE_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C"
{
#endif

// NetworkFacade 已编译能力位；已有位值禁止修改，后续能力只能追加新位。
typedef enum EN_NETWORK_FACADE_CAPABILITY
{
	NETWORK_FACADE_CAPABILITY_TCP_SERVER = 1U << 0, // 支持 TCP 服务端实例。
	NETWORK_FACADE_CAPABILITY_HTTP_SERVER = 1U << 1, // 支持 HTTP 服务端实例。
	NETWORK_FACADE_CAPABILITY_WEBSOCKET_SERVER = 1U << 2, // 支持 WebSocket 服务端实例。
	NETWORK_FACADE_CAPABILITY_TCP_CLIENT = 1U << 3, // 支持 TCP 客户端实例。
	NETWORK_FACADE_CAPABILITY_HTTP_CLIENT = 1U << 4, // 支持 HTTP 客户端实例。
	NETWORK_FACADE_CAPABILITY_WEBSOCKET_CLIENT = 1U << 5, // 支持 WebSocket 客户端实例。
	NETWORK_FACADE_CAPABILITY_TLS_CLIENT = 1U << 6 // 支持验证 CA 与 SAN/CN 的 HTTPS/WSS 客户端。
} EN_NETWORK_FACADE_CAPABILITY;

// 网络实现能力快照；函数返回时完成复制，调用方拥有本结构且可立即复用或释放。
typedef struct ST_NETWORK_FACADE_CAPABILITIES
{
	uint32_t uiStructSize; // 本结构实际字节数；必须填写 sizeof(ST_NETWORK_FACADE_CAPABILITIES)。
	uint32_t uiAbiVersion; // 调用方 ABI 版本；当前必须为 FACADE_ABI_VERSION。
	uint32_t uiFlags; // 已编译能力位，输出为 EN_NETWORK_FACADE_CAPABILITY 的按位或。
	uint64_t aullReserved[4]; // 后续扩展预留；调用方输入时必须全部清零。
	uint32_t uiSocketServerAbiVersion; // 私有 SocketServer ABI 版本；仅用于启动诊断，不作为业务分支条件。
	uint32_t uiSocketClientAbiVersion; // 私有 SocketClient ABI 版本；仅用于启动诊断，不作为业务分支条件。
} ST_NETWORK_FACADE_CAPABILITIES;

// 取得 Facade ABI、实现版本、私有依赖版本和构建标识；线程安全且不创建网络资源。
NETWORK_FACADE_API int NetworkFacade_GetAbiInfo(ST_FACADE_ABI_INFO* p_pInfo);

// 取得编译期网络能力及私有 ABI 版本；线程安全，失败时不修改调用方后续字段。
NETWORK_FACADE_API int NetworkFacade_GetCapabilities(
	ST_NETWORK_FACADE_CAPABILITIES* p_pCapabilities);

#ifdef __cplusplus
}
#endif

#endif // KDS_NETWORK_FACADE_H
