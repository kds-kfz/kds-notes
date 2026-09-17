#include "QuoteBinaryProtocol.h"

#include <cmath>
#include <cstring>
#include <limits>

namespace
{
	static const std::uint32_t QUOTE_BINARY_MAGIC = 0x5451544DU;

	// 追加固定宽度小端整数，线上布局不依赖编译器结构体对齐。
	template <typename TValue>
	void AppendUnsigned(std::vector<unsigned char>& p_refBuffer,
		TValue p_uValue)
	{
		for (std::size_t szIndex = 0; szIndex < sizeof(TValue); ++szIndex)
		{
			p_refBuffer.push_back(static_cast<unsigned char>(
				(p_uValue >> (szIndex * 8U)) & 0xFFU));
		}
	}

	// 从当前位置读取固定宽度小端整数；失败时不越过输入边界。
	template <typename TValue>
	bool ReadUnsigned(const unsigned char* p_pBuffer,
		std::size_t p_szBufferLen, std::size_t& p_refOffset,
		TValue& p_refValue)
	{
		if (p_pBuffer == nullptr || p_refOffset > p_szBufferLen ||
			p_szBufferLen - p_refOffset < sizeof(TValue))
		{
			return false;
		}
		TValue uValue = 0;
		for (std::size_t szIndex = 0; szIndex < sizeof(TValue); ++szIndex)
		{
			uValue |= static_cast<TValue>(
				static_cast<TValue>(p_pBuffer[p_refOffset + szIndex]) <<
				(szIndex * 8U));
		}
		p_refOffset += sizeof(TValue);
		p_refValue = uValue;
		return true;
	}

	// 追加有符号整数的补码位模式。
	void AppendInt32(std::vector<unsigned char>& p_refBuffer,
		std::int32_t p_iValue)
	{
		AppendUnsigned<std::uint32_t>(p_refBuffer,
			static_cast<std::uint32_t>(p_iValue));
	}

	// 追加有符号 64 位整数的补码位模式。
	void AppendInt64(std::vector<unsigned char>& p_refBuffer,
		std::int64_t p_llValue)
	{
		AppendUnsigned<std::uint64_t>(p_refBuffer,
			static_cast<std::uint64_t>(p_llValue));
	}

	// 追加 double 的 IEEE 754 位模式。
	void AppendDouble(std::vector<unsigned char>& p_refBuffer,
		double p_dValue)
	{
		std::uint64_t ullBits = 0;
		static_assert(sizeof(ullBits) == sizeof(p_dValue),
			"double must be 64-bit");
		std::memcpy(&ullBits, &p_dValue, sizeof(ullBits));
		AppendUnsigned<std::uint64_t>(p_refBuffer, ullBits);
	}

	// 读取 double 的 IEEE 754 位模式。
	bool ReadDouble(const unsigned char* p_pBuffer,
		std::size_t p_szBufferLen, std::size_t& p_refOffset,
		double& p_refValue)
	{
		std::uint64_t ullBits = 0;
		if (!ReadUnsigned<std::uint64_t>(p_pBuffer, p_szBufferLen,
			p_refOffset, ullBits))
		{
			return false;
		}
		std::memcpy(&p_refValue, &ullBits, sizeof(p_refValue));
		return true;
	}

	// 校验 Symbol UTF-8，拒绝控制字符、NUL、过长编码、代理区和越界码点。
	bool IsValidSymbolUtf8(const std::string& p_refSymbol)
	{
		std::size_t szIndex = 0;
		while (szIndex < p_refSymbol.size())
		{
			const unsigned char chFirst = static_cast<unsigned char>(
				p_refSymbol[szIndex]);
			if (chFirst <= 0x7FU)
			{
				if (chFirst < 0x20U || chFirst == 0x7FU)
				{
					return false;
				}
				++szIndex;
				continue;
			}

			std::size_t szContinuation = 0;
			unsigned char chSecondMin = 0x80U;
			unsigned char chSecondMax = 0xBFU;
			if (chFirst >= 0xC2U && chFirst <= 0xDFU)
			{
				szContinuation = 1;
			}
			else if (chFirst >= 0xE0U && chFirst <= 0xEFU)
			{
				szContinuation = 2;
				if (chFirst == 0xE0U)
				{
					chSecondMin = 0xA0U;
				}
				else if (chFirst == 0xEDU)
				{
					chSecondMax = 0x9FU;
				}
			}
			else if (chFirst >= 0xF0U && chFirst <= 0xF4U)
			{
				szContinuation = 3;
				if (chFirst == 0xF0U)
				{
					chSecondMin = 0x90U;
				}
				else if (chFirst == 0xF4U)
				{
					chSecondMax = 0x8FU;
				}
			}
			else
			{
				return false;
			}
			if (p_refSymbol.size() - szIndex <= szContinuation)
			{
				return false;
			}
			const unsigned char chSecond = static_cast<unsigned char>(
				p_refSymbol[szIndex + 1]);
			if (chSecond < chSecondMin || chSecond > chSecondMax)
			{
				return false;
			}
			for (std::size_t szOffset = 2;
				szOffset <= szContinuation; ++szOffset)
			{
				const unsigned char chCurrent = static_cast<unsigned char>(
					p_refSymbol[szIndex + szOffset]);
				if (chCurrent < 0x80U || chCurrent > 0xBFU)
				{
					return false;
				}
			}
			szIndex += szContinuation + 1;
		}
		return true;
	}
}

ST_QUOTE_BINARY_TICK::ST_QUOTE_BINARY_TICK()
	: usVersion(QUOTE_BINARY_PROTOCOL_VERSION)
	, usPlatformVersion(0)
	, iSourceNo(0)
	, ullSourceEpoch(0)
	, ullIngressSequence(0)
	, strSymbol()
	, dBid(0.0)
	, dAsk(0.0)
	, dLast(0.0)
	, ullVolume(0)
	, ullVolumeExt(0)
	, ullFlags(0)
	, llServerTime(0)
	, llServerTimeMsc(0)
	, llIngressTimeMs(0)
{
}

bool ValidateQuoteBinaryTick(const ST_QUOTE_BINARY_TICK& p_refTick,
	std::string& p_refError)
{
	p_refError.clear();
	if (p_refTick.usVersion != QUOTE_BINARY_PROTOCOL_VERSION)
	{
		p_refError =
			"QUOTE_VERSION_UNSUPPORTED: quote protocol version must be 1";
		return false;
	}
	if (p_refTick.usPlatformVersion != 4 &&
		p_refTick.usPlatformVersion != 5)
	{
		p_refError =
			"QUOTE_PLATFORM_INVALID: platform version must be 4 or 5";
		return false;
	}
	if (p_refTick.iSourceNo <= 0 || p_refTick.ullSourceEpoch == 0 ||
		p_refTick.ullIngressSequence == 0)
	{
		p_refError =
			"QUOTE_SOURCE_INVALID: source No, SourceEpoch and Sequence must be positive";
		return false;
	}
	if (p_refTick.strSymbol.empty() ||
		p_refTick.strSymbol.size() > QUOTE_BINARY_MAX_SYMBOL_BYTES)
	{
		p_refError =
			"QUOTE_SYMBOL_INVALID: UTF-8 symbol length must be between 1 and 64 bytes";
		return false;
	}
	if (!IsValidSymbolUtf8(p_refTick.strSymbol))
	{
		p_refError =
			"QUOTE_SYMBOL_UTF8_INVALID: symbol must be valid UTF-8 without control characters";
		return false;
	}
	if (!std::isfinite(p_refTick.dBid) ||
		!std::isfinite(p_refTick.dAsk) || p_refTick.dBid <= 0.0 ||
		p_refTick.dAsk <= 0.0)
	{
		p_refError =
			"QUOTE_BID_ASK_INVALID: Bid and Ask must be finite positive values";
		return false;
	}
	if (!std::isfinite(p_refTick.dLast) || p_refTick.dLast < 0.0)
	{
		p_refError =
			"QUOTE_LAST_INVALID: Last must be a finite non-negative value";
		return false;
	}
	if (p_refTick.llServerTime <= 0 ||
		p_refTick.llServerTimeMsc <= 0 || p_refTick.llIngressTimeMs <= 0 ||
		p_refTick.llServerTimeMsc / 1000LL != p_refTick.llServerTime)
	{
		p_refError =
			"QUOTE_TIME_INVALID: raw server seconds/milliseconds and ingress time are inconsistent";
		return false;
	}
	return true;
}

bool EncodeQuoteBinaryTick(const ST_QUOTE_BINARY_TICK& p_refTick,
	std::vector<unsigned char>& p_refBuffer, std::string& p_refError)
{
	p_refBuffer.clear();
	if (!ValidateQuoteBinaryTick(p_refTick, p_refError))
	{
		return false;
	}
	if (p_refTick.strSymbol.size() >
		static_cast<std::size_t>((std::numeric_limits<std::uint32_t>::max)()))
	{
		p_refError =
			"QUOTE_SYMBOL_TOO_LARGE: symbol length exceeds uint32 range";
		return false;
	}

	p_refBuffer.reserve(QUOTE_BINARY_HEADER_SIZE + p_refTick.strSymbol.size());
	AppendUnsigned<std::uint32_t>(p_refBuffer, QUOTE_BINARY_MAGIC);
	AppendUnsigned<std::uint16_t>(p_refBuffer, p_refTick.usVersion);
	AppendUnsigned<std::uint16_t>(p_refBuffer,
		static_cast<std::uint16_t>(QUOTE_BINARY_HEADER_SIZE));
	AppendUnsigned<std::uint16_t>(p_refBuffer, p_refTick.usPlatformVersion);
	AppendUnsigned<std::uint16_t>(p_refBuffer, 0);
	AppendInt32(p_refBuffer, p_refTick.iSourceNo);
	AppendUnsigned<std::uint64_t>(p_refBuffer, p_refTick.ullSourceEpoch);
	AppendUnsigned<std::uint64_t>(p_refBuffer, p_refTick.ullIngressSequence);
	AppendUnsigned<std::uint32_t>(p_refBuffer,
		static_cast<std::uint32_t>(p_refTick.strSymbol.size()));
	AppendUnsigned<std::uint64_t>(p_refBuffer, p_refTick.ullFlags);
	AppendInt64(p_refBuffer, p_refTick.llServerTime);
	AppendInt64(p_refBuffer, p_refTick.llServerTimeMsc);
	AppendInt64(p_refBuffer, p_refTick.llIngressTimeMs);
	AppendDouble(p_refBuffer, p_refTick.dBid);
	AppendDouble(p_refBuffer, p_refTick.dAsk);
	AppendDouble(p_refBuffer, p_refTick.dLast);
	AppendUnsigned<std::uint64_t>(p_refBuffer, p_refTick.ullVolume);
	AppendUnsigned<std::uint64_t>(p_refBuffer, p_refTick.ullVolumeExt);
	p_refBuffer.insert(p_refBuffer.end(), p_refTick.strSymbol.begin(),
		p_refTick.strSymbol.end());
	return true;
}

bool DecodeQuoteBinaryTick(const unsigned char* p_pBuffer,
	std::size_t p_szBufferLen, ST_QUOTE_BINARY_TICK& p_refTick,
	std::string& p_refError)
{
	p_refTick = ST_QUOTE_BINARY_TICK();
	p_refError.clear();
	if (p_pBuffer == nullptr || p_szBufferLen < QUOTE_BINARY_HEADER_SIZE)
	{
		p_refError =
			"QUOTE_BUFFER_TOO_SHORT: payload is shorter than the v1 fixed header";
		return false;
	}

	std::size_t szOffset = 0;
	std::uint32_t uiMagic = 0;
	std::uint16_t usHeaderSize = 0;
	std::uint16_t usReserved = 0;
	std::uint32_t uiSourceNo = 0;
	std::uint32_t uiSymbolLen = 0;
	std::uint64_t ullServerTime = 0;
	std::uint64_t ullServerTimeMsc = 0;
	std::uint64_t ullIngressTimeMs = 0;
	if (!ReadUnsigned<std::uint32_t>(p_pBuffer, p_szBufferLen,
			szOffset, uiMagic) ||
		!ReadUnsigned<std::uint16_t>(p_pBuffer, p_szBufferLen,
			szOffset, p_refTick.usVersion) ||
		!ReadUnsigned<std::uint16_t>(p_pBuffer, p_szBufferLen,
			szOffset, usHeaderSize) ||
		!ReadUnsigned<std::uint16_t>(p_pBuffer, p_szBufferLen,
			szOffset, p_refTick.usPlatformVersion) ||
		!ReadUnsigned<std::uint16_t>(p_pBuffer, p_szBufferLen,
			szOffset, usReserved) ||
		!ReadUnsigned<std::uint32_t>(p_pBuffer, p_szBufferLen,
			szOffset, uiSourceNo) ||
		!ReadUnsigned<std::uint64_t>(p_pBuffer, p_szBufferLen,
			szOffset, p_refTick.ullSourceEpoch) ||
		!ReadUnsigned<std::uint64_t>(p_pBuffer, p_szBufferLen,
			szOffset, p_refTick.ullIngressSequence) ||
		!ReadUnsigned<std::uint32_t>(p_pBuffer, p_szBufferLen,
			szOffset, uiSymbolLen) ||
		!ReadUnsigned<std::uint64_t>(p_pBuffer, p_szBufferLen,
			szOffset, p_refTick.ullFlags) ||
		!ReadUnsigned<std::uint64_t>(p_pBuffer, p_szBufferLen,
			szOffset, ullServerTime) ||
		!ReadUnsigned<std::uint64_t>(p_pBuffer, p_szBufferLen,
			szOffset, ullServerTimeMsc) ||
		!ReadUnsigned<std::uint64_t>(p_pBuffer, p_szBufferLen,
			szOffset, ullIngressTimeMs) ||
		!ReadDouble(p_pBuffer, p_szBufferLen, szOffset, p_refTick.dBid) ||
		!ReadDouble(p_pBuffer, p_szBufferLen, szOffset, p_refTick.dAsk) ||
		!ReadDouble(p_pBuffer, p_szBufferLen, szOffset, p_refTick.dLast) ||
		!ReadUnsigned<std::uint64_t>(p_pBuffer, p_szBufferLen,
			szOffset, p_refTick.ullVolume) ||
		!ReadUnsigned<std::uint64_t>(p_pBuffer, p_szBufferLen,
			szOffset, p_refTick.ullVolumeExt))
	{
		p_refError =
			"QUOTE_HEADER_TRUNCATED: v1 fixed header cannot be decoded";
		return false;
	}
	if (uiMagic != QUOTE_BINARY_MAGIC ||
		p_refTick.usVersion != QUOTE_BINARY_PROTOCOL_VERSION ||
		usHeaderSize != QUOTE_BINARY_HEADER_SIZE || usReserved != 0 ||
		szOffset != QUOTE_BINARY_HEADER_SIZE)
	{
		p_refError =
			"QUOTE_HEADER_INVALID: magic, protocol version, header size or reserved field is invalid";
		return false;
	}
	if (uiSymbolLen == 0 || uiSymbolLen > QUOTE_BINARY_MAX_SYMBOL_BYTES ||
		p_szBufferLen - szOffset != uiSymbolLen)
	{
		p_refError =
			"QUOTE_LENGTH_INVALID: v1 payload must contain exactly one bounded Symbol";
		return false;
	}
	p_refTick.iSourceNo = static_cast<std::int32_t>(uiSourceNo);
	p_refTick.llServerTime = static_cast<std::int64_t>(ullServerTime);
	p_refTick.llServerTimeMsc = static_cast<std::int64_t>(ullServerTimeMsc);
	p_refTick.llIngressTimeMs = static_cast<std::int64_t>(ullIngressTimeMs);
	p_refTick.strSymbol.assign(reinterpret_cast<const char*>(
		p_pBuffer + szOffset), uiSymbolLen);
	return ValidateQuoteBinaryTick(p_refTick, p_refError);
}
