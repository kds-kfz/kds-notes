#include "HttpContentDecoder.h"

#include <zlib.h>

#include <algorithm>
#include <cctype>
#include <limits>

namespace HttpContentDecoder
{
namespace
{
// HTTP 编码名称只允许 ASCII，统一折叠大小写并去掉首尾空白。
std::string NormalizeEncoding(const std::string& p_strEncoding)
{
    size_t first = 0;
    while (first < p_strEncoding.size() &&
        std::isspace(static_cast<unsigned char>(p_strEncoding[first]))) ++first;
    size_t last = p_strEncoding.size();
    while (last > first &&
        std::isspace(static_cast<unsigned char>(p_strEncoding[last - 1]))) --last;
    std::string result = p_strEncoding.substr(first, last - first);
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char value) {
        return static_cast<char>(std::tolower(value));
    });
    return result;
}

// windowBits 决定 zlib 按 gzip 包装或 zlib 包装解释输入数据。
bool Inflate(const std::vector<unsigned char>& p_vecEncoded, int p_iWindowBits,
    std::vector<unsigned char>& p_refDecoded, std::string& p_refError)
{
    if (p_vecEncoded.size() > static_cast<size_t>((std::numeric_limits<uInt>::max)()))
    {
        p_refError = "压缩响应超过 zlib 单次输入上限";
        return false;
    }

    z_stream stream = {};
    stream.next_in = const_cast<Bytef*>(p_vecEncoded.data());
    stream.avail_in = static_cast<uInt>(p_vecEncoded.size());
    const int initialized = inflateInit2(&stream, p_iWindowBits);
    if (initialized != Z_OK)
    {
        p_refError = "初始化 zlib 解压器失败，错误码=" + std::to_string(initialized);
        return false;
    }

    p_refDecoded.clear();
    unsigned char buffer[32U * 1024U];
    int result = Z_OK;
    while (result == Z_OK)
    {
        stream.next_out = buffer;
        stream.avail_out = static_cast<uInt>(sizeof(buffer));
        result = inflate(&stream, Z_NO_FLUSH);
        const size_t produced = sizeof(buffer) - stream.avail_out;
        if (produced > kMaxDecodedBodyBytes - p_refDecoded.size())
        {
            inflateEnd(&stream);
            p_refDecoded.clear();
            p_refError = "解压后的 HTTP 正文超过 256 MB 安全上限";
            return false;
        }
        p_refDecoded.insert(p_refDecoded.end(), buffer, buffer + produced);
    }
    inflateEnd(&stream);
    if (result != Z_STREAM_END)
    {
        p_refDecoded.clear();
        p_refError = "HTTP 正文解压失败，zlib 错误码=" + std::to_string(result);
        return false;
    }
    return true;
}
}

bool Decode(const std::string& p_strContentEncoding,
    const std::vector<unsigned char>& p_vecEncoded,
    std::vector<unsigned char>& p_refDecoded, std::string& p_refError)
{
    const std::string encoding = NormalizeEncoding(p_strContentEncoding);
    p_refError.clear();
    if (encoding.empty() || encoding == "identity")
    {
        p_refDecoded = p_vecEncoded;
        return true;
    }
    if (encoding == "gzip" || encoding == "x-gzip")
        return Inflate(p_vecEncoded, MAX_WBITS + 16, p_refDecoded, p_refError);
    if (encoding == "deflate" || encoding == "zlib")
    {
        if (Inflate(p_vecEncoded, MAX_WBITS, p_refDecoded, p_refError)) return true;
        // 少量旧服务把 raw deflate 错标为 deflate；兼容该历史行为，但 zlib 包装始终优先。
        return Inflate(p_vecEncoded, -MAX_WBITS, p_refDecoded, p_refError);
    }
    p_refError = "不支持的 HTTP Content-Encoding: " + encoding;
    return false;
}
}
