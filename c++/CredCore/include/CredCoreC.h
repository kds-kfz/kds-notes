#ifndef GUANGWZ_CRED_CORE_C_H
#define GUANGWZ_CRED_CORE_C_H

#include "FacadeAbi.h"

#if defined(_WIN32) || defined(_WIN64)
#if defined(CREDCORE_STATIC) || defined(CREDCORE_EXPORTS)
#define CREDCORE_C_API
#else
#define CREDCORE_C_API __declspec(dllimport)
#endif
#else
#define CREDCORE_C_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C"
{
#endif

// 指向 CredCore 私有只读 RSA 公钥对象的不透明句柄；只能由 CredCore_DestroyRs256PublicKeyVerifier 释放。
typedef void* CREDCORE_PUBLIC_KEY_HANDLE;

// RS256 操作结果。Facade 调用本身的参数、ABI 和缓冲区错误由 EN_FACADE_RESULT 返回。
typedef enum EN_CREDCORE_C_RS256_RESULT
{
	CREDCORE_C_RS256_SUCCESS = 0, // 操作成功，输出参数可以使用。
	CREDCORE_C_RS256_INVALID_ARGUMENT = -100, // 密码操作的必填输入或句柄无效。
	CREDCORE_C_RS256_INVALID_JWT_FORMAT = -101, // Compact JWT 不是严格三段格式。
	CREDCORE_C_RS256_INVALID_BASE64URL = -102, // JWT 或 JWK 字段不是严格无填充 Base64URL。
	CREDCORE_C_RS256_INVALID_HEADER_JSON = -103, // JWT Header 不是满足约束的 JSON 对象。
	CREDCORE_C_RS256_UNSUPPORTED_ALGORITHM = -104, // JWT 算法不是固定允许的 RS256。
	CREDCORE_C_RS256_MISSING_KEY_ID = -105, // JWT Header 缺少非空 kid。
	CREDCORE_C_RS256_KEY_ID_MISMATCH = -106, // JWT kid 与公钥句柄绑定值不一致。
	CREDCORE_C_RS256_INVALID_JWK = -107, // JWK n/e 无法构造合法 RSA 公钥。
	CREDCORE_C_RS256_INVALID_RSA_KEY_SIZE = -108, // RSA 模数不在 2048..4096 位范围内。
	CREDCORE_C_RS256_SIGNATURE_MISMATCH = -109, // JWT 签名与指定公钥不匹配。
	CREDCORE_C_RS256_OPENSSL_ERROR = -110, // OpenSSL Provider、摘要或公钥操作失败。
	CREDCORE_C_RS256_OPENSSL_VERSION_MISMATCH = -111, // 运行时 OpenSSL 不是构建时固定版本。
	CREDCORE_C_RS256_INTERNAL_ERROR = -112 // 未分类异常；调用方不得据此放行请求。
} EN_CREDCORE_C_RS256_RESULT;

// 取得 CredCore ABI、实现版本、OpenSSL 版本和构建标识；不创建密码资源，可并发调用。
CREDCORE_C_API int CredCore_GetAbiInfo(ST_FACADE_ABI_INFO* p_pInfo);

// 校验历史授权码。输入视图只在调用期间借用；输出为解密信息且由调用方缓冲区拥有。
// 函数返回 EN_FACADE_RESULT，p_piOperationResult 返回原授权算法结果；容量不足时只回填所需字节数。
CREDCORE_C_API int CredCore_AuthFeatureByPlainClientInfo(
	const ST_FACADE_STRING_VIEW* p_pClientInfo,
	const ST_FACADE_STRING_VIEW* p_pFeature,
	ST_FACADE_MUTABLE_BUFFER* p_pFeatureInfo,
	int32_t* p_piOperationResult);

// 生成历史自然日授权码。p_iAuthDay 单位为自然日，沿用旧接口有效范围和返回语义。
// 函数返回 EN_FACADE_RESULT，p_piOperationResult 返回原授权算法结果；输出由调用方缓冲区拥有。
CREDCORE_C_API int CredCore_BuildFeatureByPlainClientInfo(
	const ST_FACADE_STRING_VIEW* p_pClientInfo,
	int32_t p_iAuthDay,
	ST_FACADE_MUTABLE_BUFFER* p_pFeature,
	int32_t* p_piOperationResult);

// 计算完整 Compact JWT 的 SHA-256 小写十六进制指纹；成功输出固定 64 字节且不附加结尾零字符。
CREDCORE_C_API int CredCore_GetCompactJwtSha256Fingerprint(
	const ST_FACADE_STRING_VIEW* p_pCompactJwt,
	ST_FACADE_MUTABLE_BUFFER* p_pFingerprintHex,
	int32_t* p_piOperationResult);

// 严格解析 Compact JWT Header 并输出仅供选公钥使用的未受信 kid；成功前不得作为身份使用。
CREDCORE_C_API int CredCore_ReadRs256JwtKeyId(
	const ST_FACADE_STRING_VIEW* p_pCompactJwt,
	ST_FACADE_MUTABLE_BUFFER* p_pKeyId,
	int32_t* p_piOperationResult);

// 由 JWK kid、n、e 创建不可变公钥句柄；输入均为同步借用，句柄由 CredCore 独占。
CREDCORE_C_API int CredCore_CreateRs256PublicKeyVerifier(
	const ST_FACADE_STRING_VIEW* p_pKeyId,
	const ST_FACADE_STRING_VIEW* p_pModulusBase64Url,
	const ST_FACADE_STRING_VIEW* p_pExponentBase64Url,
	CREDCORE_PUBLIC_KEY_HANDLE* p_ppVerifier,
	int32_t* p_piOperationResult);

// 使用只读公钥句柄验证 Compact JWT；成功输出 Payload 原始 JSON 字节，不附加结尾零字符。
// 同一句柄可并发验签，但最后一次调用返回前不得销毁。
CREDCORE_C_API int CredCore_VerifyRs256JwtSignature(
	CREDCORE_PUBLIC_KEY_HANDLE p_pVerifier,
	const ST_FACADE_STRING_VIEW* p_pCompactJwt,
	ST_FACADE_MUTABLE_BUFFER* p_pDecodedPayloadJson,
	int32_t* p_piOperationResult);

// 释放本模块创建的公钥句柄；空句柄安全返回，调用方必须先结束同句柄的全部并发验签。
CREDCORE_C_API void CredCore_DestroyRs256PublicKeyVerifier(
	CREDCORE_PUBLIC_KEY_HANDLE p_pVerifier);

#ifdef __cplusplus
}
#endif

#endif // GUANGWZ_CRED_CORE_C_H
