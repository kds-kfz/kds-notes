#ifndef GUANGWZ_CRED_CORE_H
#define GUANGWZ_CRED_CORE_H

#include "CredCore_define.h"

#if defined(_WIN32) || defined(_WIN64)
#if defined(CREDCORE_STATIC)
#define CREDCORE_API
#elif defined(CREDCORE_EXPORTS)
// DLL 构建由 CredCore.def 统一控制导出白名单，避免源码宏意外扩大公共 ABI。
#define CREDCORE_API
#else
#define CREDCORE_API __declspec(dllimport)
#endif
#else
#define CREDCORE_API __attribute__((visibility("default")))
#endif

#include <cstdint>
#include <string>

BGN_NAMESPACE_CREDCORE
	// 指向 CredCore 私有只读 RSA 公钥对象的不透明句柄；必须由创建它的同一模块释放。
	using CREDCORE_RS256_PUBLIC_KEY_HANDLE = void*;

	// RS256 接口使用独立返回码，避免改变历史授权接口的返回语义。
	enum EN_CREDCORE_RS256_RESULT : int
	{
		CREDCORE_RS256_SUCCESS = 0, // 操作成功，输出参数可以使用。
		CREDCORE_RS256_INVALID_ARGUMENT = -100, // 必填输入为空、句柄无效或输出参数不满足契约。
		CREDCORE_RS256_INVALID_JWT_FORMAT = -101, // Compact JWT 不是严格的 Header.Payload.Signature 三段格式。
		CREDCORE_RS256_INVALID_BASE64URL = -102, // JWT 或 JWK 字段不是无填充 Base64URL 编码。
		CREDCORE_RS256_INVALID_HEADER_JSON = -103, // JWT Header 无法解析为满足约束的 JSON 对象。
		CREDCORE_RS256_UNSUPPORTED_ALGORITHM = -104, // JWT alg 不是固定允许的 RS256。
		CREDCORE_RS256_MISSING_KEY_ID = -105, // JWT Header 缺少非空 kid。
		CREDCORE_RS256_KEY_ID_MISMATCH = -106, // JWT kid 与验签句柄绑定的 kid 不一致。
		CREDCORE_RS256_INVALID_JWK = -107, // JWK n/e 无法构造合法 RSA 公钥。
		CREDCORE_RS256_INVALID_RSA_KEY_SIZE = -108, // RSA 模数位数不在 2048..4096 安全边界内。
		CREDCORE_RS256_SIGNATURE_MISMATCH = -109, // JWT RS256 签名与指定公钥不匹配。
		CREDCORE_RS256_OPENSSL_ERROR = -110, // OpenSSL Provider、摘要或公钥操作失败。
		CREDCORE_RS256_OPENSSL_VERSION_MISMATCH = -111, // 运行时 OpenSSL 与 CredCore 固定版本不一致。
		CREDCORE_RS256_INTERNAL_ERROR = -112 // 未分类的内部异常，调用方不得据此放行请求。
	};

#ifdef __cplusplus
extern "C"
{
#endif
	// 输入任意二进制安全字符串，使用历史固定密钥执行 AES-CBC/PKCS#7 加密并返回 Base64。
	// AES 内核异常会继续抛出；函数不保存调用状态，可并发调用。
	CREDCORE_API std::string Aes(const std::string& p_refSource);
	// 输入历史 Base64 密文并执行 AES-CBC 解密；格式或填充非法时返回空字符串。
	// 函数不保存调用状态，可并发调用。
	CREDCORE_API std::string Deaes(const std::string& p_refSource);
	// 输入字节指针和长度并返回 Base64；非零长度配空指针时返回空字符串，可并发调用。
	CREDCORE_API std::string Base64Encode(
		const unsigned char* p_pBytes, unsigned int p_uiLength);
	// 输入标准 Base64 并返回原始字节；非法字符沿用历史行为，返回此前已解码前缀，可并发调用。
	CREDCORE_API std::string Base64Decode(const std::string& p_refEncoded);
	// 输入明文客户信息并返回供 BuildFeatrue 使用的历史加密文本；函数可并发调用。
	CREDCORE_API std::string MakeFeatrue(const std::string p_strClientInfo);
	// 输入客户端加密信息、原文长度和授权天数，成功返回 0 并覆盖输出授权码。
	// 失败返回历史负值；函数不保存调用状态，可并发调用。
	CREDCORE_API int BuildFeatrue(std::string p_strClientInfo,
		unsigned int p_uiClientInfoLength, int p_iAuthDay,
		std::string& p_refFeature);
	// 输入明文客户信息和授权天数，成功返回 0 并输出自然日授权码，失败返回负值。
	// 输出在入口处清空；函数不保存调用状态，可并发调用。
	CREDCORE_API int BuildFeatrueByPlainClientInfo(
		const std::string& p_refClientInfo, int p_iAuthDay,
		std::string& p_refFeature);
	// 输入明文客户信息和有效秒数，成功返回 0 并输出 UTC 秒授权码，失败返回负值。
	// 输出在入口处清空；函数不保存调用状态，可并发调用。
	CREDCORE_API int BuildFeatrueByPlainClientInfoSeconds(
		const std::string& p_refClientInfo, std::string& p_refFeature,
		std::int64_t p_i64AuthSeconds = 86400);
	// 输入明文客户信息和授权码，兼容自然日与 UTC 秒格式；成功返回 0 并输出解密明文。
	// 失败返回历史负值；函数不保存调用状态，可并发调用。
	CREDCORE_API int AuthFeatrueByPlainClientInfo(
		const std::string& p_refClientInfo,
		const std::string& p_refFeature,
		std::string& p_refFeatureInfo);
	// 输入客户信息和传统自然日授权码，成功返回 0 并输出解密明文，失败返回原 nsdk 负值。
	// 函数不保存调用状态，可并发调用。
	CREDCORE_API int AuthFeatrue(const std::string p_strClientInfo,
		std::string p_strFeature, std::string& p_refFeatureInfo);

	// 输入完整 Compact JWT，成功返回 0 并输出 64 字符 SHA-256 小写十六进制指纹。
	// 格式、Base64URL、OpenSSL 或内部失败返回 EN_CREDCORE_RS256_RESULT，失败时清空输出；可并发调用。
	CREDCORE_API int GetCompactJwtSha256Fingerprint(
		const std::string& p_refCompactJwt,
		std::string& p_refFingerprintHex);
	// 输入完整 Compact JWT，严格解析 Header；成功输出仅供选公钥的未受信 kid。
	// 非 RS256、格式或 JSON 失败返回 EN_CREDCORE_RS256_RESULT 并清空输出；可并发调用。
	CREDCORE_API int ReadRs256JwtKeyId(
		const std::string& p_refCompactJwt,
		std::string& p_refKeyId);
	// 输入 JWK kid 和 Base64URLUInt n/e，成功输出 CredCore 独占的只读 RSA 公钥句柄。
	// 参数、JWK、密钥位数或 OpenSSL 失败返回 EN_CREDCORE_RS256_RESULT 并输出空句柄；可并发创建。
	CREDCORE_API int CreateRs256PublicKeyVerifier(
		const std::string& p_refKeyId,
		const std::string& p_refModulusBase64Url,
		const std::string& p_refExponentBase64Url,
		CREDCORE_RS256_PUBLIC_KEY_HANDLE& p_refVerifier);
	// 输入只读公钥句柄和 Compact JWT；签名成功返回 0 并输出 Payload 原始 JSON 文本。
	// 失败返回 EN_CREDCORE_RS256_RESULT 并清空输出；同一句柄可并发调用，但调用期间不得销毁。
	CREDCORE_API int VerifyRs256JwtSignature(
		CREDCORE_RS256_PUBLIC_KEY_HANDLE p_pVerifier,
		const std::string& p_refCompactJwt,
		std::string& p_refDecodedPayloadJson);
	// 输入由本模块创建的句柄并释放其公钥；空句柄安全返回。
	// 调用前必须结束对该句柄的全部验签，函数本身不与同句柄的销毁或验签并发。
	CREDCORE_API void DestroyRs256PublicKeyVerifier(
		CREDCORE_RS256_PUBLIC_KEY_HANDLE p_pVerifier);
#ifdef __cplusplus
}
#endif
END_NAMESPACE_CREDCORE

#endif // GUANGWZ_CRED_CORE_H
