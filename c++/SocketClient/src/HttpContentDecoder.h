#ifndef SOCKET_HTTP_CONTENT_DECODER_H
#define SOCKET_HTTP_CONTENT_DECODER_H

#include <cstddef>
#include <string>
#include <vector>

// HTTP 正文解码器集中处理 Content-Encoding，避免压测和接口测试各自实现解压逻辑。
namespace HttpContentDecoder
{
    // 解码后的正文设置独立上限，防止小体积压缩包异常膨胀占满进程内存。
    const size_t kMaxDecodedBodyBytes = 256U * 1024U * 1024U;

    // 将 gzip、deflate/zlib 正文解压为原始业务报文；identity 或空编码直接复制。
    bool Decode(const std::string& p_strContentEncoding,
        const std::vector<unsigned char>& p_vecEncoded,
        std::vector<unsigned char>& p_refDecoded, std::string& p_refError);
}

#endif
