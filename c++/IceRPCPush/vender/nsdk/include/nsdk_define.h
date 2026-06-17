#ifndef _NSDK_DEFINE_H_
#define _NSDK_DEFINE_H_

// Base define.
#if defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN64) || defined(_INC_WINDOWS)
#if !defined(OS_IS_WINDOWS)
#undef OS_IS_LINUX
#define OS_IS_WINDOWS
#endif
#else
#undef OS_IS_WINDOWS
#if !defined(OS_IS_LINUX)
#define OS_IS_LINUX
#endif
#endif

#define NSDK_OK                 0
#define NSDK_KO                 -1
#define NSDK_INVALID_PARAMTER   -99
#define NSDK_NO_DATA            100
#define NSDK_TIMEOUT            101
#define NSDK_EXISTS             102

#if !defined(_W64)
#if defined(OS_IS_WINDOWS) && _MSC_VER >= 1300
#define _W64 __w64
#else
#define _W64
#endif
#endif

#if !defined(NULL)
#ifdef __cplusplus
#define NULL                0
#else
#define NULL                ((void *)0)
#endif
#endif

#if !defined(CONST)
#define CONST                 const
#endif

#if !defined(FALSE)
#define FALSE                 0
#endif

#if !defined(TRUE)
#define TRUE                  1
#endif

#if !defined(OS_IS_WINDOWS)
#if !defined(INFINITE)
#define INFINITE              0xFFFFFFFF
#endif

#if !defined(WAIT_TIMEOUT)
#define WAIT_TIMEOUT          0x00000102
#endif

#if !defined(WAIT_OBJECT_0)
#define WAIT_OBJECT_0         0x00000000
#endif

#if !defined(WAIT_ABANDONED)
#define WAIT_ABANDONED        0x00000080
#endif

#if !defined(WAIT_FAILED)
#define WAIT_FAILED           0xFFFFFFFF
#endif
#endif

#define BGN_NAMESPACE_NSDK namespace nsdk {
#define END_NAMESPACE_NSDK }
#define USE_NAMESPACE_NSDK using namespace nsdk;

#endif
