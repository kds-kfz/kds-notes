#ifndef _FUBLICFUNC_H_
#define _FUBLICFUNC_H_

#include <cstddef>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <iostream>
#include "nsdk.h"

#define GDEL_ARRAY(p) { if (p) delete [](p); p = nullptr; }
#define GDEL(p) { if (p) delete (p); p = nullptr; }

// 宏定义
#define foreach(container, it) \
	for( decltype((container).begin()) it = (container).begin(); it != (container).end(); ++it)

#define reverse(container, it) \
	for( decltype((container).rbegin()) it = (container).rbegin(); it != (container).rend(); ++it)

#endif

