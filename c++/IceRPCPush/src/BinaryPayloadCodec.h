#pragma once

#include "Corba/JSONBINRPCU.h"

#include <string>
#include <vector>

// BinaryPayload 逐包编解码器，只负责版本、长度和 Snappy 运输，不解释业务正文。
namespace binarypayload
{
	// 达到阈值且压缩结果更小时使用 Snappy，否则保留原始二进制。
	bool Encode(const unsigned char* p_pBuffer, int p_iLength,
		int p_iProtocolVersion, int p_iCompressionThresholdBytes,
		int p_iMaxPayloadBytes, ::JSONBINRPC::BinaryPayload& p_refPayload,
		std::string& p_refError);

	// 校验版本、压缩类型、原始长度和解压结果后返回原始二进制。
	bool Decode(const ::JSONBINRPC::BinaryPayload& p_refPayload,
		int p_iProtocolVersion, int p_iMaxPayloadBytes,
		std::vector<unsigned char>& p_refBuffer, std::string& p_refError);

	// 解码不拥有内存的 BinaryPayload 视图，供最终消费 Worker 延迟解压。
	bool DecodeView(const unsigned char* p_pWireBuffer, int p_iWireLength,
		int p_iPayloadVersion, int p_iCompression, int p_iRawSize,
		int p_iProtocolVersion, int p_iMaxPayloadBytes,
		std::vector<unsigned char>& p_refBuffer, std::string& p_refError);
}
