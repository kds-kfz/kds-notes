#include "pch.h"
#include "MtLoadTest.h"
#include "MainDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
constexpr wchar_t kSingleInstanceMutexName[] = L"Local\\MtLoadTest.MainWindow.SingleInstance.v1"; // 当前用户会话互斥量名称。
constexpr wchar_t kMainWindowCaption[] = L"Mt 测试与数据工具"; // 用于激活旧实例的主窗口标题。

// 查找同类旧窗口并恢复到前台；只用于检测到单实例互斥量已存在的路径。
bool ActivateExistingInstance()
{
    // 首个进程可能刚创建互斥量、尚未完成主窗口创建；重复启动只在此处短暂等待。
    HWND mainWindow = nullptr;
    for (int attempt = 0; attempt < 40 && mainWindow == nullptr; ++attempt)
    {
        mainWindow = ::FindWindowW(nullptr, kMainWindowCaption);
        if (mainWindow == nullptr)
        {
            ::Sleep(50);
        }
    }

    if (mainWindow == nullptr)
    {
        return false;
    }

    // 若主窗口当前打开了压力测试或日志弹窗，优先恢复用户正在操作的窗口。
    HWND targetWindow = ::GetLastActivePopup(mainWindow);
    if (targetWindow == nullptr || !::IsWindow(targetWindow))
    {
        targetWindow = mainWindow;
    }

    if (::IsIconic(targetWindow))
    {
        ::ShowWindowAsync(targetWindow, SW_RESTORE);
    }
    else
    {
        ::ShowWindowAsync(targetWindow, SW_SHOW);
    }

    ::BringWindowToTop(targetWindow);
    if (!::SetForegroundWindow(targetWindow))
    {
        FLASHWINFO flashInfo{};
        flashInfo.cbSize = sizeof(flashInfo);
        flashInfo.hwnd = targetWindow;
        flashInfo.dwFlags = FLASHW_TRAY | FLASHW_TIMERNOFG;
        flashInfo.uCount = 3;
        flashInfo.dwTimeout = 0;
        ::FlashWindowEx(&flashInfo);
    }
    return true;
}
} // namespace

BEGIN_MESSAGE_MAP(CMtLoadTestApp, CWinApp)
END_MESSAGE_MAP()

CMtLoadTestApp theApp;

// 关键实现：先建立当前用户单实例互斥量，再初始化 MFC 并模态运行主窗口。
BOOL CMtLoadTestApp::InitInstance()
{
    m_hSingleInstanceMutex = ::CreateMutexW(nullptr, FALSE, kSingleInstanceMutexName);
    if (m_hSingleInstanceMutex == nullptr)
    {
        AfxMessageBox(L"无法创建 MtLoadTest 单实例标识，程序未启动。", MB_OK | MB_ICONERROR);
        return FALSE;
    }

    if (::GetLastError() == ERROR_ALREADY_EXISTS)
    {
        ::CloseHandle(m_hSingleInstanceMutex);
        m_hSingleInstanceMutex = nullptr;
        if (!ActivateExistingInstance())
        {
            AfxMessageBox(L"MtLoadTest 已在运行，但暂时无法定位现有窗口。", MB_OK | MB_ICONINFORMATION);
        }
        return FALSE;
    }

    INITCOMMONCONTROLSEX controls{};
    controls.dwSize = sizeof(controls);
    controls.dwICC = ICC_WIN95_CLASSES | ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&controls);
    CWinApp::InitInstance();
    AfxEnableControlContainer();
    SetRegistryKey(L"MtLoadTest");

    CMainDlg dialog;
    m_pMainWnd = &dialog;
    dialog.DoModal();
    return FALSE;
}

// 关键实现：在 MFC 公共清理前释放互斥量，保证进程退出后可以再次启动。
int CMtLoadTestApp::ExitInstance()
{
    if (m_hSingleInstanceMutex != nullptr)
    {
        ::CloseHandle(m_hSingleInstanceMutex);
        m_hSingleInstanceMutex = nullptr;
    }
    return CWinApp::ExitInstance();
}
