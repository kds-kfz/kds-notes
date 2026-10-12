#pragma once

// MFC 应用对象只负责创建主对话框；压测生命周期由 CLoadEngineClient 管理。

#ifndef __AFXWIN_H__
#error "请先包含 pch.h"
#endif

#include "resource.h"

class CMtLoadTestApp final : public CWinApp
{
public:
    // 初始化公共控件、单实例互斥量和主对话框；失败时返回 FALSE 终止进程。
    BOOL InitInstance() override;
    // 释放单实例句柄后交回 MFC 完成线程级清理。
    int ExitInstance() override;
    DECLARE_MESSAGE_MAP()

private:
    // 当前用户会话内的单实例互斥量，由应用对象持有至退出。
    HANDLE singleInstanceMutex_ = nullptr; // 当前用户会话内唯一互斥量句柄，由 ExitInstance 关闭。
};

extern CMtLoadTestApp theApp; // MFC 框架创建的进程级应用单例。
