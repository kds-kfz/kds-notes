#include "CompressionFacade.h"

#include <snappy.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <new>

namespace
{
	constexpr uint64_t s_ullMaximumInputLimit = 1024ULL * 1024ULL * 1024ULL;
	constexpr uint64_t s_ullMaximumOutputLimit = 4ULL * 1024ULL * 1024ULL * 1024ULL;

	class ICompressionCodec
	{
	public:
		virtual ~ICompressionCodec() = default;
		virtual size_t GetMaxCompressedLength(size_t p_uiInputLength) const = 0;
		virtual bool Compress(const uint8_t* p_pInput, size_t p_uiInputLength,
			uint8_t* p_pOutput, size_t* p_puiOutputLength) const = 0;
		virtual bool GetUncompressedLength(const uint8_t* p_pInput,
			size_t p_uiInputLength, size_t* p_puiOutputLength) const = 0;
		virtual bool Decompress(const uint8_t* p_pInput, size_t p_uiInputLength,
			uint8_t* p_pOutput) const = 0;
	};

	class CSnappyCompressionCodec final : public ICompressionCodec
	{
	public:
		size_t GetMaxCompressedLength(size_t p_uiInputLength) const override
		{
			return snappy::MaxCompressedLength(p_uiInputLength);
		}

		bool Compress(const uint8_t* p_pInput, size_t p_uiInputLength,
			uint8_t* p_pOutput, size_t* p_puiOutputLength) const override
		{
			snappy::RawCompress(reinterpret_cast<const char*>(p_pInput),
				p_uiInputLength, reinterpret_cast<char*>(p_pOutput), p_puiOutputLength);
			return true;
		}

		bool GetUncompressedLength(const uint8_t* p_pInput, size_t p_uiInputLength,
			size_t* p_puiOutputLength) const override
		{
			return snappy::GetUncompressedLength(
				reinterpret_cast<const char*>(p_pInput), p_uiInputLength,
				p_puiOutputLength);
		}

		bool Decompress(const uint8_t* p_pInput, size_t p_uiInputLength,
			uint8_t* p_pOutput) const override
		{
			return snappy::RawUncompress(reinterpret_cast<const char*>(p_pInput),
				p_uiInputLength, reinterpret_cast<char*>(p_pOutput));
		}
	};

	class ICompressionCodecFactory
	{
	public:
		virtual ~ICompressionCodecFactory() = default;
		virtual std::unique_ptr<ICompressionCodec> Create(
			EN_COMPRESSION_ALGORITHM p_enAlgorithm) const = 0;
	};

	class CCompressionCodecFactory final : public ICompressionCodecFactory
	{
	public:
		std::unique_ptr<ICompressionCodec> Create(
			EN_COMPRESSION_ALGORITHM p_enAlgorithm) const override
		{
			if (p_enAlgorithm == COMPRESSION_ALGORITHM_SNAPPY)
			{
				return std::unique_ptr<ICompressionCodec>(new CSnappyCompressionCodec());
			}
			return std::unique_ptr<ICompressionCodec>();
		}
	};

	class CCodecHandle final
	{
	public:
		CCodecHandle(std::unique_ptr<ICompressionCodec> p_ptrCodec,
			uint64_t p_ullMaxInputBytes, uint64_t p_ullMaxOutputBytes)
			: m_ptrCodec(std::move(p_ptrCodec)),
			m_ullMaxInputBytes(p_ullMaxInputBytes),
			m_ullMaxOutputBytes(p_ullMaxOutputBytes)
		{
		}

		std::unique_ptr<ICompressionCodec> m_ptrCodec; // 私有 Codec 所有权；随句柄析构释放。
		uint64_t m_ullMaxInputBytes; // 单次输入字节上限；创建后不可修改。
		uint64_t m_ullMaxOutputBytes; // 单次输出字节上限；创建后不可修改。
	};

	bool IsZeroReserved(const uint64_t* p_paullReserved)
	{
		return p_paullReserved[0] == 0U && p_paullReserved[1] == 0U &&
			p_paullReserved[2] == 0U && p_paullReserved[3] == 0U;
	}

	bool IsValidInput(const ST_FACADE_BUFFER_VIEW* p_pInput)
	{
		return p_pInput != nullptr &&
			p_pInput->uiStructSize >= sizeof(ST_FACADE_BUFFER_VIEW) &&
			p_pInput->uiAbiVersion == FACADE_ABI_VERSION &&
			p_pInput->uiFlags == 0U && IsZeroReserved(p_pInput->aullReserved) &&
			(p_pInput->pData != nullptr || p_pInput->uiLength == 0U);
	}

	bool IsValidOutput(ST_FACADE_MUTABLE_BUFFER* p_pOutput)
	{
		return p_pOutput != nullptr &&
			p_pOutput->uiStructSize >= sizeof(ST_FACADE_MUTABLE_BUFFER) &&
			p_pOutput->uiAbiVersion == FACADE_ABI_VERSION &&
			p_pOutput->uiFlags == 0U && IsZeroReserved(p_pOutput->aullReserved) &&
			(p_pOutput->pData != nullptr || p_pOutput->uiCapacity == 0U);
	}

	CCodecHandle* GetHandle(COMPRESSION_CODEC_HANDLE p_pCodec)
	{
		return static_cast<CCodecHandle*>(p_pCodec);
	}

	void CopyText(char* p_pszDestination, size_t p_uiCapacity, const char* p_pszSource)
	{
		const size_t szLength = (std::min)(std::strlen(p_pszSource), p_uiCapacity - 1U);
		std::memcpy(p_pszDestination, p_pszSource, szLength);
		p_pszDestination[szLength] = '\0';
	}
}

int CompressionFacade_GetAbiInfo(ST_FACADE_ABI_INFO* p_pInfo)
{
	if (p_pInfo == nullptr || p_pInfo->uiStructSize < sizeof(ST_FACADE_ABI_INFO) ||
		p_pInfo->uiAbiVersion != FACADE_ABI_VERSION)
	{
		return FACADE_ABI_MISMATCH;
	}
	std::memset(p_pInfo, 0, sizeof(*p_pInfo));
	p_pInfo->uiStructSize = sizeof(*p_pInfo);
	p_pInfo->uiAbiVersion = FACADE_ABI_VERSION;
	CopyText(p_pInfo->szFacadeName, sizeof(p_pInfo->szFacadeName), "CompressionFacade");
	CopyText(p_pInfo->szImplementationVersion,
		sizeof(p_pInfo->szImplementationVersion), "1.0.0");
	CopyText(p_pInfo->szDependencyVersion,
		sizeof(p_pInfo->szDependencyVersion), "snappy-private-static");
	CopyText(p_pInfo->szBuildId, sizeof(p_pInfo->szBuildId), __DATE__ " " __TIME__);
	return FACADE_SUCCESS;
}

int CompressionFacade_CreateCodec(const ST_COMPRESSION_CODEC_CONFIG* p_pConfig,
	COMPRESSION_CODEC_HANDLE* p_pCodec)
{
	if (p_pCodec == nullptr)
	{
		return FACADE_INVALID_ARGUMENT;
	}
	*p_pCodec = nullptr;
	if (p_pConfig == nullptr ||
		p_pConfig->uiStructSize < sizeof(ST_COMPRESSION_CODEC_CONFIG) ||
		p_pConfig->uiAbiVersion != FACADE_ABI_VERSION ||
		p_pConfig->uiFlags != 0U || !IsZeroReserved(p_pConfig->aullReserved) ||
		p_pConfig->uiReservedAlignment != 0U || p_pConfig->ullMaxInputBytes == 0U ||
		p_pConfig->ullMaxInputBytes > s_ullMaximumInputLimit ||
		p_pConfig->ullMaxOutputBytes == 0U ||
		p_pConfig->ullMaxOutputBytes > s_ullMaximumOutputLimit)
	{
		return FACADE_INVALID_ARGUMENT;
	}
	CCompressionCodecFactory clFactory;
	std::unique_ptr<ICompressionCodec> ptrCodec = clFactory.Create(p_pConfig->enAlgorithm);
	if (!ptrCodec)
	{
		return FACADE_NOT_SUPPORTED;
	}
	try
	{
		*p_pCodec = new CCodecHandle(std::move(ptrCodec),
			p_pConfig->ullMaxInputBytes, p_pConfig->ullMaxOutputBytes);
		return FACADE_SUCCESS;
	}
	catch (...)
	{
		return FACADE_INTERNAL_ERROR;
	}
}

void CompressionFacade_DestroyCodec(COMPRESSION_CODEC_HANDLE p_pCodec)
{
	delete GetHandle(p_pCodec);
}

int CompressionFacade_GetMaxCompressedLength(COMPRESSION_CODEC_HANDLE p_pCodec,
	size_t p_uiInputLength, size_t* p_puiMaximumLength)
{
	CCodecHandle* pHandle = GetHandle(p_pCodec);
	if (pHandle == nullptr || p_puiMaximumLength == nullptr ||
		p_uiInputLength > pHandle->m_ullMaxInputBytes)
	{
		return FACADE_INVALID_ARGUMENT;
	}
	const size_t szMaximum = pHandle->m_ptrCodec->GetMaxCompressedLength(p_uiInputLength);
	if (szMaximum > pHandle->m_ullMaxOutputBytes)
	{
		return FACADE_RESOURCE_EXHAUSTED;
	}
	*p_puiMaximumLength = szMaximum;
	return FACADE_SUCCESS;
}

int CompressionFacade_Compress(COMPRESSION_CODEC_HANDLE p_pCodec,
	const ST_FACADE_BUFFER_VIEW* p_pInput, ST_FACADE_MUTABLE_BUFFER* p_pOutput)
{
	CCodecHandle* pHandle = GetHandle(p_pCodec);
	if (pHandle == nullptr || !IsValidInput(p_pInput) || !IsValidOutput(p_pOutput) ||
		p_pInput->uiLength > pHandle->m_ullMaxInputBytes)
	{
		return FACADE_INVALID_ARGUMENT;
	}
	p_pOutput->uiLength = 0U;
	const size_t szRequired = pHandle->m_ptrCodec->GetMaxCompressedLength(p_pInput->uiLength);
	if (szRequired > pHandle->m_ullMaxOutputBytes)
	{
		return FACADE_RESOURCE_EXHAUSTED;
	}
	if (p_pOutput->uiCapacity < szRequired)
	{
		p_pOutput->uiLength = szRequired;
		return FACADE_BUFFER_TOO_SMALL;
	}
	size_t szActual = p_pOutput->uiCapacity;
	if (!pHandle->m_ptrCodec->Compress(p_pInput->pData, p_pInput->uiLength,
		p_pOutput->pData, &szActual))
	{
		return FACADE_INTERNAL_ERROR;
	}
	p_pOutput->uiLength = szActual;
	return FACADE_SUCCESS;
}

int CompressionFacade_GetUncompressedLength(COMPRESSION_CODEC_HANDLE p_pCodec,
	const ST_FACADE_BUFFER_VIEW* p_pInput, size_t* p_puiUncompressedLength)
{
	CCodecHandle* pHandle = GetHandle(p_pCodec);
	if (pHandle == nullptr || !IsValidInput(p_pInput) ||
		p_puiUncompressedLength == nullptr ||
		p_pInput->uiLength > pHandle->m_ullMaxInputBytes)
	{
		return FACADE_INVALID_ARGUMENT;
	}
	size_t szRequired = 0U;
	if (!pHandle->m_ptrCodec->GetUncompressedLength(p_pInput->pData,
		p_pInput->uiLength, &szRequired))
	{
		return FACADE_PROTOCOL_ERROR;
	}
	if (szRequired > pHandle->m_ullMaxOutputBytes)
	{
		return FACADE_RESOURCE_EXHAUSTED;
	}
	*p_puiUncompressedLength = szRequired;
	return FACADE_SUCCESS;
}

int CompressionFacade_Decompress(COMPRESSION_CODEC_HANDLE p_pCodec,
	const ST_FACADE_BUFFER_VIEW* p_pInput, ST_FACADE_MUTABLE_BUFFER* p_pOutput)
{
	if (!IsValidOutput(p_pOutput))
	{
		return FACADE_INVALID_ARGUMENT;
	}
	p_pOutput->uiLength = 0U;
	size_t szRequired = 0U;
	const int iLengthResult = CompressionFacade_GetUncompressedLength(
		p_pCodec, p_pInput, &szRequired);
	if (iLengthResult != FACADE_SUCCESS)
	{
		return iLengthResult;
	}
	if (p_pOutput->uiCapacity < szRequired)
	{
		p_pOutput->uiLength = szRequired;
		return FACADE_BUFFER_TOO_SMALL;
	}
	CCodecHandle* pHandle = GetHandle(p_pCodec);
	if (!pHandle->m_ptrCodec->Decompress(p_pInput->pData,
		p_pInput->uiLength, p_pOutput->pData))
	{
		return FACADE_PROTOCOL_ERROR;
	}
	p_pOutput->uiLength = szRequired;
	return FACADE_SUCCESS;
}
