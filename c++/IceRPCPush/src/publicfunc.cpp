#include "publicfunc.h"

// 将 UTF-8 文本转为系统 ANSI 编码，避免旧日志查看工具显示 Ice 异常时乱码。
const char* UTF82ASC(const char* p_szSrcBuf, char* p_szOutBuf, int p_iOutLen)
{
	if (p_szOutBuf == NULL || p_iOutLen <= 0)
	{
		return p_szOutBuf;
	}
	p_szOutBuf[0] = '\0';
	if (p_szSrcBuf == NULL || p_szSrcBuf[0] == '\0')
	{
		return p_szOutBuf;
	}

	int iLen = MultiByteToWideChar(CP_UTF8, 0, p_szSrcBuf, -1, NULL, 0);
	if (iLen <= 0)
	{
		strncpy(p_szOutBuf, p_szSrcBuf, p_iOutLen - 1);
		p_szOutBuf[p_iOutLen - 1] = '\0';
		return p_szOutBuf;
	}

	std::vector<wchar_t> aWideBuf(iLen + 1, 0);
	MultiByteToWideChar(CP_UTF8, 0, p_szSrcBuf, -1, aWideBuf.data(), iLen);

	iLen = WideCharToMultiByte(CP_ACP, 0, aWideBuf.data(), -1, NULL, 0, NULL, NULL);
	if (iLen <= 0)
	{
		strncpy(p_szOutBuf, p_szSrcBuf, p_iOutLen - 1);
		p_szOutBuf[p_iOutLen - 1] = '\0';
		return p_szOutBuf;
	}

	std::vector<char> aAnsiBuf(iLen + 1, 0);
	WideCharToMultiByte(CP_ACP, 0, aWideBuf.data(), -1, aAnsiBuf.data(), iLen, NULL, NULL);
	strncpy(p_szOutBuf, aAnsiBuf.data(), p_iOutLen - 1);
	p_szOutBuf[p_iOutLen - 1] = '\0';
	return p_szOutBuf;
}

long long g_lCrowded = 0;	// 全局异步请求拥挤程度。

// 返回当前日期或时间的压缩整数，便于拼接日志文件名。
UINT GetCurDate(bool p_bDate)
{
	time_t lNow = time(NULL);
	tm stNowTime = {0};
	localtime_s(&stNowTime, &lNow);
	if (p_bDate)
	{
		return (stNowTime.tm_year + 1900) * 10000 + (stNowTime.tm_mon + 1) * 100 + stNowTime.tm_mday;
	}
	return stNowTime.tm_hour * 10000 + stNowTime.tm_min * 100 + stNowTime.tm_sec;
}

// 获取进程所在目录，历史配置和日志路径以此为默认根目录。
const char* GetRootPath()
{
	static char s_szModulePath[MAX_PATH] = {0};
	if (s_szModulePath[0] == '\0')
	{
		GetModuleFileNameA(NULL, s_szModulePath, sizeof(s_szModulePath) - 1);
		char* pPos = strrchr(s_szModulePath, '\\');
		if (pPos != NULL)
		{
			*pPos = '\0';
		}
	}
	return s_szModulePath;
}

// 循环发送直到指定长度全部写出，短写时继续补发。
bool Sendn(SOCKET p_hSocket, const void* p_pBuf, int p_iSize, int* p_pLastError)
{
	if (p_pBuf == NULL || p_iSize < 0)
	{
		if (p_pLastError != NULL)
		{
			*p_pLastError = WSAEINVAL;
		}
		return false;
	}

	int iLeft = p_iSize;
	const char* pBuffer = static_cast<const char*>(p_pBuf);
	while (iLeft > 0)
	{
		int iThisTime = send(p_hSocket, &pBuffer[p_iSize - iLeft], iLeft, 0);
		if (iThisTime == SOCKET_ERROR)
		{
			if (p_pLastError != NULL)
			{
				*p_pLastError = WSAGetLastError();
			}
			return false;
		}
		iLeft -= iThisTime;
	}
	return true;
}

// 按指定分隔符切分字符串，保留旧订阅协议尾部分隔符产生空段的行为。
void TokenizeOR(std::vector<std::string>& p_refTokens, const std::string& p_strText, const std::string& p_strDelimiters)
{
	std::string::size_type uLastPos = 0;
	std::string::size_type uPos = p_strText.find_first_of(p_strDelimiters, uLastPos);
	while (std::string::npos != uPos)
	{
		p_refTokens.push_back(p_strText.substr(uLastPos, uPos - uLastPos));
		uLastPos = uPos + 1;
		uPos = p_strText.find_first_of(p_strDelimiters, uLastPos);
	}

	if (uLastPos == 0)
	{
		p_refTokens.push_back(p_strText);
	}
	else if (uLastPos == p_strText.size())
	{
		p_refTokens.push_back("");
	}
}

// Sunday 快速搜索算法，旧订阅解析用它判断取消订阅标记。
char* SundayQuickSearch(const char* p_szText, const char* p_szPattern)
{
	if (p_szText == NULL || p_szPattern == NULL)
	{
		return NULL;
	}

	int iTextLen = static_cast<int>(strlen(p_szText));
	int iPatternLen = static_cast<int>(strlen(p_szPattern));
	if (iPatternLen == 0)
	{
		return const_cast<char*>(p_szText);
	}

	int aShiftTable[256] = {0};
	for (int i = 0; i < 256; ++i)
	{
		aShiftTable[i] = iPatternLen + 1;
	}

	const char* pPattern = NULL;
	for (pPattern = p_szPattern; *pPattern != '\0'; ++pPattern)
	{
		aShiftTable[static_cast<unsigned char>(*pPattern)] = iPatternLen - static_cast<int>(pPattern - p_szPattern);
	}

	const char* pText = p_szText;
	while (pText + iPatternLen <= p_szText + iTextLen)
	{
		const char* pCurPattern = p_szPattern;
		const char* pCurText = pText;
		for (; *pCurPattern != '\0'; ++pCurPattern, ++pCurText)
		{
			if (*pCurPattern != *pCurText)
			{
				break;
			}
		}
		if (*pCurPattern == '\0')
		{
			return const_cast<char*>(pText);
		}
		pText += aShiftTable[static_cast<unsigned char>(pText[iPatternLen])];
	}

	return NULL;
}