#if !defined(__MA_DEFINE_H__)
#define __MA_DEFINE_H__

#define BGN_NAMESPACE_MA        namespace ma {
#define END_NAMESPACE_MA        }
#define USE_NAMESPACE_MA        using namespace ma;

#define MA_KO                          -1
#define MA_OK                           0
#define MA_INFO                         0
#define MA_DEBUG                        1
#define MA_WARN                         2
#define MA_ERROR                        3
#define MA_FATAL                        4
#define MA_TRACE                        5

#define MA_NO_DATA                      100
#define MA_TIMEOUT                      101
#define MA_EXISTS                       102
#define MA_MORE                         103

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

#if defined(OS_IS_WINDOWS)
  #define MA_DLL_EXPORT         __declspec(dllexport)
  #define MA_DLL_IMPORT         __declspec(dllimport)
#else
  #define MA_DLL_EXPORT
  #define MA_DLL_IMPORT
#endif  // defined(OS_IS_WINDOWS)


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

#if defined(OS_IS_WINDOWS)
  //#define snprintf              _snprintf
  #define atoll                 _atoi64
  #define memicmp               _memicmp
  #define stricmp               _stricmp
  #define strnicmp              _strnicmp
#else
  #define stricmp               strcasecmp
  #define strnicmp              strncasecmp
#endif  // defined(OS_IS_WINDOWS)

// Base types.
#if !defined(OS_IS_WINDOWS) && !defined(__SQL)
  typedef signed int            BOOL;

  typedef unsigned char         BYTE, *PBYTE;
  typedef unsigned short        WORD, *PWORD;
  typedef unsigned int          DWORD, *PDWORD;

  typedef void                  VOID, *PVOID, *HANDLE;

  //typedef signed char           CHAR, *PCHAR;
  typedef char                  CHAR, *PCHAR;
  typedef unsigned char         UCHAR, *PUCHAR;
  typedef signed short          SHORT, *PSHORT;
  typedef unsigned short        USHORT, *PUSHORT;
  typedef signed int            INT, *PINT;
  typedef unsigned int          UINT, *PUINT;
  typedef signed long           LONG, *PLONG;
  typedef unsigned long         ULONG, *PULONG;
  typedef signed long long      LONGLONG, *PLONGLONG;
  typedef unsigned long long    ULONGLONG, *PULONGLONG;
  typedef float                 FLOAT, *PFLOAT;
  typedef double                DOUBLE, *PDOUBLE;

  typedef signed char           INT8, *PINT8;
  typedef unsigned char         UINT8, *PUINT8;
  typedef signed short          INT16, *PINT16;
  typedef unsigned short        UINT16, *PUINT16;
  typedef signed int            INT32, *PINT32;
  typedef unsigned int          UINT32, *PUINT32;
  #if defined(OS_IS_WINDOWS)
    typedef signed __int64      INT64, *PINT64;
    typedef unsigned __int64    UINT64, *PUINT64;
  #else
    typedef LONGLONG            INT64, *PINT64;
    typedef ULONGLONG           UINT64, *PUINT64;
  #endif

  #if (defined(OS_IS_LINUX) && !defined(__xlC__))
    typedef INT8                __int8;
    typedef INT16               __int16;
    typedef INT32               __int32;
    typedef INT64               __int64;
  #endif
/*
  #if defined(_WIN64)
    typedef INT64               INT_PTR, *PINT_PTR;
    typedef UINT64              UINT_PTR, *PUINT_PTR;

    typedef INT64               LONG_PTR, *PLONG_PTR;
    typedef UINT64              ULONG_PTR, *PULONG_PTR;
  #else
    typedef _W64 INT            INT_PTR, *PINT_PTR;
    typedef _W64 UINT           UINT_PTR, *PUINT_PTR;

    typedef _W64 LONG           LONG_PTR, *PLONG_PTR;
    typedef _W64 ULONG          ULONG_PTR, *PULONG_PTR;
  #endif
*/
  typedef WORD                  WCHAR, *PWCHAR;
  #if !defined(_TCHAR_DEFINED)
    #if defined(UNICODE)
      typedef WCHAR             TCHAR, *PTCHAR;
    #else
      //typedef CHAR              TCHAR, *PTCHAR;
      typedef char              TCHAR, *PTCHAR;
    #endif
  #endif  // !defined(_TCHAR_DEFINED)

  typedef char                  *LPSTR;
  typedef CONST CHAR            *LPCSTR;
  typedef TCHAR                 *LPTSTR;
  typedef CONST TCHAR           *LPCTSTR;
#endif  // !defined(OS_IS_WINDOWS) && !defined(__SQL)

#if defined(OS_IS_LINUX)
  typedef long long             INT64, *PINT64;
  typedef unsigned long long    UINT64, *PUINT64;
#endif


// Base defining functions.
#if !defined(_ma_max)
  //#define max(a,b)              (((a) > (b)) ? (a) : (b))
  #define _ma_max(a,b)          (((a) > (b)) ? (a) : (b))
#endif

#if !defined(_ma_min)
  //#define min(a,b)              (((a) < (b)) ? (a) : (b))
  #define _ma_min(a,b)          (((a) < (b)) ? (a) : (b))
#endif

#if !defined(MAKEWORD) && !defined(OS_IS_WINDOWS)
  #define MAKEWORD(a, b)        ((WORD)(((BYTE)((DWORD)(a) & 0xff)) | ((WORD)((BYTE)((DWORD)(b) & 0xff))) << 8))
#endif
#if !defined(MAKELONG) && !defined(OS_IS_WINDOWS)
  #define MAKELONG(a, b)        ((LONG)(((WORD)((DWORD)(a) & 0xffff)) | ((DWORD)((WORD)((DWORD)(b) & 0xffff))) << 16))
#endif
#if !defined(LOWORD) && !defined(OS_IS_WINDOWS)
  #define LOWORD(l)             ((WORD)((DWORD)(l) & 0xffff))
#endif
#if !defined(HIWORD) && !defined(OS_IS_WINDOWS)
  #define HIWORD(l)             ((WORD)((DWORD)(l) >> 16))
#endif
#if !defined(LOBYTE) && !defined(OS_IS_WINDOWS)
  #define LOBYTE(w)             ((BYTE)((DWORD)(w) & 0xff))
#endif
#if !defined(HIBYTE) && !defined(OS_IS_WINDOWS)
  #define HIBYTE(w)             ((BYTE)((DWORD)(w) >> 8))
#endif

#if defined(OS_IS_WINDOWS) && !defined(snprintf)
#include <stdio.h>
#include <stdarg.h>
#define snprintf(str, size, format, ...)  (_snprintf(str, size, format, ## __VA_ARGS__) + ((str)[size -1] = 0x00))
#endif

#endif  // __MA_DEFINE_H__