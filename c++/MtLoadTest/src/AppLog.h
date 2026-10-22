#pragma once

// MtLoadTest 本地运行日志存储。
// 本类只保存界面操作、压测状态和 DLL 返回的脱敏诊断，不接触请求密码、Token 或报文正文。

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

struct AppLogEntry
{
    uint64_t sequence = 0;      // 进程内单调递增序号，用于增量刷新。
    std::wstring timestamp;     // 本地时间，精确到毫秒。
    std::wstring level;         // INFO/WARN/ERROR 等日志等级。
    std::wstring message;       // 已脱敏的人类可读消息。
};

// 线程安全的低频应用日志。
// 本类拥有内存条目和日志文件路径，但不拥有压测引擎或界面控件。
class CAppLog final
{
public:
    // 在程序目录下创建 log 子目录，并按日期生成 UTF-8 日志文件。
    bool Initialize(const std::wstring& executableDirectory, std::wstring& error);

    // 追加一条低频运行日志。线程安全；磁盘写入失败不会中断压测。
    void Add(const wchar_t* level, const std::wstring& message);

    // 返回不可变副本，供日志窗口定时刷新。
    std::vector<AppLogEntry> Snapshot() const;
    // 返回当前日志文件绝对路径的副本，供“打开日志”操作使用。
    std::wstring FilePath() const;

    // 只清空界面内存，不截断已经写入的日志文件。
    void ClearMemory();

private:
    // 将 Windows 宽字符串转换为写盘使用的 UTF-8；转换失败返回空字符串。
    static std::string ToUtf8(const std::wstring& value);

    mutable std::mutex m_clMutex;              // 保护条目、路径和序号。
    std::vector<AppLogEntry> m_vecEntries;      // 当前进程内可供界面查看的日志副本。
    std::wstring m_wstrFilePath;                 // 当前日期日志文件绝对路径。
    uint64_t m_u64NextSequence = 1;             // 下一条日志的进程内序号。
};
