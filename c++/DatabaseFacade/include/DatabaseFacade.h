#ifndef KDS_DATABASE_FACADE_H
#define KDS_DATABASE_FACADE_H

#include "FacadeAbi.h"

#if defined(_WIN32)
#if defined(DATABASE_FACADE_STATIC)
#define DATABASE_FACADE_API
#elif defined(DATABASE_FACADE_EXPORTS)
#define DATABASE_FACADE_API __declspec(dllexport)
#else
#define DATABASE_FACADE_API __declspec(dllimport)
#endif
#else
#define DATABASE_FACADE_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C"
{
#endif

// 数据库 Provider 稳定编号；已有数值不可修改，后续 Provider 只能追加。
typedef enum EN_DATABASE_PROVIDER
{
	DATABASE_PROVIDER_UNKNOWN = 0,
	DATABASE_PROVIDER_POSTGRESQL = 1,
	DATABASE_PROVIDER_MYSQL = 2,
	DATABASE_PROVIDER_SQLITE = 3
} EN_DATABASE_PROVIDER;

// 数据库池创建配置。所有字符串在 CreatePool 返回前由 DLL 深拷贝。
typedef struct ST_DATABASE_POOL_CONFIG
{
	uint32_t uiStructSize; // 本结构实际字节数；必须填写 sizeof(ST_DATABASE_POOL_CONFIG)。
	uint32_t uiAbiVersion; // 调用方 ABI 版本；当前必须为 FACADE_ABI_VERSION。
	uint32_t uiFlags; // 行为标志；当前没有已定义标志，必须为 0。
	uint64_t aullReserved[4]; // 后续扩展预留；调用方必须全部清零。
	EN_DATABASE_PROVIDER enProvider; // Provider 类型；首期只支持 PostgreSQL。
	uint32_t uiPort; // 服务端 TCP 端口，范围 1..65535。
	ST_FACADE_STRING_VIEW stHost; // UTF-8 主机名或 IP；创建期间借用。
	ST_FACADE_STRING_VIEW stDatabase; // UTF-8 数据库名；创建期间借用。
	ST_FACADE_STRING_VIEW stUser; // UTF-8 用户名；创建期间借用。
	ST_FACADE_STRING_VIEW stPassword; // UTF-8 密码；创建期间借用且禁止记录日志。
	ST_FACADE_STRING_VIEW stApplicationName; // UTF-8 application_name；用于服务端诊断。
	uint32_t uiMinConnections; // 启动时建立的最小连接数，范围 1..1024。
	uint32_t uiMaxConnections; // 最大连接数，范围 uiMinConnections..1024。
	uint32_t uiAcquireTimeoutMs; // 获取连接默认超时，范围 1..600000 毫秒。
	uint32_t uiConnectTimeoutMs; // 新建物理连接超时，范围 100..600000 毫秒。
	uint32_t uiStatementTimeoutMs; // 服务端语句超时，范围 0..3600000 毫秒；0 表示服务端默认。
	uint32_t uiHealthCheckIntervalMs; // 空闲连接检查周期，范围 1000..3600000 毫秒。
	uint32_t uiIdleShrinkMs; // 超过最小连接数后的空闲回收时间，范围 0..86400000 毫秒。
} ST_DATABASE_POOL_CONFIG;

// 单个 SQL 参数。值在同步 Execute 调用期间借用，bNull 非零时忽略 stValue。
typedef struct ST_DATABASE_PARAMETER
{
	uint32_t uiStructSize; // 本结构实际字节数；必须填写 sizeof(ST_DATABASE_PARAMETER)。
	uint32_t uiAbiVersion; // 调用方 ABI 版本；当前必须为 FACADE_ABI_VERSION。
	uint32_t uiFlags; // 行为标志；当前没有已定义标志，必须为 0。
	uint64_t aullReserved[4]; // 后续扩展预留；调用方必须全部清零。
	int32_t bNull; // 非零表示 SQL NULL；为 0 时 stValue 必须有效。
	uint32_t uiReservedAlignment; // 对齐和扩展预留；调用方必须为 0。
	ST_FACADE_STRING_VIEW stValue; // UTF-8 文本参数；允许包含零字节但 PostgreSQL 文本参数通常不使用。
} ST_DATABASE_PARAMETER;

// 批量事务中的一条 SQL；数组及其嵌套数据仅在同步调用期间借用。
typedef struct ST_DATABASE_BATCH_ITEM
{
	uint32_t uiStructSize; // 本结构实际字节数；必须填写 sizeof(ST_DATABASE_BATCH_ITEM)。
	uint32_t uiAbiVersion; // 调用方 ABI 版本；当前必须为 FACADE_ABI_VERSION。
	uint32_t uiFlags; // 行为标志；当前没有已定义标志，必须为 0。
	uint64_t aullReserved[4]; // 后续扩展预留；调用方必须全部清零。
	ST_FACADE_STRING_VIEW stSql; // UTF-8 SQL，参数占位符由 Provider 定义；PostgreSQL 使用 $1..$N。
	const ST_DATABASE_PARAMETER* pParameters; // 参数数组首地址；uiParameterCount 为 0 时允许为空。
	size_t uiParameterCount; // 参数个数，范围 0..65535。
} ST_DATABASE_BATCH_ITEM;

// 连接池瞬时统计。计数在查询时复制，之后不会自动更新。
typedef struct ST_DATABASE_POOL_STAT
{
	uint32_t uiStructSize; // 本结构实际字节数；必须填写 sizeof(ST_DATABASE_POOL_STAT)。
	uint32_t uiAbiVersion; // 调用方 ABI 版本；当前必须为 FACADE_ABI_VERSION。
	uint32_t uiFlags; // 行为标志；当前没有已定义标志，必须为 0。
	uint64_t aullReserved[4]; // 后续扩展预留；调用方必须全部清零。
	uint32_t uiTotalConnections; // 当前物理连接对象总数。
	uint32_t uiIdleConnections; // 当前可立即获取的连接数。
	uint32_t uiBusyConnections; // 当前被 SQL 或事务占用的连接数。
	uint32_t uiCreatingConnections; // 当前正在建立的连接数。
	uint64_t ullAcquireCount; // 累计成功获取连接次数。
	uint64_t ullWaitCount; // 累计发生等待的获取次数。
	uint64_t ullTimeoutCount; // 累计获取连接超时次数。
	uint64_t ullConnectFailureCount; // 累计物理连接建立失败次数。
	uint64_t ullDiscardedCount; // 累计因断开或协议错误丢弃的连接数。
	uint64_t ullReconnectCount; // 后台维护线程累计补建成功的物理连接数；不含初始化及业务扩容。
	uint64_t ullShrinkCount; // 累计因空闲时间达到 uiIdleShrinkMs 而主动回收的连接数。
} ST_DATABASE_POOL_STAT;

typedef void* DATABASE_POOL_HANDLE; // 连接池句柄；由 DatabaseFacade_DestroyPool 释放。
typedef void* DATABASE_TRANSACTION_HANDLE; // 单线程事务句柄；由 DatabaseFacade_DestroyTransaction 释放。
typedef void* DATABASE_RESULT_HANDLE; // 不可变结果句柄；可并发只读并由 DatabaseFacade_DestroyResult 释放。

DATABASE_FACADE_API int DatabaseFacade_GetAbiInfo(ST_FACADE_ABI_INFO* p_pInfo);
DATABASE_FACADE_API int DatabaseFacade_CreatePool(
	const ST_DATABASE_POOL_CONFIG* p_pConfig, DATABASE_POOL_HANDLE* p_pPool);
DATABASE_FACADE_API void DatabaseFacade_DestroyPool(DATABASE_POOL_HANDLE p_pPool);
DATABASE_FACADE_API int DatabaseFacade_IsPoolReady(
	DATABASE_POOL_HANDLE p_pPool, int32_t* p_piReady);
DATABASE_FACADE_API int DatabaseFacade_GetPoolStat(
	DATABASE_POOL_HANDLE p_pPool, ST_DATABASE_POOL_STAT* p_pStat);
DATABASE_FACADE_API int DatabaseFacade_GetLastError(
	DATABASE_POOL_HANDLE p_pPool, ST_FACADE_MUTABLE_BUFFER* p_pError);

DATABASE_FACADE_API int DatabaseFacade_Execute(
	DATABASE_POOL_HANDLE p_pPool, const ST_FACADE_STRING_VIEW* p_pSql,
	const ST_DATABASE_PARAMETER* p_pParameters, size_t p_uiParameterCount,
	uint32_t p_uiAcquireTimeoutMs, DATABASE_RESULT_HANDLE* p_pResult);
DATABASE_FACADE_API int DatabaseFacade_ExecuteBatch(
	DATABASE_POOL_HANDLE p_pPool, const ST_DATABASE_BATCH_ITEM* p_pItems,
	size_t p_uiItemCount, uint32_t p_uiAcquireTimeoutMs,
	DATABASE_RESULT_HANDLE* p_pResult);
DATABASE_FACADE_API int DatabaseFacade_Prepare(
	DATABASE_POOL_HANDLE p_pPool, const ST_FACADE_STRING_VIEW* p_pName,
	const ST_FACADE_STRING_VIEW* p_pSql);
DATABASE_FACADE_API int DatabaseFacade_ExecutePrepared(
	DATABASE_POOL_HANDLE p_pPool, const ST_FACADE_STRING_VIEW* p_pName,
	const ST_DATABASE_PARAMETER* p_pParameters, size_t p_uiParameterCount,
	uint32_t p_uiAcquireTimeoutMs, DATABASE_RESULT_HANDLE* p_pResult);

DATABASE_FACADE_API int DatabaseFacade_Begin(
	DATABASE_POOL_HANDLE p_pPool, uint32_t p_uiAcquireTimeoutMs,
	DATABASE_TRANSACTION_HANDLE* p_pTransaction);
DATABASE_FACADE_API int DatabaseFacade_TransactionExecute(
	DATABASE_TRANSACTION_HANDLE p_pTransaction,
	const ST_FACADE_STRING_VIEW* p_pSql,
	const ST_DATABASE_PARAMETER* p_pParameters, size_t p_uiParameterCount,
	DATABASE_RESULT_HANDLE* p_pResult);
DATABASE_FACADE_API int DatabaseFacade_Commit(
	DATABASE_TRANSACTION_HANDLE p_pTransaction);
DATABASE_FACADE_API int DatabaseFacade_Rollback(
	DATABASE_TRANSACTION_HANDLE p_pTransaction);
DATABASE_FACADE_API void DatabaseFacade_DestroyTransaction(
	DATABASE_TRANSACTION_HANDLE p_pTransaction);

DATABASE_FACADE_API void DatabaseFacade_DestroyResult(DATABASE_RESULT_HANDLE p_pResult);
DATABASE_FACADE_API int DatabaseFacade_ResultGetAffectedRows(
	DATABASE_RESULT_HANDLE p_pResult, uint64_t* p_pullAffectedRows);
DATABASE_FACADE_API int DatabaseFacade_ResultGetDimensions(
	DATABASE_RESULT_HANDLE p_pResult, size_t* p_puiRows, size_t* p_puiColumns);
DATABASE_FACADE_API int DatabaseFacade_ResultGetColumnName(
	DATABASE_RESULT_HANDLE p_pResult, size_t p_uiColumn,
	ST_FACADE_MUTABLE_BUFFER* p_pName);
DATABASE_FACADE_API int DatabaseFacade_ResultIsNull(
	DATABASE_RESULT_HANDLE p_pResult, size_t p_uiRow, size_t p_uiColumn,
	int32_t* p_piNull);
DATABASE_FACADE_API int DatabaseFacade_ResultGetField(
	DATABASE_RESULT_HANDLE p_pResult, size_t p_uiRow, size_t p_uiColumn,
	ST_FACADE_MUTABLE_BUFFER* p_pValue);
DATABASE_FACADE_API int DatabaseFacade_ResultGetError(
	DATABASE_RESULT_HANDLE p_pResult, ST_FACADE_MUTABLE_BUFFER* p_pError);

#ifdef __cplusplus
}
#endif

#endif // KDS_DATABASE_FACADE_H
