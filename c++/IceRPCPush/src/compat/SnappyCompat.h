#pragma once

#include <cstddef>
#include <string>

// IceRPCPush 只依赖这些历史 Snappy 语义，具体算法统一由 CompressionStatic 私有实现。
namespace icerpccompression
{
	// 返回输入数据可能需要的最大压缩空间；失败时返回 0。
	std::size_t MaxCompressedLength(std::size_t p_uiInputLength);

	// 把完整输入压缩到字符串；返回实际压缩字节数，失败时清空输出并返回 0。
	std::size_t Compress(const char* p_pInput, std::size_t p_uiInputLength,
		std::string* p_pOutput);

	// 把完整输入压缩到调用方缓冲区；失败时把输出长度设置为 0。
	void RawCompress(const char* p_pInput, std::size_t p_uiInputLength,
		char* p_pOutput, std::size_t* p_puiOutputLength);

	// 只读取压缩包声明的原始长度，不写入输出缓冲区。
	bool GetUncompressedLength(const char* p_pInput, std::size_t p_uiInputLength,
		std::size_t* p_puiOutputLength);

	// 解压到字符串；失败时清空输出，禁止保留半成品。
	bool Uncompress(const char* p_pInput, std::size_t p_uiInputLength,
		std::string* p_pOutput);

	// 解压到已按原始长度分配的调用方缓冲区。
	bool RawUncompress(const char* p_pInput, std::size_t p_uiInputLength,
		char* p_pOutput);
}

// 保留旧源码中的 snappy:: 调用形式，但不再直接包含或暴露第三方 Snappy 头文件。
namespace snappy = icerpccompression;
