#ifndef KDS_COMPRESSION_FACADE_H
#define KDS_COMPRESSION_FACADE_H

#include "FacadeAbi.h"

#if defined(_WIN32)
#if defined(COMPRESSION_FACADE_STATIC)
#define COMPRESSION_FACADE_API
#elif defined(COMPRESSION_FACADE_EXPORTS)
#define COMPRESSION_FACADE_API __declspec(dllexport)
#else
#define COMPRESSION_FACADE_API __declspec(dllimport)
#endif
#else
#define COMPRESSION_FACADE_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C"
{
#endif

// 压缩算法稳定编号；已有数值不可修改，后续算法只能追加。
typedef enum EN_COMPRESSION_ALGORITHM
{
	COMPRESSION_ALGORITHM_UNKNOWN = 0,
	COMPRESSION_ALGORITHM_SNAPPY = 1,
	COMPRESSION_ALGORITHM_ZLIB = 2,
	COMPRESSION_ALGORITHM_GZIP = 3
} EN_COMPRESSION_ALGORITHM;

// Codec 创建配置；创建成功后 DLL 深拷贝全部数值，调用方可立即释放本结构。
typedef struct ST_COMPRESSION_CODEC_CONFIG
{
	uint32_t uiStructSize; // 本结构实际字节数；必须填写 sizeof(ST_COMPRESSION_CODEC_CONFIG)。
	uint32_t uiAbiVersion; // 调用方 ABI 版本；当前必须为 FACADE_ABI_VERSION。
	uint32_t uiFlags; // 行为标志；当前没有已定义标志，必须为 0。
	uint64_t aullReserved[4]; // 后续扩展预留；调用方必须全部清零。
	EN_COMPRESSION_ALGORITHM enAlgorithm; // 请求算法；首期仅 SNAPPY 有效。
	uint32_t uiReservedAlignment; // 枚举后的对齐保留字段；调用方必须为 0。
	uint64_t ullMaxInputBytes; // 单次压缩或压缩包输入上限，范围 1..1073741824 字节。
	uint64_t ullMaxOutputBytes; // 单次输出上限，范围 1..4294967296 字节，防止解压炸弹。
} ST_COMPRESSION_CODEC_CONFIG;

// Codec 不透明句柄；只能由 CompressionFacade_DestroyCodec 释放。
typedef void* COMPRESSION_CODEC_HANDLE;

// 取得 ABI、实现版本、私有压缩库版本和构建标识；线程安全且不创建 Codec。
COMPRESSION_FACADE_API int CompressionFacade_GetAbiInfo(ST_FACADE_ABI_INFO* p_pInfo);

// 按算法配置创建 Codec；同一句柄可并发调用，销毁不得与其他调用并发。
COMPRESSION_FACADE_API int CompressionFacade_CreateCodec(
	const ST_COMPRESSION_CODEC_CONFIG* p_pConfig,
	COMPRESSION_CODEC_HANDLE* p_pCodec);

// 释放本 DLL 创建的 Codec；空句柄安全返回。
COMPRESSION_FACADE_API void CompressionFacade_DestroyCodec(
	COMPRESSION_CODEC_HANDLE p_pCodec);

// 计算给定输入的压缩输出容量上界；结果单位为字节。
COMPRESSION_FACADE_API int CompressionFacade_GetMaxCompressedLength(
	COMPRESSION_CODEC_HANDLE p_pCodec, size_t p_uiInputLength,
	size_t* p_puiMaximumLength);

// 压缩完整输入；容量不足时不写部分结果并在输出 uiLength 返回所需容量。
COMPRESSION_FACADE_API int CompressionFacade_Compress(
	COMPRESSION_CODEC_HANDLE p_pCodec, const ST_FACADE_BUFFER_VIEW* p_pInput,
	ST_FACADE_MUTABLE_BUFFER* p_pOutput);

// 从压缩包读取原始长度但不解压；损坏输入返回 FACADE_PROTOCOL_ERROR。
COMPRESSION_FACADE_API int CompressionFacade_GetUncompressedLength(
	COMPRESSION_CODEC_HANDLE p_pCodec, const ST_FACADE_BUFFER_VIEW* p_pInput,
	size_t* p_puiUncompressedLength);

// 解压完整输入；先校验配置输出上限，容量不足时不写部分结果。
COMPRESSION_FACADE_API int CompressionFacade_Decompress(
	COMPRESSION_CODEC_HANDLE p_pCodec, const ST_FACADE_BUFFER_VIEW* p_pInput,
	ST_FACADE_MUTABLE_BUFFER* p_pOutput);

#ifdef __cplusplus
}
#endif

#endif // KDS_COMPRESSION_FACADE_H
