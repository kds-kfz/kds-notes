#ifndef SOCKET_SERVER_PLATFORM_H
#define SOCKET_SERVER_PLATFORM_H

// Use the bundled HP-Socket SDK on Windows and the native SDK on Linux.
#if defined(_WIN32)
#include "SocketInterface.h"
#include "HPSocket.h"
#include <windows.h>
#define SOCKET_SERVER_TRY __try
#define SOCKET_SERVER_CATCH(p_dwCode) __except ((p_dwCode) = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER)
#else
#if defined(__has_include)
#if __has_include(<hpsocket/HPSocket.h>) && __has_include(<hpsocket/SocketInterface.h>)
#include <hpsocket/SocketInterface.h>
#include <hpsocket/HPSocket.h>
#elif __has_include(<HPSocket.h>) && __has_include(<SocketInterface.h>)
#include <SocketInterface.h>
#include <HPSocket.h>
#else
#error Linux build requires the native HP-Socket development headers.
#endif
#else
#include <hpsocket/SocketInterface.h>
#include <hpsocket/HPSocket.h>
#endif
// Linux catches C++ exceptions, not process signals such as access violations.
#define SOCKET_SERVER_TRY try
#define SOCKET_SERVER_CATCH(p_dwCode) catch (...)
#endif

#endif
