#ifndef GUANGWZ_CRED_CORE_CXX_ADAPTER_H
#define GUANGWZ_CRED_CORE_CXX_ADAPTER_H

#include "CredCoreC.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

// 本文件只在调用方内部把 std::string 转换为 CredCore C ABI 视图；STL 对象不会跨越 DLL 边界。
namespace credcore
{
	using CREDCORE_RS256_PUBLIC_KEY_HANDLE = CREDCORE_PUBLIC_KEY_HANDLE;
	constexpr int CREDCORE_RS256_SUCCESS = CREDCORE_C_RS256_SUCCESS;
	constexpr int CREDCORE_RS256_INVALID_ARGUMENT = CREDCORE_C_RS256_INVALID_ARGUMENT;
	constexpr int CREDCORE_RS256_INVALID_JWT_FORMAT = CREDCORE_C_RS256_INVALID_JWT_FORMAT;
	constexpr int CREDCORE_RS256_INVALID_BASE64URL = CREDCORE_C_RS256_INVALID_BASE64URL;
	constexpr int CREDCORE_RS256_INVALID_HEADER_JSON = CREDCORE_C_RS256_INVALID_HEADER_JSON;
	constexpr int CREDCORE_RS256_UNSUPPORTED_ALGORITHM = CREDCORE_C_RS256_UNSUPPORTED_ALGORITHM;
	constexpr int CREDCORE_RS256_MISSING_KEY_ID = CREDCORE_C_RS256_MISSING_KEY_ID;
	constexpr int CREDCORE_RS256_KEY_ID_MISMATCH = CREDCORE_C_RS256_KEY_ID_MISMATCH;
	constexpr int CREDCORE_RS256_INVALID_JWK = CREDCORE_C_RS256_INVALID_JWK;
	constexpr int CREDCORE_RS256_INVALID_RSA_KEY_SIZE = CREDCORE_C_RS256_INVALID_RSA_KEY_SIZE;
	constexpr int CREDCORE_RS256_SIGNATURE_MISMATCH = CREDCORE_C_RS256_SIGNATURE_MISMATCH;
	constexpr int CREDCORE_RS256_OPENSSL_ERROR = CREDCORE_C_RS256_OPENSSL_ERROR;
	constexpr int CREDCORE_RS256_OPENSSL_VERSION_MISMATCH = CREDCORE_C_RS256_OPENSSL_VERSION_MISMATCH;
	constexpr int CREDCORE_RS256_INTERNAL_ERROR = CREDCORE_C_RS256_INTERNAL_ERROR;

	// 将调用方字符串转换为同步只读视图；返回结构不拥有字符数据，原字符串必须覆盖整个调用期。
	inline ST_FACADE_STRING_VIEW MakeStringView(const std::string& p_refValue) noexcept
	{
		ST_FACADE_STRING_VIEW stView = {};
		stView.uiStructSize = sizeof(stView);
		stView.uiAbiVersion = FACADE_ABI_VERSION;
		stView.pszData = p_refValue.data();
		stView.uiLength = p_refValue.size();
		return stView;
	}

	// 用调用方 vector 建立可写输出视图；vector 只在本地存在，DLL 只借用其连续存储空间。
	inline ST_FACADE_MUTABLE_BUFFER MakeMutableBuffer(
		std::vector<std::uint8_t>& p_refBuffer) noexcept
	{
		ST_FACADE_MUTABLE_BUFFER stBuffer = {};
		stBuffer.uiStructSize = sizeof(stBuffer);
		stBuffer.uiAbiVersion = FACADE_ABI_VERSION;
		stBuffer.pData = p_refBuffer.empty() ? nullptr : p_refBuffer.data();
		stBuffer.uiCapacity = p_refBuffer.size();
		return stBuffer;
	}

	// 执行返回字符串的 C ABI 调用。若估算容量不足，仅按 DLL 回填的精确长度重试一次。
	// 返回密码操作结果；ABI 或内存失败返回 p_iFacadeFailure，失败时清空输出，可并发调用。
	template <typename TFunction>
	inline int InvokeStringOperation(std::size_t p_szInitialCapacity,
		std::string& p_refOutput, int p_iFacadeFailure, TFunction p_clFunction)
	{
		p_refOutput.clear();
		try
		{
			std::vector<std::uint8_t> aBuffer(p_szInitialCapacity);
			ST_FACADE_MUTABLE_BUFFER stBuffer = MakeMutableBuffer(aBuffer);
			std::int32_t iOperationResult = p_iFacadeFailure;
			int iFacadeResult = p_clFunction(&stBuffer, &iOperationResult);
			if (iFacadeResult == FACADE_BUFFER_TOO_SMALL)
			{
				// DLL 返回的是完整结果所需字节数；禁止部分结果拼接，以免改变令牌或 JWT 文本。
				aBuffer.resize(stBuffer.uiLength);
				stBuffer = MakeMutableBuffer(aBuffer);
				iFacadeResult = p_clFunction(&stBuffer, &iOperationResult);
			}
			if (iFacadeResult != FACADE_SUCCESS || iOperationResult != 0 ||
				stBuffer.uiLength > aBuffer.size())
			{
				return iFacadeResult == FACADE_SUCCESS ?
					static_cast<int>(iOperationResult) : p_iFacadeFailure;
			}
			if (stBuffer.uiLength > 0U)
			{
				p_refOutput.assign(reinterpret_cast<const char*>(aBuffer.data()),
					stBuffer.uiLength);
			}
			return static_cast<int>(iOperationResult);
		}
		catch (...)
		{
			p_refOutput.clear();
			return p_iFacadeFailure;
		}
	}

	// 兼容 V3 现有调用语义，但实际只把字节视图传入 DLL，不跨边界传递 std::string。
	inline int AuthFeatrueByPlainClientInfo(const std::string& p_refClientInfo,
		const std::string& p_refFeature, std::string& p_refFeatureInfo)
	{
		const ST_FACADE_STRING_VIEW stClientInfo = MakeStringView(p_refClientInfo);
		const ST_FACADE_STRING_VIEW stFeature = MakeStringView(p_refFeature);
		return InvokeStringOperation(std::max<std::size_t>(p_refFeature.size(), 256U),
			p_refFeatureInfo, -1, [&](ST_FACADE_MUTABLE_BUFFER* p_pOutput,
				std::int32_t* p_piOperationResult)
			{
				return CredCore_AuthFeatureByPlainClientInfo(&stClientInfo, &stFeature,
					p_pOutput, p_piOperationResult);
			});
	}

	// 兼容 V3 现有自然日授权码生成调用；容量估算溢出时直接走 DLL 的精确长度回填路径。
	inline int BuildFeatrueByPlainClientInfo(const std::string& p_refClientInfo,
		int p_iAuthDay, std::string& p_refFeature)
	{
		const ST_FACADE_STRING_VIEW stClientInfo = MakeStringView(p_refClientInfo);
		const std::size_t szInitialCapacity = p_refClientInfo.size() <=
			(std::numeric_limits<std::size_t>::max() - 1024U) / 4U ?
			p_refClientInfo.size() * 4U + 1024U : 0U;
		return InvokeStringOperation(szInitialCapacity, p_refFeature, -1,
			[&](ST_FACADE_MUTABLE_BUFFER* p_pOutput, std::int32_t* p_piOperationResult)
			{
				return CredCore_BuildFeatureByPlainClientInfo(&stClientInfo, p_iAuthDay,
					p_pOutput, p_piOperationResult);
			});
	}

	// 计算 JWT 指纹；成功输出固定 64 字节，ABI 失败映射为内部错误并关闭缓存命中路径。
	inline int GetCompactJwtSha256Fingerprint(const std::string& p_refCompactJwt,
		std::string& p_refFingerprintHex)
	{
		const ST_FACADE_STRING_VIEW stCompactJwt = MakeStringView(p_refCompactJwt);
		return InvokeStringOperation(64U, p_refFingerprintHex,
			CREDCORE_RS256_INTERNAL_ERROR,
			[&](ST_FACADE_MUTABLE_BUFFER* p_pOutput, std::int32_t* p_piOperationResult)
			{
				return CredCore_GetCompactJwtSha256Fingerprint(&stCompactJwt,
					p_pOutput, p_piOperationResult);
			});
	}

	// 解析未受信 kid；128 字节是 CredCore 固定上限，容量不足仍按 ABI 所需长度重试。
	inline int ReadRs256JwtKeyId(const std::string& p_refCompactJwt,
		std::string& p_refKeyId)
	{
		const ST_FACADE_STRING_VIEW stCompactJwt = MakeStringView(p_refCompactJwt);
		return InvokeStringOperation(128U, p_refKeyId, CREDCORE_RS256_INTERNAL_ERROR,
			[&](ST_FACADE_MUTABLE_BUFFER* p_pOutput, std::int32_t* p_piOperationResult)
			{
				return CredCore_ReadRs256JwtKeyId(&stCompactJwt, p_pOutput,
					p_piOperationResult);
			});
	}

	// 创建只读公钥句柄；句柄所有权只在操作成功后转移给调用方。
	inline int CreateRs256PublicKeyVerifier(const std::string& p_refKeyId,
		const std::string& p_refModulusBase64Url,
		const std::string& p_refExponentBase64Url,
		CREDCORE_RS256_PUBLIC_KEY_HANDLE& p_refVerifier)
	{
		p_refVerifier = nullptr;
		const ST_FACADE_STRING_VIEW stKeyId = MakeStringView(p_refKeyId);
		const ST_FACADE_STRING_VIEW stModulus = MakeStringView(p_refModulusBase64Url);
		const ST_FACADE_STRING_VIEW stExponent = MakeStringView(p_refExponentBase64Url);
		std::int32_t iOperationResult = CREDCORE_RS256_INTERNAL_ERROR;
		const int iFacadeResult = CredCore_CreateRs256PublicKeyVerifier(&stKeyId,
			&stModulus, &stExponent, &p_refVerifier, &iOperationResult);
		return iFacadeResult == FACADE_SUCCESS ?
			static_cast<int>(iOperationResult) : CREDCORE_RS256_INTERNAL_ERROR;
	}

	// 验证 RS256 并返回 Payload 文本；预分配 JWT 长度可避免正常请求执行两次密码运算。
	inline int VerifyRs256JwtSignature(CREDCORE_RS256_PUBLIC_KEY_HANDLE p_pVerifier,
		const std::string& p_refCompactJwt, std::string& p_refDecodedPayloadJson)
	{
		const ST_FACADE_STRING_VIEW stCompactJwt = MakeStringView(p_refCompactJwt);
		return InvokeStringOperation(p_refCompactJwt.size(), p_refDecodedPayloadJson,
			CREDCORE_RS256_INTERNAL_ERROR,
			[&](ST_FACADE_MUTABLE_BUFFER* p_pOutput, std::int32_t* p_piOperationResult)
			{
				return CredCore_VerifyRs256JwtSignature(p_pVerifier, &stCompactJwt,
					p_pOutput, p_piOperationResult);
			});
	}

	// 释放 C ABI 句柄；空句柄由 DLL 按契约安全忽略。
	inline void DestroyRs256PublicKeyVerifier(
		CREDCORE_RS256_PUBLIC_KEY_HANDLE p_pVerifier) noexcept
	{
		CredCore_DestroyRs256PublicKeyVerifier(p_pVerifier);
	}
}

#endif // GUANGWZ_CRED_CORE_CXX_ADAPTER_H
