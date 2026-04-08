#include "publicfunc.h"

#include <cstdlib>
#include <cstring>
#include <windows.h>

std::basic_string<TCHAR> MakeBindAddress(const char* p_szSrc)
{
#if defined(UNICODE) || defined(_UNICODE)
	if (nullptr == p_szSrc)
		return std::basic_string<TCHAR>();

	const size_t dwSrcLen = strlen(p_szSrc);
	std::vector<TCHAR> vecBuffer(dwSrcLen + 1, 0);
	size_t dwConverted = 0;
	if (0 != mbstowcs_s(&dwConverted, vecBuffer.data(), vecBuffer.size(), p_szSrc, _TRUNCATE))
		return std::basic_string<TCHAR>();
	return std::basic_string<TCHAR>(vecBuffer.data());
#else
	return (nullptr == p_szSrc) ? std::basic_string<TCHAR>() : std::basic_string<TCHAR>(p_szSrc);
#endif
}

void CopyTextToAnsi(char* p_szDst, size_t p_dwDstLen, const TCHAR* p_szSrc)
{
	if (nullptr == p_szDst || 0 == p_dwDstLen)
		return;

	p_szDst[0] = '\0';
	if (nullptr == p_szSrc)
		return;

#if defined(UNICODE) || defined(_UNICODE)
	WideCharToMultiByte(CP_ACP, 0, p_szSrc, -1, p_szDst, (int)p_dwDstLen, nullptr, nullptr);
#else
	_snprintf(p_szDst, p_dwDstLen, "%s", p_szSrc);
#endif
	p_szDst[p_dwDstLen - 1] = '\0';
}
