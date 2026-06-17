#pragma once

#include <Ice/Ice.h>

// Ice 3.8 使用 shared_ptr 管理 servant，这里包装旧对象但不接管删除权。
namespace icecompat
{
    template <typename T>
/* 创建非拥有型 ObjectPtr，避免 m_refAdapter 释放由 DLL 句柄生命周期管理的对象。 */
    Ice::ObjectPtr MakeServantPtr(T* p_pServant)
    {
        return Ice::ObjectPtr(p_pServant, [](Ice::Object*) {});
    }
}
