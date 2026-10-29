#pragma once

#include <cstdint>
#include <ctime>

namespace icerpcfoundation
{
	// 返回本地日期 YYYYMMDD；FoundationStatic 不可用时返回 0。
	std::uint32_t GetCurrentDate();

	// 确保 UTF-8 目录存在；目录已存在也返回 true。
	bool EnsureDirectory(const char* p_pszPath);

	// 让当前日志线程休眠指定毫秒，不持有业务锁。
	void SleepMs(std::uint32_t p_uiMilliseconds);

	// 线程安全地把 time_t 转换为本地时间；失败时返回全零结构。
	std::tm SafeLocalTime(std::time_t p_ttTime);
}
