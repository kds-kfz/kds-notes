#ifndef _FUBLICFUNC_H_
#define _FUBLICFUNC_H_

#include <cstddef>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <tchar.h>

#include "nsdk.h"

#define GDEL_ARRAY(p) { if (p) delete [](p); p = NULL; }
#define GDEL(p) { if (p) delete (p); p = NULL; }

// 宏定义
#define foreach(container, it) \
	for( decltype((container).begin()) it = (container).begin(); it != (container).end(); ++it)

#define reverse(container, it) \
	for( decltype((container).rbegin()) it = (container).rbegin(); it != (container).rend(); ++it)

std::basic_string<TCHAR> MakeBindAddress(const char* p_szSrc);
void CopyTextToAnsi(char* p_szDst, size_t p_dwDstLen, const TCHAR* p_szSrc);

#endif
