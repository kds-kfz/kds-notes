#pragma once

#include "targetver.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <WS2tcpip.h>
#include <mstcpip.h>
#include <windows.h>
#include <intrin.h>
#include <assert.h>

#include <Ice/Ice.h>
#include <IceGrid/IceGrid.h>

#include <algorithm>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <time.h>
#include <vector>

#include "Corba/JSONBINRPCU.h"
#include "compat/IceCompat.h"
#include "compat/ByteCompat.h"
#include "compat/IceServantCompat.h"
#include "compat/SnappyCompat.h"
#include "compat/SimplePool.h"
#include "zlib.h"

#define __IJSONRPC_DLL

#ifndef ASSERT
#define ASSERT(p_bExpression) assert(p_bExpression)
#endif

// 将 UTF-8 文本转成本地 ANSI 编码，主要用于 Ice 异常信息写入旧日志查看工具。
const char* UTF82ASC(const char* p_szSrcBuf, char* p_szOutBuf, int p_iOutLen);

// Sunday 字符串搜索辅助，返回 p_szPattern 在 p_szText 中首次出现的位置。
char* SundayQuickSearch(const char* p_szText, const char* p_szPattern);

// 按指定分隔符切分字符串，旧订阅协议依赖尾部分隔符产生空 token 的行为。
void TokenizeOR(std::vector<std::string>& p_refTokens, const std::string& p_strText, const std::string& p_strDelimiters);

// 返回 YYYYMMDD 或 HHMMSS 数值，主要用于日志文件命名和诊断。
UINT GetCurDate(bool p_bDate = true);

// 获取当前进程所在目录，配置和日志默认都以该目录为根。
const char* GetRootPath();

// 循环发送直到指定字节全部写出，失败时返回 Winsock 错误码。
bool Sendn(SOCKET p_hSocket, const void* p_pBuf, int p_iSize, int* p_pLastError);

// 全局异步请求积压计数，用于上层判断是否需要限流。
extern long long g_lCrowded;

// 将 size_t 安全转换成旧 ABI 的长度类型，超过目标上限时按上限截断。
template<typename T>
T SafeSizeToLength(size_t p_uSize)
{
	const size_t uMaxValue = static_cast<size_t>((std::numeric_limits<T>::max)());
	if (p_uSize > uMaxValue)
	{
		return (std::numeric_limits<T>::max)();
	}
	return static_cast<T>(p_uSize);
}
// 将 long long 安全转换成协议字段宽度，低于/高于目标范围时按边界截断。
template<typename T>
T SafeLongLongToLength(long long p_lValue)
{
	const long long lMinValue = static_cast<long long>((std::numeric_limits<T>::min)());
	const unsigned long long uMaxValue = static_cast<unsigned long long>((std::numeric_limits<T>::max)());
	if (p_lValue < lMinValue)
	{
		return (std::numeric_limits<T>::min)();
	}
	if (static_cast<unsigned long long>(p_lValue) > uMaxValue)
	{
		return (std::numeric_limits<T>::max)();
	}
	return static_cast<T>(p_lValue);
}
