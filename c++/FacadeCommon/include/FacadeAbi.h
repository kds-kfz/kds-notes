#ifndef KDS_FACADE_ABI_H
#define KDS_FACADE_ABI_H

#include <stddef.h>
#include <stdint.h>

#define FACADE_ABI_VERSION 1U
#define FACADE_BUILD_ID_CAPACITY 64U
#define FACADE_VERSION_CAPACITY 32U

#ifdef __cplusplus
extern "C"
{
#endif

// 所有 Facade 共用的稳定返回码；新增返回码只能追加，禁止修改已经发布的数值。
typedef enum EN_FACADE_RESULT
{
	FACADE_SUCCESS = 0,
	FACADE_INVALID_ARGUMENT = -1,
	FACADE_ABI_MISMATCH = -2,
	FACADE_BUFFER_TOO_SMALL = -3,
	FACADE_NOT_SUPPORTED = -4,
	FACADE_NOT_INITIALIZED = -5,
	FACADE_RESOURCE_EXHAUSTED = -6,
	FACADE_TIMEOUT = -7,
	FACADE_IO_ERROR = -8,
	FACADE_PROTOCOL_ERROR = -9,
	FACADE_INTERNAL_ERROR = -10
} EN_FACADE_RESULT;

// 只读 UTF-8 字符串视图。数据由调用方拥有，只保证在当前同步调用期间有效。
typedef struct ST_FACADE_STRING_VIEW
{
	uint32_t uiStructSize; // 本结构实际字节数；调用方必须填写 sizeof(ST_FACADE_STRING_VIEW)。
	uint32_t uiAbiVersion; // 调用方使用的 ABI 版本；当前必须为 FACADE_ABI_VERSION。
	uint32_t uiFlags; // 行为标志；当前没有已定义标志，调用方必须填写 0。
	uint64_t aullReserved[4]; // 后续扩展预留；调用方必须全部清零。
	const char* pszData; // UTF-8 数据首地址；uiLength 为 0 时允许为空，不要求以零结尾。
	size_t uiLength; // UTF-8 数据字节数，不包含任何可选的结尾零字符。
} ST_FACADE_STRING_VIEW;

// 只读二进制缓冲区视图。数据由调用方拥有，只保证在当前同步调用期间有效。
typedef struct ST_FACADE_BUFFER_VIEW
{
	uint32_t uiStructSize; // 本结构实际字节数；调用方必须填写 sizeof(ST_FACADE_BUFFER_VIEW)。
	uint32_t uiAbiVersion; // 调用方使用的 ABI 版本；当前必须为 FACADE_ABI_VERSION。
	uint32_t uiFlags; // 行为标志；当前没有已定义标志，调用方必须填写 0。
	uint64_t aullReserved[4]; // 后续扩展预留；调用方必须全部清零。
	const uint8_t* pData; // 输入字节首地址；uiLength 为 0 时允许为空。
	size_t uiLength; // 输入数据字节数；调用期间不得改变对应内存。
} ST_FACADE_BUFFER_VIEW;

// 调用方提供的可写缓冲区。容量不足时函数返回所需字节数，不写入部分结果。
typedef struct ST_FACADE_MUTABLE_BUFFER
{
	uint32_t uiStructSize; // 本结构实际字节数；调用方必须填写 sizeof(ST_FACADE_MUTABLE_BUFFER)。
	uint32_t uiAbiVersion; // 调用方使用的 ABI 版本；当前必须为 FACADE_ABI_VERSION。
	uint32_t uiFlags; // 行为标志；当前没有已定义标志，调用方必须填写 0。
	uint64_t aullReserved[4]; // 后续扩展预留；调用方必须全部清零。
	uint8_t* pData; // 输出缓冲区首地址；uiCapacity 为 0 时允许为空。
	size_t uiCapacity; // pData 可写容量，单位为字节。
	size_t uiLength; // 成功时为实际字节数；容量不足时为所需字节数；其他失败时为 0。
} ST_FACADE_MUTABLE_BUFFER;

// Facade 运行时版本信息。固定数组由 DLL 填充并保证以零字符结束。
typedef struct ST_FACADE_ABI_INFO
{
	uint32_t uiStructSize; // 本结构实际字节数；调用方必须填写 sizeof(ST_FACADE_ABI_INFO)。
	uint32_t uiAbiVersion; // 调用方使用的 ABI 版本；输入和输出均为 FACADE_ABI_VERSION。
	uint32_t uiFlags; // 实现能力标志；未定义位始终为 0。
	uint64_t aullReserved[4]; // 后续扩展预留；调用方输入时必须清零。
	char szFacadeName[FACADE_BUILD_ID_CAPACITY]; // Facade 稳定名称，例如 FoundationFacade。
	char szImplementationVersion[FACADE_VERSION_CAPACITY]; // 实现语义版本，例如 1.0.0。
	char szDependencyVersion[FACADE_BUILD_ID_CAPACITY]; // 私有核心依赖版本；没有依赖时为 none。
	char szBuildId[FACADE_BUILD_ID_CAPACITY]; // 构建标识，当前由编译日期和时间组成。
} ST_FACADE_ABI_INFO;

#ifdef __cplusplus
}
#endif

#endif // KDS_FACADE_ABI_H
