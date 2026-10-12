#ifndef GUANGWZ_CRED_CORE_H
#define GUANGWZ_CRED_CORE_H

#include "CredCore_define.h"

#if defined(_WIN32) || defined(_WIN64)
#if defined(CREDCORE_STATIC)
#define CREDCORE_API
#elif defined(CREDCORE_EXPORTS)
#define CREDCORE_API __declspec(dllexport)
#else
#define CREDCORE_API __declspec(dllimport)
#endif
#else
#define CREDCORE_API __attribute__((visibility("default")))
#endif

#include <cstdint>
#include <string>

BGN_NAMESPACE_CREDCORE
#ifdef __cplusplus
extern "C"
{
#endif
	// 使用授权协议固定密钥执行 AES-CBC 加密，并返回 Base64 文本。
	CREDCORE_API std::string Aes(const std::string& p_refSource);
	// 解码 Base64 并执行 AES-CBC 解密，校验并移除 PKCS#7 填充。
	CREDCORE_API std::string Deaes(const std::string& p_refSource);
	// 将指定字节序列编码为 Base64 文本。
	CREDCORE_API std::string Base64Encode(
		const unsigned char* p_pBytes, unsigned int p_uiLength);
	// 将 Base64 文本解码为原始字节序列。
	CREDCORE_API std::string Base64Decode(const std::string& p_refEncoded);
	// 客户端根据明文客户信息生成供 BuildFeatrue 使用的加密客户信息。
	CREDCORE_API std::string MakeFeatrue(const std::string p_strClientInfo);
	// 根据客户端加密信息生成按自然日有效的授权码。
	CREDCORE_API int BuildFeatrue(std::string p_strClientInfo,
		unsigned int p_uiClientInfoLength, int p_iAuthDay,
		std::string& p_refFeature);
	// 根据明文客户信息生成按自然日有效的授权码。
	CREDCORE_API int BuildFeatrueByPlainClientInfo(
		const std::string& p_refClientInfo, int p_iAuthDay,
		std::string& p_refFeature);
	// 根据明文客户信息生成按 UTC 秒有效的授权码。
	CREDCORE_API int BuildFeatrueByPlainClientInfoSeconds(
		const std::string& p_refClientInfo, std::string& p_refFeature,
		std::int64_t p_i64AuthSeconds = 86400);
	// 校验明文客户信息授权码，同时兼容自然日和 UTC 秒两种格式。
	CREDCORE_API int AuthFeatrueByPlainClientInfo(
		const std::string& p_refClientInfo,
		const std::string& p_refFeature,
		std::string& p_refFeatureInfo);
	// 校验传统自然日授权码，返回码保持与原 nsdk 接口一致。
	CREDCORE_API int AuthFeatrue(const std::string p_strClientInfo,
		std::string p_strFeature, std::string& p_refFeatureInfo);
#ifdef __cplusplus
}
#endif
END_NAMESPACE_CREDCORE

#endif // GUANGWZ_CRED_CORE_H
