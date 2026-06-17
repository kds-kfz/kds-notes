#pragma once

// snappy 库已按旧 C++ 接口编译，直接复用 snappy.hHandle 中的 Compress/Uncompress/RawCompress 等导出函数。
#include "snappy.h"