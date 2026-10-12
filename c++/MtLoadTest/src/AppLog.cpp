#include "pch.h"
#include "AppLog.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace
{
constexpr size_t kMaxMemoryEntries = 1000;

// 把 SYSTEMTIME 格式化为固定宽度本地时间，便于日志按文本排序。
std::wstring FormatTimestamp(const SYSTEMTIME& value)
{
    wchar_t text[32]{};
    swprintf_s(text, L"%04u-%02u-%02u %02u:%02u:%02u.%03u",
        value.wYear, value.wMonth, value.wDay,
        value.wHour, value.wMinute, value.wSecond, value.wMilliseconds);
    return text;
}
}

// 关键实现：创建日志目录并按当天日期选择文件，初始化失败不留下半路径状态。
bool CAppLog::Initialize(const std::wstring& executableDirectory, std::wstring& error)
{
    SYSTEMTIME now{};
    GetLocalTime(&now);
    wchar_t fileName[64]{};
    swprintf_s(fileName, L"MtLoadTest_%04u%02u%02u.log", now.wYear, now.wMonth, now.wDay);

    std::error_code createError;
    const std::filesystem::path directory = std::filesystem::path(executableDirectory) / L"log";
    std::filesystem::create_directories(directory, createError);
    if (createError)
    {
        error = L"无法创建日志目录: " + directory.wstring();
        return false;
    }

    const std::filesystem::path path = directory / fileName;
    std::ofstream probe(path, std::ios::binary | std::ios::app);
    if (!probe)
    {
        error = L"无法打开日志文件: " + path.wstring();
        return false;
    }

    std::lock_guard<std::mutex> guard(mutex_);
    filePath_ = path.wstring();
    return true;
}

// 关键实现：在同一锁内分配序号和写入内存，磁盘追加失败不影响界面继续运行。
void CAppLog::Add(const wchar_t* level, const std::wstring& message)
{
    SYSTEMTIME now{};
    GetLocalTime(&now);
    AppLogEntry entry;
    entry.timestamp = FormatTimestamp(now);
    entry.level = level == nullptr ? L"INFO" : level;
    entry.message = message;
    for (wchar_t& character : entry.message)
    {
        if (character == L'\r' || character == L'\n') character = L' ';
    }

    std::lock_guard<std::mutex> guard(mutex_);
    entry.sequence = nextSequence_++;
    entries_.push_back(entry);
    if (entries_.size() > kMaxMemoryEntries)
        entries_.erase(entries_.begin(), entries_.begin() + (entries_.size() - kMaxMemoryEntries));

    if (filePath_.empty()) return;
    const std::wstring line = L"[" + entry.timestamp + L"][" + entry.level + L"] " + entry.message + L"\r\n";
    std::ofstream output(std::filesystem::path(filePath_), std::ios::binary | std::ios::app);
    if (output)
    {
        const std::string utf8 = ToUtf8(line);
        output.write(utf8.data(), static_cast<std::streamsize>(utf8.size()));
    }
}

// 关键实现：在锁内复制全部条目，调用方随后可无锁遍历。
std::vector<AppLogEntry> CAppLog::Snapshot() const
{
    std::lock_guard<std::mutex> guard(mutex_);
    return entries_;
}

// 关键实现：返回路径副本，避免调用方持有受锁保护字符串的引用。
std::wstring CAppLog::FilePath() const
{
    std::lock_guard<std::mutex> guard(mutex_);
    return filePath_;
}

// 关键实现：只清空内存条目并保留递增序号和磁盘历史。
void CAppLog::ClearMemory()
{
    std::lock_guard<std::mutex> guard(mutex_);
    entries_.clear();
}

// 关键实现：按两次转换取得精确 UTF-8 容量，避免固定缓冲区截断中文。
std::string CAppLog::ToUtf8(const std::wstring& value)
{
    if (value.empty()) return {};
    const int size = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
        nullptr, 0, nullptr, nullptr);
    std::string result(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
        result.data(), size, nullptr, nullptr);
    return result;
}
