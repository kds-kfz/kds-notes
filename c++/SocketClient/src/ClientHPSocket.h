#ifndef SOCKET_CLIENT_HPSOCKET_H
#define SOCKET_CLIENT_HPSOCKET_H

// Windows 使用仓库内的服务端同版头文件；Linux 使用原生发行包的头文件。
// 两个平台的 HP-Socket 实现不同，不得把 Windows 头文件带入 Linux 构建。
#if defined(_WIN32)
#include "HPSocket.h"
#elif defined(__has_include)
#if __has_include(<hpsocket/HPSocket.h>)
#include <hpsocket/HPSocket.h>
#elif __has_include(<HPSocket.h>)
#include <HPSocket.h>
#else
#error Linux build requires HP-Socket development headers.
#endif
#else
#include <hpsocket/HPSocket.h>
#endif

#endif
