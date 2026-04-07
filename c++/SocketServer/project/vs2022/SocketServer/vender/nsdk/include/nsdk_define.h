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

#define BGN_NAMESPACE_NSDK namespace nsdk {
#define END_NAMESPACE_NSDK }
#define USE_NAMESPACE_NSDK using namespace nsdk;

#endif
