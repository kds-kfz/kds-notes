#ifndef _FUBLICFUNC_H_
#define _FUBLICFUNC_H_

#include <cstddef>
#include <cstring>
#include <ctime>
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 8
#include <experimental/filesystem>
namespace socket_server_filesystem = std::experimental::filesystem;
#else
#include <filesystem>
namespace socket_server_filesystem = std::filesystem;
#endif
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <iostream>

// SocketServer 的私有路径缓冲区上限。保持原有 260 字节约束，避免日志文件名行为变化。
constexpr std::size_t SOCKET_SERVER_MAX_PATH = 260U;

// 将 C 字符串复制到调用方缓冲区。输入只在调用期间借用；输出始终以 NUL 结尾。
// 目标无效时不写入；源为空时输出空字符串。函数不使用共享状态，可并发调用。
inline void SocketServerSafeCopyCString(char* p_szTarget,
	std::size_t p_szTargetCapacity, const char* p_szSource) noexcept
{
	if (p_szTarget == nullptr || p_szTargetCapacity == 0U)
	{
		return;
	}

	p_szTarget[0] = '\0';
	if (p_szSource == nullptr)
	{
		return;
	}

	const std::size_t szSourceLength = std::strlen(p_szSource);
	const std::size_t szCopyLength =
		szSourceLength < p_szTargetCapacity ? szSourceLength : p_szTargetCapacity - 1U;
	if (szCopyLength > 0U)
	{
		std::memcpy(p_szTarget, p_szSource, szCopyLength);
	}
	p_szTarget[szCopyLength] = '\0';
}

// 检查路径是否为已存在目录。返回 0 表示存在，-1 表示参数无效，-2 表示不存在或不可访问。
inline int SocketServerFolderExists(const char* p_szFolderPath) noexcept
{
	if (p_szFolderPath == nullptr || p_szFolderPath[0] == '\0')
	{
		return -1;
	}

	std::error_code clError;
	return socket_server_filesystem::is_directory(p_szFolderPath, clError) && !clError ? 0 : -2;
}

// 递归创建日志目录。返回 0 表示目录已存在或创建成功，负数表示参数或文件系统错误。
inline int SocketServerCreateFolder(const char* p_szFolderPath) noexcept
{
	if (p_szFolderPath == nullptr || p_szFolderPath[0] == '\0')
	{
		return -1;
	}

	std::error_code clError;
	if (socket_server_filesystem::is_directory(p_szFolderPath, clError) && !clError)
	{
		return 0;
	}
	clError.clear();
	return socket_server_filesystem::create_directories(p_szFolderPath, clError) && !clError ? 0 : -2;
}

// 返回本地日期 YYYYMMDD。失败返回 0，由日志初始化按既有失败路径处理。
inline unsigned int SocketServerCurrentDate() noexcept
{
	const std::time_t tNow = std::time(nullptr);
	std::tm stLocalTime = {};
#if defined(_WIN32)
	if (localtime_s(&stLocalTime, &tNow) != 0)
#else
	if (localtime_r(&tNow, &stLocalTime) == nullptr)
#endif
	{
		return 0U;
	}

	return static_cast<unsigned int>((stLocalTime.tm_year + 1900) * 10000
		+ (stLocalTime.tm_mon + 1) * 100 + stLocalTime.tm_mday);
}

// 将时间戳转换为线程安全的本地时间副本。转换失败返回全零结构，不暴露 CRT 静态缓冲区。
inline std::tm SocketServerLocalTime(std::time_t p_tValue) noexcept
{
	std::tm stLocalTime = {};
#if defined(_WIN32)
	(void)localtime_s(&stLocalTime, &p_tValue);
#else
	(void)localtime_r(&p_tValue, &stLocalTime);
#endif
	return stLocalTime;
}

// 宏定义
#define foreach(container, it) \
	for( decltype((container).begin()) it = (container).begin(); it != (container).end(); ++it)

//防止重名 reverse
#define foreach_reverse(container, it) \
	for( decltype((container).rbegin()) it = (container).rbegin(); it != (container).rend(); ++it)

#endif

