#include "SnappyCompat.h"

#include "CompressionFacade.h"

#include <cstdint>

namespace
{
	// 压缩句柄由 IceRPCPush 所在最终 DLL 独占，函数静态对象保证并发首次访问只初始化一次。
	class CCompressionRuntime final
	{
	public:
		CCompressionRuntime()
			: m_hCodec(nullptr)
		{
			ST_COMPRESSION_CODEC_CONFIG stConfig = {};
			stConfig.uiStructSize = sizeof(stConfig);
			stConfig.uiAbiVersion = FACADE_ABI_VERSION;
			stConfig.enAlgorithm = COMPRESSION_ALGORITHM_SNAPPY;
			stConfig.ullMaxInputBytes = 1024ULL * 1024ULL * 1024ULL;
			stConfig.ullMaxOutputBytes = 4ULL * 1024ULL * 1024ULL * 1024ULL;
			if (CompressionFacade_CreateCodec(&stConfig, &m_hCodec) != FACADE_SUCCESS)
			{
				m_hCodec = nullptr;
			}
		}

		~CCompressionRuntime()
		{
			CompressionFacade_DestroyCodec(m_hCodec);
			m_hCodec = nullptr;
		}

		COMPRESSION_CODEC_HANDLE GetCodec() const
		{
			return m_hCodec;
		}

	private:
		COMPRESSION_CODEC_HANDLE m_hCodec; // CompressionStatic 创建的只读 Codec，随最终 DLL 卸载释放。
	};

	CCompressionRuntime& GetCompressionRuntime()
	{
		static CCompressionRuntime s_clRuntime;
		return s_clRuntime;
	}

	ST_FACADE_BUFFER_VIEW MakeInputView(const char* p_pInput,
		std::size_t p_uiInputLength)
	{
		ST_FACADE_BUFFER_VIEW stInput = {};
		stInput.uiStructSize = sizeof(stInput);
		stInput.uiAbiVersion = FACADE_ABI_VERSION;
		stInput.pData = reinterpret_cast<const uint8_t*>(p_pInput);
		stInput.uiLength = p_uiInputLength;
		return stInput;
	}

	ST_FACADE_MUTABLE_BUFFER MakeOutputView(char* p_pOutput,
		std::size_t p_uiCapacity)
	{
		ST_FACADE_MUTABLE_BUFFER stOutput = {};
		stOutput.uiStructSize = sizeof(stOutput);
		stOutput.uiAbiVersion = FACADE_ABI_VERSION;
		stOutput.pData = reinterpret_cast<uint8_t*>(p_pOutput);
		stOutput.uiCapacity = p_uiCapacity;
		return stOutput;
	}
}

std::size_t icerpccompression::MaxCompressedLength(std::size_t p_uiInputLength)
{
	std::size_t uiMaximumLength = 0U;
	const COMPRESSION_CODEC_HANDLE hCodec = GetCompressionRuntime().GetCodec();
	if (hCodec == nullptr || CompressionFacade_GetMaxCompressedLength(
		hCodec, p_uiInputLength, &uiMaximumLength) != FACADE_SUCCESS)
	{
		return 0U;
	}
	return uiMaximumLength;
}

std::size_t icerpccompression::Compress(const char* p_pInput,
	std::size_t p_uiInputLength, std::string* p_pOutput)
{
	if (p_pOutput == nullptr || (p_pInput == nullptr && p_uiInputLength != 0U))
	{
		return 0U;
	}
	p_pOutput->clear();
	const std::size_t uiMaximumLength = MaxCompressedLength(p_uiInputLength);
	if (uiMaximumLength == 0U)
	{
		return 0U;
	}
	p_pOutput->resize(uiMaximumLength);
	const ST_FACADE_BUFFER_VIEW stInput = MakeInputView(p_pInput, p_uiInputLength);
	ST_FACADE_MUTABLE_BUFFER stOutput = MakeOutputView(
		p_pOutput->data(), p_pOutput->size());
	const int iResult = CompressionFacade_Compress(
		GetCompressionRuntime().GetCodec(), &stInput, &stOutput);
	if (iResult != FACADE_SUCCESS)
	{
		p_pOutput->clear();
		return 0U;
	}
	p_pOutput->resize(stOutput.uiLength);
	return stOutput.uiLength;
}

void icerpccompression::RawCompress(const char* p_pInput,
	std::size_t p_uiInputLength, char* p_pOutput,
	std::size_t* p_puiOutputLength)
{
	if (p_puiOutputLength == nullptr ||
		(p_pInput == nullptr && p_uiInputLength != 0U) || p_pOutput == nullptr)
	{
		if (p_puiOutputLength != nullptr)
		{
			*p_puiOutputLength = 0U;
		}
		return;
	}
	const ST_FACADE_BUFFER_VIEW stInput = MakeInputView(p_pInput, p_uiInputLength);
	ST_FACADE_MUTABLE_BUFFER stOutput = MakeOutputView(
		p_pOutput, *p_puiOutputLength);
	if (CompressionFacade_Compress(GetCompressionRuntime().GetCodec(),
		&stInput, &stOutput) != FACADE_SUCCESS)
	{
		*p_puiOutputLength = 0U;
		return;
	}
	*p_puiOutputLength = stOutput.uiLength;
}

bool icerpccompression::GetUncompressedLength(const char* p_pInput,
	std::size_t p_uiInputLength, std::size_t* p_puiOutputLength)
{
	if (p_puiOutputLength == nullptr ||
		(p_pInput == nullptr && p_uiInputLength != 0U))
	{
		return false;
	}
	*p_puiOutputLength = 0U;
	const ST_FACADE_BUFFER_VIEW stInput = MakeInputView(p_pInput, p_uiInputLength);
	return CompressionFacade_GetUncompressedLength(
		GetCompressionRuntime().GetCodec(), &stInput,
		p_puiOutputLength) == FACADE_SUCCESS;
}

bool icerpccompression::Uncompress(const char* p_pInput,
	std::size_t p_uiInputLength, std::string* p_pOutput)
{
	if (p_pOutput == nullptr)
	{
		return false;
	}
	p_pOutput->clear();
	std::size_t uiOutputLength = 0U;
	if (!GetUncompressedLength(p_pInput, p_uiInputLength, &uiOutputLength))
	{
		return false;
	}
	p_pOutput->resize(uiOutputLength);
	const ST_FACADE_BUFFER_VIEW stInput = MakeInputView(p_pInput, p_uiInputLength);
	ST_FACADE_MUTABLE_BUFFER stOutput = MakeOutputView(
		p_pOutput->data(), p_pOutput->size());
	if (CompressionFacade_Decompress(GetCompressionRuntime().GetCodec(),
		&stInput, &stOutput) != FACADE_SUCCESS)
	{
		p_pOutput->clear();
		return false;
	}
	p_pOutput->resize(stOutput.uiLength);
	return true;
}

bool icerpccompression::RawUncompress(const char* p_pInput,
	std::size_t p_uiInputLength, char* p_pOutput)
{
	std::size_t uiOutputLength = 0U;
	if (p_pOutput == nullptr ||
		!GetUncompressedLength(p_pInput, p_uiInputLength, &uiOutputLength))
	{
		return false;
	}
	const ST_FACADE_BUFFER_VIEW stInput = MakeInputView(p_pInput, p_uiInputLength);
	ST_FACADE_MUTABLE_BUFFER stOutput = MakeOutputView(p_pOutput, uiOutputLength);
	return CompressionFacade_Decompress(GetCompressionRuntime().GetCodec(),
		&stInput, &stOutput) == FACADE_SUCCESS;
}
