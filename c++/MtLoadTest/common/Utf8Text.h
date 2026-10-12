#pragma once

// Windows 平台文本边界适配。
// 业务、配置和协议层统一保存 UTF-8 std::string；仅在 MFC 和 Win32
// 界面/文件 API 边界使用 UTF-16，并在进入或离开平台 API 时立即转换。

#include <string>

namespace mttool
{
// 把严格合法的 UTF-8 转换为 Windows UTF-16；非法输入返回空字符串。
std::wstring Utf8ToWide(const std::string& text);

// 把 Windows UTF-16 转换为严格 UTF-8；非法输入返回空字符串。
std::string WideToUtf8(const std::wstring& text);
} // namespace mttool
