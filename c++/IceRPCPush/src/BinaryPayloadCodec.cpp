#include "BinaryPayloadCodec.h"

#include <cstring>
#include <snappy.h>

namespace binarypayload
{
	bool Encode(const unsigned char* p_pBuffer, int p_iLength,
		int p_iProtocolVersion, int p_iCompressionThresholdBytes,
		int p_iMaxPayloadBytes, ::JSONBINRPC::BinaryPayload& p_refPayload,
		std::string& p_refError)
	{
		p_refPayload.version = p_iProtocolVersion;
		p_refPayload.compression =
			::JSONBINRPC::BinaryCompression::BinaryCompressionNone;
		p_refPayload.rawSize = 0;
		p_refPayload.data.clear();
		p_refError.clear();
		if (p_iLength < 0 || (p_iLength > 0 && p_pBuffer == nullptr))
		{
			p_refError = "BINARY_PAYLOAD_LENGTH_INVALID: input pointer or length is invalid";
			return false;
		}
		if (p_iMaxPayloadBytes <= 0 || p_iLength > p_iMaxPayloadBytes)
		{
			p_refError = "BINARY_PAYLOAD_TOO_LARGE: payload exceeds BinaryMaxPayloadBytes";
			return false;
		}
		p_refPayload.rawSize = p_iLength;
		if (p_iLength == 0)
		{
			return true;
		}

		if (p_iLength >= p_iCompressionThresholdBytes)
		{
			std::string strCompressed;
			snappy::Compress(reinterpret_cast<const char*>(p_pBuffer),
				static_cast<std::size_t>(p_iLength), &strCompressed);
			if (!strCompressed.empty() && strCompressed.size() <
				static_cast<std::size_t>(p_iLength))
			{
				p_refPayload.compression =
					::JSONBINRPC::BinaryCompression::BinaryCompressionSnappy;
				p_refPayload.data.resize(strCompressed.size());
				std::memcpy(p_refPayload.data.data(), strCompressed.data(),
					strCompressed.size());
				return true;
			}
		}

		p_refPayload.data.resize(static_cast<std::size_t>(p_iLength));
		std::memcpy(p_refPayload.data.data(), p_pBuffer,
			static_cast<std::size_t>(p_iLength));
		return true;
	}

	bool Decode(const ::JSONBINRPC::BinaryPayload& p_refPayload,
		int p_iProtocolVersion, int p_iMaxPayloadBytes,
		std::vector<unsigned char>& p_refBuffer, std::string& p_refError)
	{
		return DecodeView(p_refPayload.data.empty() ? nullptr :
			reinterpret_cast<const unsigned char*>(p_refPayload.data.data()),
			static_cast<int>(p_refPayload.data.size()), p_refPayload.version,
			static_cast<int>(p_refPayload.compression), p_refPayload.rawSize,
			p_iProtocolVersion, p_iMaxPayloadBytes, p_refBuffer, p_refError);
	}

	bool DecodeView(const unsigned char* p_pWireBuffer, int p_iWireLength,
		int p_iPayloadVersion, int p_iCompression, int p_iRawSize,
		int p_iProtocolVersion, int p_iMaxPayloadBytes,
		std::vector<unsigned char>& p_refBuffer, std::string& p_refError)
	{
		p_refBuffer.clear();
		p_refError.clear();
		if (p_iPayloadVersion != p_iProtocolVersion)
		{
			p_refError = "BINARY_PROTOCOL_VERSION_UNSUPPORTED: payload version does not match local version";
			return false;
		}
		if (p_iWireLength < 0 || (p_iWireLength > 0 && p_pWireBuffer == nullptr))
		{
			p_refError = "BINARY_PAYLOAD_LENGTH_INVALID: wire pointer or length is invalid";
			return false;
		}
		if (p_iRawSize < 0 || p_iMaxPayloadBytes <= 0 ||
			p_iRawSize > p_iMaxPayloadBytes)
		{
			p_refError = "BINARY_PAYLOAD_TOO_LARGE: raw payload length is invalid";
			return false;
		}
		if (p_iRawSize == 0)
		{
			if (p_iWireLength != 0)
			{
				p_refError = "BINARY_PAYLOAD_LENGTH_INVALID: empty payload contains wire data";
				return false;
			}
			return true;
		}
		if (p_iCompression == static_cast<int>(
			::JSONBINRPC::BinaryCompression::BinaryCompressionNone))
		{
			if (p_iWireLength != p_iRawSize)
			{
				p_refError = "BINARY_PAYLOAD_LENGTH_INVALID: raw size does not match wire size";
				return false;
			}
			p_refBuffer.resize(static_cast<std::size_t>(p_iWireLength));
			std::memcpy(p_refBuffer.data(), p_pWireBuffer,
				static_cast<std::size_t>(p_iWireLength));
			return true;
		}
		if (p_iCompression != static_cast<int>(
			::JSONBINRPC::BinaryCompression::BinaryCompressionSnappy) ||
			p_iWireLength == 0)
		{
			p_refError = "BINARY_COMPRESSION_UNSUPPORTED: compression type is invalid";
			return false;
		}

		std::size_t szDecodedLength = 0;
		if (!snappy::GetUncompressedLength(
			reinterpret_cast<const char*>(p_pWireBuffer),
			static_cast<std::size_t>(p_iWireLength), &szDecodedLength) ||
			szDecodedLength != static_cast<std::size_t>(p_iRawSize))
		{
			p_refError = "BINARY_SNAPPY_DECOMPRESS_FAILED: decompressed length does not match raw size";
			return false;
		}
		p_refBuffer.resize(szDecodedLength);
		if (!snappy::RawUncompress(
			reinterpret_cast<const char*>(p_pWireBuffer),
			static_cast<std::size_t>(p_iWireLength),
			reinterpret_cast<char*>(p_refBuffer.data())))
		{
			p_refBuffer.clear();
			p_refError = "BINARY_SNAPPY_DECOMPRESS_FAILED: compressed payload is invalid";
			return false;
		}
		return true;
	}
}
