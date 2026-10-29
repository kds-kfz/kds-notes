#include "CredCoreC.h"

#include "CredCore.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

namespace
{
	// 校验所有公共结构共有的 ABI 头。预留位必须为零，避免新旧调用方对未发布语义产生分歧。
	int ValidateCommonHeader(std::uint32_t p_uiStructSize,
		std::uint32_t p_uiExpectedSize, std::uint32_t p_uiAbiVersion,
		std::uint32_t p_uiFlags, const std::uint64_t* p_pReserved,
		std::size_t p_szReservedCount) noexcept
	{
		if (p_uiStructSize < p_uiExpectedSize || p_uiFlags != 0U ||
			p_pReserved == nullptr)
		{
			return FACADE_INVALID_ARGUMENT;
		}
		if (p_uiAbiVersion != FACADE_ABI_VERSION)
		{
			return FACADE_ABI_MISMATCH;
		}
		for (std::size_t szIndex = 0; szIndex < p_szReservedCount; ++szIndex)
		{
			if (p_pReserved[szIndex] != 0U)
			{
				return FACADE_INVALID_ARGUMENT;
			}
		}
		return FACADE_SUCCESS;
	}

	// 校验只读字符串视图。空字符串允许空指针，非空数据必须在整个同步调用期间有效。
	int ValidateStringView(const ST_FACADE_STRING_VIEW* p_pView) noexcept
	{
		if (p_pView == nullptr)
		{
			return FACADE_INVALID_ARGUMENT;
		}
		const int iHeaderResult = ValidateCommonHeader(p_pView->uiStructSize,
			sizeof(*p_pView), p_pView->uiAbiVersion, p_pView->uiFlags,
			p_pView->aullReserved,
			sizeof(p_pView->aullReserved) / sizeof(p_pView->aullReserved[0]));
		return iHeaderResult == FACADE_SUCCESS && p_pView->uiLength > 0U &&
			p_pView->pszData == nullptr ? FACADE_INVALID_ARGUMENT : iHeaderResult;
	}

	// 校验并清空输出长度。容量为零时允许空指针，用于调用方查询所需长度。
	int PrepareMutableBuffer(ST_FACADE_MUTABLE_BUFFER* p_pBuffer) noexcept
	{
		if (p_pBuffer == nullptr)
		{
			return FACADE_INVALID_ARGUMENT;
		}
		const int iHeaderResult = ValidateCommonHeader(p_pBuffer->uiStructSize,
			sizeof(*p_pBuffer), p_pBuffer->uiAbiVersion, p_pBuffer->uiFlags,
			p_pBuffer->aullReserved,
			sizeof(p_pBuffer->aullReserved) / sizeof(p_pBuffer->aullReserved[0]));
		if (iHeaderResult != FACADE_SUCCESS)
		{
			return iHeaderResult;
		}
		if (p_pBuffer->uiCapacity > 0U && p_pBuffer->pData == nullptr)
		{
			return FACADE_INVALID_ARGUMENT;
		}
		p_pBuffer->uiLength = 0U;
		return FACADE_SUCCESS;
	}

	// 将同步视图复制为库内字符串，保证旧 C++ 实现只接收由 CredCore 自己分配和释放的 STL 对象。
	std::string CopyStringView(const ST_FACADE_STRING_VIEW& p_refView)
	{
		return p_refView.uiLength == 0U ? std::string() :
			std::string(p_refView.pszData, p_refView.uiLength);
	}

	// 原子复制完整输出。容量不足时只回填所需长度，不写入部分结果。
	int CopyStringResult(const std::string& p_refResult,
		ST_FACADE_MUTABLE_BUFFER& p_refOutput) noexcept
	{
		p_refOutput.uiLength = p_refResult.size();
		if (p_refOutput.uiCapacity < p_refResult.size())
		{
			return FACADE_BUFFER_TOO_SMALL;
		}
		if (!p_refResult.empty())
		{
			std::memcpy(p_refOutput.pData, p_refResult.data(), p_refResult.size());
		}
		return FACADE_SUCCESS;
	}

	// 校验 ABI 信息结构的输入头，成功后由调用函数覆盖全部公开输出字段。
	int ValidateAbiInfo(const ST_FACADE_ABI_INFO* p_pInfo) noexcept
	{
		return p_pInfo == nullptr ? FACADE_INVALID_ARGUMENT :
			ValidateCommonHeader(p_pInfo->uiStructSize, sizeof(*p_pInfo),
				p_pInfo->uiAbiVersion, p_pInfo->uiFlags, p_pInfo->aullReserved,
				sizeof(p_pInfo->aullReserved) / sizeof(p_pInfo->aullReserved[0]));
	}

	// 向固定数组写入诊断文本；数组末尾始终保留零字符，字符串过长时截断。
	template <std::size_t t_szCapacity>
	void CopyFixedText(char (&p_refTarget)[t_szCapacity], const char* p_szText) noexcept
	{
		std::memset(p_refTarget, 0, sizeof(p_refTarget));
		if (p_szText == nullptr || t_szCapacity == 0U)
		{
			return;
		}
		const std::size_t szLength = std::min<std::size_t>(
			std::strlen(p_szText), t_szCapacity - 1U);
		std::memcpy(p_refTarget, p_szText, szLength);
	}
}

extern "C"
{
	// 输出只读版本快照；函数不访问全局可变状态，可并发调用。
	int CredCore_GetAbiInfo(ST_FACADE_ABI_INFO* p_pInfo)
	{
		const int iValidationResult = ValidateAbiInfo(p_pInfo);
		if (iValidationResult != FACADE_SUCCESS)
		{
			return iValidationResult;
		}
		p_pInfo->uiFlags = 0U;
		CopyFixedText(p_pInfo->szFacadeName, "CredCore");
		CopyFixedText(p_pInfo->szImplementationVersion, "1.0.0.3");
		CopyFixedText(p_pInfo->szDependencyVersion, "OpenSSL-3.5.6");
		CopyFixedText(p_pInfo->szBuildId, __DATE__ " " __TIME__);
		return FACADE_SUCCESS;
	}

	// 将 C ABI 输入转换为库内字符串并调用历史兼容实现；异常不得越过 DLL 边界。
	int CredCore_AuthFeatureByPlainClientInfo(
		const ST_FACADE_STRING_VIEW* p_pClientInfo,
		const ST_FACADE_STRING_VIEW* p_pFeature,
		ST_FACADE_MUTABLE_BUFFER* p_pFeatureInfo,
		std::int32_t* p_piOperationResult)
	{
		const int iClientResult = ValidateStringView(p_pClientInfo);
		const int iFeatureResult = ValidateStringView(p_pFeature);
		const int iOutputResult = PrepareMutableBuffer(p_pFeatureInfo);
		if (iClientResult != FACADE_SUCCESS || iFeatureResult != FACADE_SUCCESS ||
			iOutputResult != FACADE_SUCCESS || p_piOperationResult == nullptr)
		{
			return iClientResult != FACADE_SUCCESS ? iClientResult :
				(iFeatureResult != FACADE_SUCCESS ? iFeatureResult :
					(iOutputResult != FACADE_SUCCESS ? iOutputResult : FACADE_INVALID_ARGUMENT));
		}
		try
		{
			std::string strFeatureInfo;
			*p_piOperationResult = credcore::AuthFeatrueByPlainClientInfo(
				CopyStringView(*p_pClientInfo), CopyStringView(*p_pFeature), strFeatureInfo);
			return *p_piOperationResult == 0 ?
				CopyStringResult(strFeatureInfo, *p_pFeatureInfo) : FACADE_SUCCESS;
		}
		catch (...)
		{
			*p_piOperationResult = -1;
			return FACADE_INTERNAL_ERROR;
		}
	}

	// 生成自然日授权码并原子复制到调用方缓冲区；异常映射为 Facade 内部错误。
	int CredCore_BuildFeatureByPlainClientInfo(
		const ST_FACADE_STRING_VIEW* p_pClientInfo, std::int32_t p_iAuthDay,
		ST_FACADE_MUTABLE_BUFFER* p_pFeature,
		std::int32_t* p_piOperationResult)
	{
		const int iClientResult = ValidateStringView(p_pClientInfo);
		const int iOutputResult = PrepareMutableBuffer(p_pFeature);
		if (iClientResult != FACADE_SUCCESS || iOutputResult != FACADE_SUCCESS ||
			p_piOperationResult == nullptr)
		{
			return iClientResult != FACADE_SUCCESS ? iClientResult :
				(iOutputResult != FACADE_SUCCESS ? iOutputResult : FACADE_INVALID_ARGUMENT);
		}
		try
		{
			std::string strFeature;
			*p_piOperationResult = credcore::BuildFeatrueByPlainClientInfo(
				CopyStringView(*p_pClientInfo), p_iAuthDay, strFeature);
			return *p_piOperationResult == 0 ?
				CopyStringResult(strFeature, *p_pFeature) : FACADE_SUCCESS;
		}
		catch (...)
		{
			*p_piOperationResult = -1;
			return FACADE_INTERNAL_ERROR;
		}
	}

	// 计算 JWT 指纹；密码结果与 ABI/缓冲区结果分开返回，调用方必须同时检查。
	int CredCore_GetCompactJwtSha256Fingerprint(
		const ST_FACADE_STRING_VIEW* p_pCompactJwt,
		ST_FACADE_MUTABLE_BUFFER* p_pFingerprintHex,
		std::int32_t* p_piOperationResult)
	{
		const int iTokenResult = ValidateStringView(p_pCompactJwt);
		const int iOutputResult = PrepareMutableBuffer(p_pFingerprintHex);
		if (iTokenResult != FACADE_SUCCESS || iOutputResult != FACADE_SUCCESS ||
			p_piOperationResult == nullptr)
		{
			return iTokenResult != FACADE_SUCCESS ? iTokenResult :
				(iOutputResult != FACADE_SUCCESS ? iOutputResult : FACADE_INVALID_ARGUMENT);
		}
		try
		{
			std::string strFingerprint;
			*p_piOperationResult = credcore::GetCompactJwtSha256Fingerprint(
				CopyStringView(*p_pCompactJwt), strFingerprint);
			return *p_piOperationResult == credcore::CREDCORE_RS256_SUCCESS ?
				CopyStringResult(strFingerprint, *p_pFingerprintHex) : FACADE_SUCCESS;
		}
		catch (...)
		{
			*p_piOperationResult = credcore::CREDCORE_RS256_INTERNAL_ERROR;
			return FACADE_INTERNAL_ERROR;
		}
	}

	// 解析 JWT kid；签名成功前只把结果用于公钥选择，不能作为身份声明。
	int CredCore_ReadRs256JwtKeyId(const ST_FACADE_STRING_VIEW* p_pCompactJwt,
		ST_FACADE_MUTABLE_BUFFER* p_pKeyId,
		std::int32_t* p_piOperationResult)
	{
		const int iTokenResult = ValidateStringView(p_pCompactJwt);
		const int iOutputResult = PrepareMutableBuffer(p_pKeyId);
		if (iTokenResult != FACADE_SUCCESS || iOutputResult != FACADE_SUCCESS ||
			p_piOperationResult == nullptr)
		{
			return iTokenResult != FACADE_SUCCESS ? iTokenResult :
				(iOutputResult != FACADE_SUCCESS ? iOutputResult : FACADE_INVALID_ARGUMENT);
		}
		try
		{
			std::string strKeyId;
			*p_piOperationResult = credcore::ReadRs256JwtKeyId(
				CopyStringView(*p_pCompactJwt), strKeyId);
			return *p_piOperationResult == credcore::CREDCORE_RS256_SUCCESS ?
				CopyStringResult(strKeyId, *p_pKeyId) : FACADE_SUCCESS;
		}
		catch (...)
		{
			*p_piOperationResult = credcore::CREDCORE_RS256_INTERNAL_ERROR;
			return FACADE_INTERNAL_ERROR;
		}
	}

	// 构造只读公钥句柄；失败时强制清空输出，避免调用方误用旧句柄。
	int CredCore_CreateRs256PublicKeyVerifier(
		const ST_FACADE_STRING_VIEW* p_pKeyId,
		const ST_FACADE_STRING_VIEW* p_pModulusBase64Url,
		const ST_FACADE_STRING_VIEW* p_pExponentBase64Url,
		CREDCORE_PUBLIC_KEY_HANDLE* p_ppVerifier,
		std::int32_t* p_piOperationResult)
	{
		if (p_ppVerifier != nullptr)
		{
			*p_ppVerifier = nullptr;
		}
		const int iKeyIdResult = ValidateStringView(p_pKeyId);
		const int iModulusResult = ValidateStringView(p_pModulusBase64Url);
		const int iExponentResult = ValidateStringView(p_pExponentBase64Url);
		if (iKeyIdResult != FACADE_SUCCESS || iModulusResult != FACADE_SUCCESS ||
			iExponentResult != FACADE_SUCCESS || p_ppVerifier == nullptr ||
			p_piOperationResult == nullptr)
		{
			return iKeyIdResult != FACADE_SUCCESS ? iKeyIdResult :
				(iModulusResult != FACADE_SUCCESS ? iModulusResult :
					(iExponentResult != FACADE_SUCCESS ? iExponentResult : FACADE_INVALID_ARGUMENT));
		}
		try
		{
			credcore::CREDCORE_RS256_PUBLIC_KEY_HANDLE pVerifier = nullptr;
			*p_piOperationResult = credcore::CreateRs256PublicKeyVerifier(
				CopyStringView(*p_pKeyId), CopyStringView(*p_pModulusBase64Url),
				CopyStringView(*p_pExponentBase64Url), pVerifier);
			*p_ppVerifier = pVerifier;
			return FACADE_SUCCESS;
		}
		catch (...)
		{
			*p_piOperationResult = credcore::CREDCORE_RS256_INTERNAL_ERROR;
			return FACADE_INTERNAL_ERROR;
		}
	}

	// 执行验签并复制 Payload；同一只读句柄可并发使用，函数不保存请求状态。
	int CredCore_VerifyRs256JwtSignature(CREDCORE_PUBLIC_KEY_HANDLE p_pVerifier,
		const ST_FACADE_STRING_VIEW* p_pCompactJwt,
		ST_FACADE_MUTABLE_BUFFER* p_pDecodedPayloadJson,
		std::int32_t* p_piOperationResult)
	{
		const int iTokenResult = ValidateStringView(p_pCompactJwt);
		const int iOutputResult = PrepareMutableBuffer(p_pDecodedPayloadJson);
		if (p_pVerifier == nullptr || iTokenResult != FACADE_SUCCESS ||
			iOutputResult != FACADE_SUCCESS || p_piOperationResult == nullptr)
		{
			return p_pVerifier == nullptr ? FACADE_INVALID_ARGUMENT :
				(iTokenResult != FACADE_SUCCESS ? iTokenResult :
					(iOutputResult != FACADE_SUCCESS ? iOutputResult : FACADE_INVALID_ARGUMENT));
		}
		try
		{
			std::string strPayload;
			*p_piOperationResult = credcore::VerifyRs256JwtSignature(p_pVerifier,
				CopyStringView(*p_pCompactJwt), strPayload);
			return *p_piOperationResult == credcore::CREDCORE_RS256_SUCCESS ?
				CopyStringResult(strPayload, *p_pDecodedPayloadJson) : FACADE_SUCCESS;
		}
		catch (...)
		{
			*p_piOperationResult = credcore::CREDCORE_RS256_INTERNAL_ERROR;
			return FACADE_INTERNAL_ERROR;
		}
	}

	// 释放句柄的实现仍由 CredCore 完成；空句柄沿用原接口安全语义。
	void CredCore_DestroyRs256PublicKeyVerifier(CREDCORE_PUBLIC_KEY_HANDLE p_pVerifier)
	{
		credcore::DestroyRs256PublicKeyVerifier(p_pVerifier);
	}
}
