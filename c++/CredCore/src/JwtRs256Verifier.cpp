#include "CredCore.h"
#include "JwtRs256Verifier.h"

#include <nlohmann/json.hpp>

#include <openssl/bn.h>
#include <openssl/core_names.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/param_build.h>
#include <openssl/params.h>
#include <openssl/rsa.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#if OPENSSL_VERSION_MAJOR != 3 || OPENSSL_VERSION_MINOR != 5 || OPENSSL_VERSION_PATCH != 6
#error CredCore RS256 must be compiled with OpenSSL 3.5.6 headers.
#endif

namespace credcore::detail
{
	ST_RS256_PUBLIC_KEY_VERIFIER::ST_RS256_PUBLIC_KEY_VERIFIER(
		std::string p_strKeyId, EVP_PKEY* p_pPublicKey) noexcept
		: strKeyId(std::move(p_strKeyId)), pPublicKey(p_pPublicKey)
	{
	}

	ST_RS256_PUBLIC_KEY_VERIFIER::~ST_RS256_PUBLIC_KEY_VERIFIER()
	{
		EVP_PKEY_free(pPublicKey);
	}
}

namespace
{
	constexpr std::size_t s_szJwtHeaderMaxBytes = 4096;
	constexpr std::size_t s_szJwtKeyIdMaxBytes = 128;
	constexpr std::size_t s_szJwkModulusMaxBase64UrlBytes = 683;
	constexpr std::size_t s_szJwkExponentMaxBase64UrlBytes = 6;
	constexpr int s_iRsaMinBits = 2048;
	constexpr int s_iRsaMaxBits = 4096;

	// 保存 Compact JWT 三段及验签输入的只读视图；所有字段均借用原 JWT 字符串内存。
	struct ST_COMPACT_JWT_PARTS
	{
		std::string_view strHeader; // 第一段 Base64URL Header；借用调用方 JWT 字符串内存。
		std::string_view strPayload; // 第二段 Base64URL Payload；借用调用方 JWT 字符串内存。
		std::string_view strSignature; // 第三段 Base64URL Signature；借用调用方 JWT 字符串内存。
		std::string_view strSigningInput; // 原始 Header.Payload 字节；借用内存并按原样参与验签。
	};

	template <typename T, void (*FreeFunction)(T*)>
	using TOpenSslPtr = std::unique_ptr<T, decltype(FreeFunction)>;

	// 以下智能指针分别绑定 OpenSSL 对应释放函数，离开作用域时自动归还资源。
	using TBigNumberPtr = TOpenSslPtr<BIGNUM, BN_free>;
	using TDigestContextPtr = TOpenSslPtr<EVP_MD_CTX, EVP_MD_CTX_free>;
	using TParameterBuilderPtr = TOpenSslPtr<OSSL_PARAM_BLD, OSSL_PARAM_BLD_free>;
	using TParametersPtr = TOpenSslPtr<OSSL_PARAM, OSSL_PARAM_free>;
	using TPublicKeyPtr = TOpenSslPtr<EVP_PKEY, EVP_PKEY_free>;
	using TPublicKeyContextPtr = TOpenSslPtr<EVP_PKEY_CTX, EVP_PKEY_CTX_free>;

	// 返回运行时 libcrypto 是否与编译头文件版本完全一致；无输入、无副作用且可并发调用。
	bool IsExpectedOpenSslVersion() noexcept
	{
		return OpenSSL_version_num() == OPENSSL_VERSION_NUMBER;
	}

	// 输入单个 Base64URL 字符并返回 0..63；非法字符返回 -1，可并发调用。
	int Base64UrlValue(const char p_cValue) noexcept
	{
		// 大写字母连续映射到 0..25。
		if (p_cValue >= 'A' && p_cValue <= 'Z')
		{
			return p_cValue - 'A';
		}
		// 小写字母连续映射到 26..51。
		if (p_cValue >= 'a' && p_cValue <= 'z')
		{
			return p_cValue - 'a' + 26;
		}
		// 数字连续映射到 52..61。
		if (p_cValue >= '0' && p_cValue <= '9')
		{
			return p_cValue - '0' + 52;
		}
		// URL 安全字符 '-' 和 '_' 分别替代标准 Base64 的 '+' 和 '/'，对应 62、63。
		if (p_cValue == '-')
		{
			return 62;
		}
		if (p_cValue == '_')
		{
			return 63;
		}
		return -1;
	}

	// 严格解码无填充 Base64URL；成功覆盖输出字节，失败清空输出且返回 false，可并发调用。
	bool DecodeBase64Url(const std::string_view p_refEncoded,
		std::vector<unsigned char>& p_refDecoded)
	{
		p_refDecoded.clear();
		// JWT/JWK 段不得为空；无填充 Base64URL 的合法长度余数只能是 0、2、3，
		// 余数为 1 时仅剩 6 位，无法还原出合法尾字节，必须按格式错误拒绝。
		if (p_refEncoded.empty() || p_refEncoded.size() % 4 == 1)
		{
			return false;
		}

		// 每 4 个编码字符最多还原 3 字节；向上取整只预留容量，不改变实际输出长度。
		p_refDecoded.reserve((p_refEncoded.size() * 3 + 3) / 4);
		std::uint32_t u32Buffer = 0;
		unsigned int uBufferedBits = 0;
		for (const char chValue : p_refEncoded)
		{
			const int iDecodedValue = Base64UrlValue(chValue);
			// 严格模式只接受 RFC 4648 Base64URL 字母表，不兼容标准 Base64 的 '+'、'/' 和填充 '='。
			if (iDecodedValue < 0)
			{
				p_refDecoded.clear();
				return false;
			}

			// 一个 Base64URL 字符承载 6 位：先把已有残余位左移 6 位，再把当前字符值拼到低位。
			// 每轮末尾都会移除完整字节，缓冲区最多只保留 5 个残余位，不会无限增长。
			u32Buffer = (u32Buffer << 6) | static_cast<std::uint32_t>(iDecodedValue);
			uBufferedBits += 6;
			// 累计到至少 8 位时可还原一个字节；单次只新增 6 位，因此每轮最多输出一个字节。
			if (uBufferedBits >= 8)
			{
				uBufferedBits -= 8;
				// 右移剩余位数取得本轮最高的完整 8 位，保持网络字节序的从高位到低位输出顺序。
				p_refDecoded.push_back(static_cast<unsigned char>(u32Buffer >> uBufferedBits));
				// 已输出的高位必须丢弃，只保留低 uBufferedBits 位供下一个字符继续拼接；
				// 没有残余位时直接清零，避免执行无意义的移位掩码计算。
				u32Buffer &= uBufferedBits == 0 ? 0U : (1U << uBufferedBits) - 1U;
			}
		}

		// 尾部不足 8 位只能是编码产生的零填充位；非零尾位属于非规范编码，
		// 若接受会使多个不同字符串映射到同一字节序列，破坏 JWT/JWK 的唯一表示。
		if (uBufferedBits != 0 && u32Buffer != 0)
		{
			p_refDecoded.clear();
			return false;
		}
		return true;
	}

	// 将输入 JWT 拆成三个非空 Base64URL 视图；失败返回格式错误，输出仅在成功时可用，可并发调用。
	int SplitCompactJwt(const std::string& p_refCompactJwt,
		ST_COMPACT_JWT_PARTS& p_refParts) noexcept
	{
		if (p_refCompactJwt.empty())
		{
			return credcore::CREDCORE_RS256_INVALID_ARGUMENT;
		}

		const std::size_t szFirstDot = p_refCompactJwt.find('.');
		// Compact JWT 必须恰好包含 Header.Payload.Signature 三个非空段。
		if (szFirstDot == std::string::npos || szFirstDot == 0)
		{
			return credcore::CREDCORE_RS256_INVALID_JWT_FORMAT;
		}
		const std::size_t szSecondDot = p_refCompactJwt.find('.', szFirstDot + 1);
		if (szSecondDot == std::string::npos || szSecondDot == szFirstDot + 1 ||
			szSecondDot + 1 >= p_refCompactJwt.size() ||
			p_refCompactJwt.find('.', szSecondDot + 1) != std::string::npos)
		{
			return credcore::CREDCORE_RS256_INVALID_JWT_FORMAT;
		}

		const std::string_view strJwt(p_refCompactJwt);
		// 四个视图都借用原字符串，不复制令牌；原字符串必须覆盖这些视图的整个使用期。
		p_refParts.strHeader = strJwt.substr(0, szFirstDot);
		p_refParts.strPayload = strJwt.substr(
			szFirstDot + 1, szSecondDot - szFirstDot - 1);
		p_refParts.strSignature = strJwt.substr(szSecondDot + 1);
		// RS256 的签名输入是原始 Header.Payload 字节，不能解码后再序列化，否则会改变签名内容。
		p_refParts.strSigningInput = strJwt.substr(0, szSecondDot);
		return credcore::CREDCORE_RS256_SUCCESS;
	}

	// 校验 kid 的长度和控制字符边界；不解释 kid 业务含义，无副作用且可并发调用。
	bool IsValidKeyId(const std::string& p_refKeyId) noexcept
	{
		// 128 字节是本库的查找键和日志边界；禁止控制字符，避免同一 kid 产生不可见差异或污染日志。
		if (p_refKeyId.empty() || p_refKeyId.size() > s_szJwtKeyIdMaxBytes)
		{
			return false;
		}
		for (const unsigned char chValue : p_refKeyId)
		{
			if (chValue < 0x20 || chValue == 0x7f)
			{
				return false;
			}
		}
		return true;
	}

	// 解码并严格解析 JWT Header；成功输出未受信 kid，失败清空输出，可并发调用。
	int ParseRs256Header(const ST_COMPACT_JWT_PARTS& p_refParts,
		std::string& p_refKeyId)
	{
		p_refKeyId.clear();
		std::vector<unsigned char> aDecodedHeader;
		if (!DecodeBase64Url(p_refParts.strHeader, aDecodedHeader))
		{
			return credcore::CREDCORE_RS256_INVALID_BASE64URL;
		}
		// 解码器已拒绝空段，这里仍保留空结果防御，避免其契约将来放宽后绕过 JSON 对象校验；
		// 4 KiB 足以容纳受支持的 JOSE Header，同时限制未认证输入的 JSON 解析内存和 CPU 消耗。
		if (aDecodedHeader.empty() || aDecodedHeader.size() > s_szJwtHeaderMaxBytes)
		{
			return credcore::CREDCORE_RS256_INVALID_HEADER_JSON;
		}

		const std::string strHeaderJson(
			reinterpret_cast<const char*>(aDecodedHeader.data()), aDecodedHeader.size());
		// 第三个参数禁用异常并用 discarded 映射错误；第四个参数拒绝非标准 JSON 注释，
		// 防止不同 JOSE 实现对同一 Header 产生不同解释。
		const nlohmann::json stHeader = nlohmann::json::parse(
			strHeaderJson, nullptr, false, false);
		if (stHeader.is_discarded() || !stHeader.is_object())
		{
			return credcore::CREDCORE_RS256_INVALID_HEADER_JSON;
		}

		// 算法必须由服务端白名单固定为 RS256，不能接受令牌自行选择其他算法。
		const auto stAlgorithm = stHeader.find("alg");
		if (stAlgorithm == stHeader.end() || !stAlgorithm->is_string() ||
			stAlgorithm->get_ref<const std::string&>() != "RS256")
		{
			return credcore::CREDCORE_RS256_UNSUPPORTED_ALGORITHM;
		}
		// 本库没有实现 JOSE crit 扩展协商；出现任何关键扩展都必须关闭放行，不能忽略后继续验签。
		const auto stCritical = stHeader.find("crit");
		if (stCritical != stHeader.end())
		{
			return credcore::CREDCORE_RS256_UNSUPPORTED_ALGORITHM;
		}
		// RFC 7797 的 b64=false 会改变签名输入语义；当前仅支持标准的缺省 b64=true。
		const auto stBase64Payload = stHeader.find("b64");
		if (stBase64Payload != stHeader.end() &&
			(!stBase64Payload->is_boolean() || !stBase64Payload->get<bool>()))
		{
			return credcore::CREDCORE_RS256_UNSUPPORTED_ALGORITHM;
		}

		const auto stKeyId = stHeader.find("kid");
		if (stKeyId == stHeader.end() || !stKeyId->is_string())
		{
			return credcore::CREDCORE_RS256_MISSING_KEY_ID;
		}
		p_refKeyId = stKeyId->get<std::string>();
		if (!IsValidKeyId(p_refKeyId))
		{
			p_refKeyId.clear();
			return credcore::CREDCORE_RS256_MISSING_KEY_ID;
		}
		return credcore::CREDCORE_RS256_SUCCESS;
	}

	// 校验 RSA n/e 的符号、奇偶、指数和模数位数；不取得指针所有权，可并发调用。
	int ValidateRsaNumbers(const BIGNUM* p_pModulus, const BIGNUM* p_pExponent) noexcept
	{
		// RSA 模数和指数必须为正奇数，指数不能为 1；指数限制在 32 位内，
		// 既覆盖常用 65537，也阻止异常大指数造成不受控的验签开销。
		if (p_pModulus == nullptr || p_pExponent == nullptr ||
			BN_is_zero(p_pModulus) || BN_is_negative(p_pModulus) ||
			!BN_is_odd(p_pModulus) || BN_is_zero(p_pExponent) ||
			BN_is_negative(p_pExponent) || !BN_is_odd(p_pExponent) ||
			BN_num_bits(p_pExponent) > 32 || BN_is_one(p_pExponent))
		{
			return credcore::CREDCORE_RS256_INVALID_JWK;
		}

		// 低于 2048 位不满足当前安全基线，高于 4096 位超出服务约定并会放大单次验签成本。
		const int iModulusBits = BN_num_bits(p_pModulus);
		if (iModulusBits < s_iRsaMinBits || iModulusBits > s_iRsaMaxBits)
		{
			return credcore::CREDCORE_RS256_INVALID_RSA_KEY_SIZE;
		}
		return credcore::CREDCORE_RS256_SUCCESS;
	}

	// 清空当前线程的 OpenSSL 错误队列，避免把本次失败泄漏到后续调用；线程间互不影响。
	void ClearOpenSslErrors() noexcept
	{
		ERR_clear_error();
	}
}

BGN_NAMESPACE_CREDCORE
extern "C"
{
	// 输入完整 Compact JWT，成功输出 SHA-256 小写十六进制指纹；失败清空输出，可并发调用。
	int GetCompactJwtSha256Fingerprint(const std::string& p_refCompactJwt,
		std::string& p_refFingerprintHex)
	{
		p_refFingerprintHex.clear();
		try
		{
			ST_COMPACT_JWT_PARTS stParts{};
			int iResult = SplitCompactJwt(p_refCompactJwt, stParts);
			if (iResult != CREDCORE_RS256_SUCCESS)
			{
				return iResult;
			}
			std::vector<unsigned char> aScratch;
			// 指纹会作为验签缓存键；先拒绝非规范三段，避免畸形令牌进入“已验证”缓存命名空间。
			if (!DecodeBase64Url(stParts.strHeader, aScratch) ||
				!DecodeBase64Url(stParts.strPayload, aScratch) ||
				!DecodeBase64Url(stParts.strSignature, aScratch))
			{
				return CREDCORE_RS256_INVALID_BASE64URL;
			}
			// DLL 只对编译时 OpenSSL 版本完成兼容验证，运行版本不一致时按安全失败处理。
			if (!IsExpectedOpenSslVersion())
			{
				return CREDCORE_RS256_OPENSSL_VERSION_MISMATCH;
			}

			std::array<unsigned char, EVP_MAX_MD_SIZE> aDigest{};
			unsigned int uDigestLength = 0;
			ClearOpenSslErrors();
			// SHA-256 摘要固定为 32 字节；成功码和长度必须同时满足，避免把依赖异常当成有效指纹。
			if (EVP_Digest(p_refCompactJwt.data(), p_refCompactJwt.size(),
				aDigest.data(), &uDigestLength, EVP_sha256(), nullptr) != 1 ||
				uDigestLength != 32)
			{
				ClearOpenSslErrors();
				return CREDCORE_RS256_OPENSSL_ERROR;
			}
			ClearOpenSslErrors();

			constexpr char s_aHex[] = "0123456789abcdef";
			// 每个摘要字节展开为高、低两个十六进制半字节，所以输出长度固定为摘要长度的两倍。
			p_refFingerprintHex.resize(uDigestLength * 2);
			for (unsigned int uIndex = 0; uIndex < uDigestLength; ++uIndex)
			{
				// 高 4 位和低 4 位分别查小写十六进制表，生成稳定的 64 字符 SHA-256 缓存键。
				p_refFingerprintHex[uIndex * 2] = s_aHex[aDigest[uIndex] >> 4];
				p_refFingerprintHex[uIndex * 2 + 1] = s_aHex[aDigest[uIndex] & 0x0f];
			}
			return CREDCORE_RS256_SUCCESS;
		}
		catch (...)
		{
			p_refFingerprintHex.clear();
			return CREDCORE_RS256_INTERNAL_ERROR;
		}
	}

	// 输入完整 Compact JWT，成功输出未受信 kid；失败清空输出，可并发调用。
	int ReadRs256JwtKeyId(const std::string& p_refCompactJwt,
		std::string& p_refKeyId)
	{
		p_refKeyId.clear();
		try
		{
			ST_COMPACT_JWT_PARTS stParts{};
			const int iResult = SplitCompactJwt(p_refCompactJwt, stParts);
			return iResult == CREDCORE_RS256_SUCCESS
				? ParseRs256Header(stParts, p_refKeyId) : iResult;
		}
		catch (...)
		{
			p_refKeyId.clear();
			return CREDCORE_RS256_INTERNAL_ERROR;
		}
	}

	// 输入已校验来源的 JWK kid/n/e，成功输出独占句柄；失败输出空句柄，可并发创建不同句柄。
	int CreateRs256PublicKeyVerifier(const std::string& p_refKeyId,
		const std::string& p_refModulusBase64Url,
		const std::string& p_refExponentBase64Url,
		CREDCORE_RS256_PUBLIC_KEY_HANDLE& p_refVerifier)
	{
		p_refVerifier = nullptr;
		try
		{
			// 4096 位模数的无填充 Base64URL 最长 683 字节，32 位指数最长 6 字节；
			// 在解码和分配前限制输入，避免 JWKS 中的超长字段放大资源消耗。
			if (!IsValidKeyId(p_refKeyId) || p_refModulusBase64Url.empty() ||
				p_refExponentBase64Url.empty() ||
				p_refModulusBase64Url.size() > s_szJwkModulusMaxBase64UrlBytes ||
				p_refExponentBase64Url.size() > s_szJwkExponentMaxBase64UrlBytes)
			{
				return CREDCORE_RS256_INVALID_ARGUMENT;
			}
			// 公钥对象由 OpenSSL 动态库创建和释放，版本不一致可能破坏 ABI 和 Provider 行为。
			if (!IsExpectedOpenSslVersion())
			{
				return CREDCORE_RS256_OPENSSL_VERSION_MISMATCH;
			}

			std::vector<unsigned char> aModulusBytes;
			std::vector<unsigned char> aExponentBytes;
			// JWK Base64urlUInt 必须使用最短无符号大端表示，前导零会产生同值不同编码；
			// BN_bin2bn 的长度参数是 int，因此转换前还要证明 size_t 可以无损收窄。
			if (!DecodeBase64Url(p_refModulusBase64Url, aModulusBytes) ||
				!DecodeBase64Url(p_refExponentBase64Url, aExponentBytes) ||
				aModulusBytes.front() == 0 || aExponentBytes.front() == 0 ||
				aModulusBytes.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)()) ||
				aExponentBytes.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)()))
			{
				return CREDCORE_RS256_INVALID_JWK;
			}

			ClearOpenSslErrors();
			TBigNumberPtr pModulus(BN_bin2bn(aModulusBytes.data(),
				static_cast<int>(aModulusBytes.size()), nullptr), BN_free);
			TBigNumberPtr pExponent(BN_bin2bn(aExponentBytes.data(),
				static_cast<int>(aExponentBytes.size()), nullptr), BN_free);
			if (!pModulus || !pExponent)
			{
				ClearOpenSslErrors();
				return CREDCORE_RS256_OPENSSL_ERROR;
			}

			const int iNumberResult = ValidateRsaNumbers(pModulus.get(), pExponent.get());
			if (iNumberResult != CREDCORE_RS256_SUCCESS)
			{
				ClearOpenSslErrors();
				return iNumberResult;
			}

			TParameterBuilderPtr pParameterBuilder(
				OSSL_PARAM_BLD_new(), OSSL_PARAM_BLD_free);
			if (!pParameterBuilder ||
				OSSL_PARAM_BLD_push_BN(pParameterBuilder.get(),
					OSSL_PKEY_PARAM_RSA_N, pModulus.get()) != 1 ||
				OSSL_PARAM_BLD_push_BN(pParameterBuilder.get(),
					OSSL_PKEY_PARAM_RSA_E, pExponent.get()) != 1)
			{
				ClearOpenSslErrors();
				return CREDCORE_RS256_OPENSSL_ERROR;
			}
			TParametersPtr pParameters(
				OSSL_PARAM_BLD_to_param(pParameterBuilder.get()), OSSL_PARAM_free);
			TPublicKeyContextPtr pPublicKeyContext(
				EVP_PKEY_CTX_new_from_name(nullptr, "RSA", nullptr), EVP_PKEY_CTX_free);
			if (!pParameters || !pPublicKeyContext ||
				EVP_PKEY_fromdata_init(pPublicKeyContext.get()) != 1)
			{
				ClearOpenSslErrors();
				return CREDCORE_RS256_OPENSSL_ERROR;
			}

			EVP_PKEY* pRawPublicKey = nullptr;
			if (EVP_PKEY_fromdata(pPublicKeyContext.get(), &pRawPublicKey,
				EVP_PKEY_PUBLIC_KEY, pParameters.get()) != 1 || pRawPublicKey == nullptr)
			{
				ClearOpenSslErrors();
				return CREDCORE_RS256_OPENSSL_ERROR;
			}
			TPublicKeyPtr pPublicKey(pRawPublicKey, EVP_PKEY_free);
			// 即使按 RSA 参数创建，仍向 Provider 查询实际密钥类型，防止 Provider 实现与请求类型不一致。
			if (EVP_PKEY_is_a(pPublicKey.get(), "RSA") != 1)
			{
				ClearOpenSslErrors();
				return CREDCORE_RS256_INVALID_JWK;
			}

			auto* pVerifier = new (std::nothrow) detail::ST_RS256_PUBLIC_KEY_VERIFIER(
				p_refKeyId, pPublicKey.get());
			if (pVerifier == nullptr)
			{
				ClearOpenSslErrors();
				return CREDCORE_RS256_INTERNAL_ERROR;
			}
			// 从此处起公钥所有权转交句柄；release 防止局部智能指针与句柄析构发生二次释放。
			pPublicKey.release();
			p_refVerifier = pVerifier;
			ClearOpenSslErrors();
			return CREDCORE_RS256_SUCCESS;
		}
		catch (...)
		{
			p_refVerifier = nullptr;
			ClearOpenSslErrors();
			return CREDCORE_RS256_INTERNAL_ERROR;
		}
	}

	// 使用只读公钥句柄校验 JWT RS256 签名；成功输出 Payload 原文，失败清空输出。
	// 同一不可变句柄可并发验签，但调用期间不得销毁句柄。
	int VerifyRs256JwtSignature(CREDCORE_RS256_PUBLIC_KEY_HANDLE p_pVerifier,
		const std::string& p_refCompactJwt,
		std::string& p_refDecodedPayloadJson)
	{
		p_refDecodedPayloadJson.clear();
		try
		{
			if (p_pVerifier == nullptr || p_refCompactJwt.empty())
			{
				return CREDCORE_RS256_INVALID_ARGUMENT;
			}
			// 验签路径同样拒绝运行时 OpenSSL 版本漂移，不能因依赖不匹配降级放行。
			if (!IsExpectedOpenSslVersion())
			{
				return CREDCORE_RS256_OPENSSL_VERSION_MISMATCH;
			}

			ST_COMPACT_JWT_PARTS stParts{};
			int iResult = SplitCompactJwt(p_refCompactJwt, stParts);
			if (iResult != CREDCORE_RS256_SUCCESS)
			{
				return iResult;
			}

			std::string strKeyId;
			iResult = ParseRs256Header(stParts, strKeyId);
			if (iResult != CREDCORE_RS256_SUCCESS)
			{
				return iResult;
			}
			const auto* pVerifier =
				static_cast<const detail::ST_RS256_PUBLIC_KEY_VERIFIER*>(p_pVerifier);
			// Header 的 kid 只能选择与当前句柄绑定的公钥，防止调用方选错钥后仍继续验证。
			if (strKeyId != pVerifier->strKeyId)
			{
				return CREDCORE_RS256_KEY_ID_MISMATCH;
			}

			std::vector<unsigned char> aPayload;
			std::vector<unsigned char> aSignature;
			if (!DecodeBase64Url(stParts.strPayload, aPayload) ||
				!DecodeBase64Url(stParts.strSignature, aSignature))
			{
				return CREDCORE_RS256_INVALID_BASE64URL;
			}
			// RSASSA-PKCS1-v1_5 签名长度必须等于 RSA 模数长度，先拒绝畸形长度再进入密码运算。
			if (aSignature.size() !=
				static_cast<std::size_t>(EVP_PKEY_get_size(pVerifier->pPublicKey)))
			{
				return CREDCORE_RS256_SIGNATURE_MISMATCH;
			}

			ClearOpenSslErrors();
			TDigestContextPtr pDigestContext(EVP_MD_CTX_new(), EVP_MD_CTX_free);
			EVP_PKEY_CTX* pPublicKeyContext = nullptr;
			// JWA RS256 固定使用 SHA-256 与 RSASSA-PKCS1-v1_5；验签数据必须是未经重编码的原始签名输入。
			if (!pDigestContext || EVP_DigestVerifyInit(pDigestContext.get(),
				&pPublicKeyContext, EVP_sha256(), nullptr, pVerifier->pPublicKey) != 1 ||
				pPublicKeyContext == nullptr ||
				EVP_PKEY_CTX_set_rsa_padding(pPublicKeyContext, RSA_PKCS1_PADDING) <= 0 ||
				EVP_DigestVerifyUpdate(pDigestContext.get(), stParts.strSigningInput.data(),
					stParts.strSigningInput.size()) != 1)
			{
				ClearOpenSslErrors();
				return CREDCORE_RS256_OPENSSL_ERROR;
			}

			const int iVerifyResult = EVP_DigestVerifyFinal(pDigestContext.get(),
				aSignature.data(), aSignature.size());
			ClearOpenSslErrors();
			// OpenSSL 约定 1=签名有效、0=签名不匹配、负值=密码库内部错误，三类结果不能合并。
			if (iVerifyResult == 0)
			{
				return CREDCORE_RS256_SIGNATURE_MISMATCH;
			}
			if (iVerifyResult < 0)
			{
				return CREDCORE_RS256_OPENSSL_ERROR;
			}

			// Payload 只有在签名有效后才向调用方暴露，避免上层误用未经认证的 login 等声明。
			p_refDecodedPayloadJson.assign(
				reinterpret_cast<const char*>(aPayload.data()), aPayload.size());
			return CREDCORE_RS256_SUCCESS;
		}
		catch (...)
		{
			p_refDecodedPayloadJson.clear();
			ClearOpenSslErrors();
			return CREDCORE_RS256_INTERNAL_ERROR;
		}
	}

	// 释放由 CreateRs256PublicKeyVerifier 创建的句柄；空句柄安全，调用期间不得存在并发验签。
	void DestroyRs256PublicKeyVerifier(CREDCORE_RS256_PUBLIC_KEY_HANDLE p_pVerifier)
	{
		delete static_cast<detail::ST_RS256_PUBLIC_KEY_VERIFIER*>(p_pVerifier);
	}
}
END_NAMESPACE_CREDCORE
