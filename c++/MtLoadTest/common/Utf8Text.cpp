#include "pch.h"
#include "common/Utf8Text.h"

#include <vector>

namespace mttool
{
// 关键实现复制自 V3 common/v2compat/mt/publicfunc.h：先查询准确容量，
// 再使用 MB_ERR_INVALID_CHARS 拒绝损坏的 UTF-8，避免凭据和中文静默替换。
std::wstring Utf8ToWide(const std::string& text)
{
    if (text.empty())
        return {};

    const int size = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
        text.c_str(), -1, nullptr, 0);
    if (size <= 1)
        return {};

    std::vector<wchar_t> buffer(static_cast<size_t>(size));
    if (::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
        text.c_str(), -1, buffer.data(), size) <= 0)
        return {};
    return std::wstring(buffer.data());
}

// 关键实现复制自 V3 common/v2compat/mt/publicfunc.h：转换包含结尾零字符，
// 返回值只保留正文，所有跨层字符串因而保持标准 UTF-8 std::string。
std::string WideToUtf8(const std::wstring& text)
{
    if (text.empty())
        return {};

    const int size = ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
        text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 1)
        return {};

    std::vector<char> buffer(static_cast<size_t>(size));
    if (::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
        text.c_str(), -1, buffer.data(), size, nullptr, nullptr) <= 0)
        return {};
    return std::string(buffer.data());
}
} // namespace mttool
