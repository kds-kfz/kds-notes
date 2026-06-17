#pragma once

#include <cstddef>
#include <cstring>
#include <vector>

// Ice 3.8 生成的 AByte 使用 std::byte，这里集中处理与 char* 的转换。
namespace bytecompat
{
    template <typename ByteVector>
/* 返回可写 char* 视图，便于复用旧版按字节 memcpy 的代码。 */
    inline char* GetByteData(ByteVector& p_refBytes)
    {
        return reinterpret_cast<char*>(p_refBytes.data());
    }

    template <typename ByteVector>
/* 返回只读 char* 视图，避免各处重复 reinterpret_cast。 */
    inline const char* GetByteData(const ByteVector& p_refBytes)
    {
        return reinterpret_cast<const char*>(p_refBytes.data());
    }

    template <typename ByteVector>
/* 按原始字节复制到 Ice AByte，p_uLen 为 0 时只调整长度不访问 p_pSrc。 */
    inline void assign(ByteVector& p_refOut, const void* p_pSrc, std::size_t p_uLen)
    {
        p_refOut.resize(p_uLen);
        if (p_uLen > 0)
        {
            std::memcpy(p_refOut.data(), p_pSrc, p_uLen);
        }
    }
}
