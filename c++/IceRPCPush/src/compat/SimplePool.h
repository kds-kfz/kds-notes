#pragma once

#include <cstddef>
#include <new>

// 兼容旧内存池接口的简单实现，按固定块大小直接向 CRT 申请内存。
class CSimplePool
{
public:
/* p_uBlockSize 是每次 malloc 返回块的大小，调用方按旧协议控制上限。 */
    explicit CSimplePool(std::size_t p_uBlockSize)
        : m_uBlockSize(p_uBlockSize)
    {
    }

/* 返回一个固定大小块；失败返回 nullptr，调用方需按旧逻辑判空。 */
    void* Malloc()
    {
        return ::operator new(m_uBlockSize, std::nothrow);
    }

/* 释放由本池 malloc 返回的块，允许传入 nullptr。 */
    void Free(void* p_pBuffer)
    {
        ::operator delete(p_pBuffer);
    }

    std::size_t GetMaxSize() const
    {
        return m_uBlockSize;
    }

private:
// 单块字节数，生命周期与池对象一致。
    std::size_t m_uBlockSize;
};
