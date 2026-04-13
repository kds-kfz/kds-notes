#ifndef _FUBLICFUNC_H_
#define _FUBLICFUNC_H_

#include <cstddef>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <iostream>
#include "nsdk.h"
#include "nsdk_atomic.h"

// 宏定义
#define foreach(container, it) \
	for( decltype((container).begin()) it = (container).begin(); it != (container).end(); ++it)

//防止重名 reverse
#define foreach_reverse(container, it) \
	for( decltype((container).rbegin()) it = (container).rbegin(); it != (container).rend(); ++it)

int ReleasePackDataFunc(void* lpParameter);

#endif

