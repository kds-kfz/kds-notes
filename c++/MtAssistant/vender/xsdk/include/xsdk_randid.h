//------------------------------------------------------------------
// 版权声明：本程序模块属于金证券商综合业务系统(KBSS)的一部分
//           金证科技股份有限公司  版权所有
//
// 文件名称：xsdk_randid.h
// 模块名称：链路追踪traceid和spandid生成类
// 模块描述：通过该类产生traceid 和spandid
//
// 开发作者：顾富洋
// 创建日期：2022-06-13
// 模块版本：1.0.000.000
//-------------------------------------------------------------------
// 修改日期      版本              作者            备注
//-------------------------------------------------------------------
// 2022-06-13  1.0.000.000        钟兆斌          原创
//-------------------------------------------------------------------

#if !defined(__xsdk_randid_h__)
#define __xsdk_randid_h__
#include "xsdk_define.h"
#include <string.h>
BGN_NAMESPACE_XSDK
#define _CRT_RAND_S
#if defined(_CRT_RAND_S)
#endif
#ifdef OS_IS_WINDOWS
#pragma once
extern"C"
{
  _CRTIMP errno_t __cdecl rand_s ( __out unsigned int *_RandomValue);
}
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/timeb.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#ifndef  uint64_t
typedef unsigned long long uint64_t;
#endif

static  uint64_t TRACEID_TIME_SHIFT = 28; //生成traceid时，时间需要左移的位数（把右边28位空出来给随机数）
static  uint64_t TRACEID_TIME_MAX = (1 << 28) - 1; //traceid中时间的最大值（最大的28位数字）
static  uint64_t TRACEID_MAX_RAND = (1 << 28) - 1; //traceid中随机数的最大值（最大的28位数字）

static  uint64_t SPANID_MAX = 1 << 28; //spanid的最大数。bpu设计是用4个128进制（7位）字符来存

class CRandId
{
  int fd;

public:
  CRandId()
  {
    fd = -1;

#if defined(OS_IS_LINUX)
    fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0)
    {
      //printf("open /dev/urandom error: %d\n", errno);
    }
#endif // __linux__ 
  }

  ~CRandId()
  {

#if defined(OS_IS_LINUX)
    if (fd > 0)
    {
      close(fd);
    }
#endif // __linux__ 
  }

  //获取traceid：秒时间（高28位）+随机数（低28位）
  uint64_t get_traceid()
  {
    uint64_t timestamp = getsecond() & TRACEID_TIME_MAX;
    unsigned number = getid();

    return (timestamp << TRACEID_TIME_SHIFT)
           | (number & TRACEID_MAX_RAND);
  }

  //获取spanid：28位以内的随机数
  unsigned get_spanid()
  {
    unsigned number = getid();
    return number % SPANID_MAX;
  }

private:
  //获取现在的秒数时间
  uint64_t getsecond() const
  {
#if defined(OS_IS_LINUX)
    struct timeval  aTimeVal;
    struct timezone aTimezone;
    gettimeofday(&aTimeVal, &aTimezone);
    return aTimeVal.tv_sec;
#else
    struct timeb tb;
    ftime(&tb);
    return tb.time;
#endif
  }

  //生成随机数
  unsigned getid()
  {
    unsigned number = 0;

#if defined(OS_IS_LINUX)
    if (fd >= 0)
    {
      read(fd, &number, sizeof(number));
    }
    else
    {
      number = rand();
    }
#elif defined(OS_IS_WINDOWS)
    rand_s(&number);
#else
    number = rand();
#endif

    return number;
  }

public:
  //------------------------------------------------------------------------------
// 功能描述：
//     将十进制的字串转换成128进制的字符串
//     例 :
//         nNumber = 123456
//         pszData = 000007tp
//         nDataLen = 8
// 参数说明：
//     pszData[out]
//         用于存放转换后的128进制数值，其大小必须是p_iLength
//     nNumber[in]
//         将要转换的十进制数
//     nDataLen[in]
//         标识pszData的长度
// 返回说明：
//     返回0正确，-1失败
  static int Encode(unsigned long long nNumber,unsigned char* pszData, int nDataLen)
  {
    if (pszData != NULL && nDataLen <= 10)
    {
      unsigned char szNumber[10];
      int nLen = nDataLen;
      int nPos = nDataLen;
      while (--nLen >= 0 && --nPos >= 0)
      {
        szNumber[nPos] = (unsigned char)(nNumber & 0x007f) + 48;
        nNumber >>= 7;
      }
      memcpy((void*)pszData, (void*)&szNumber[nPos], nDataLen);
      return 0;
    }
    return -1;
  }
  //------------------------------------------------------------------------------
// 功能描述：
//     将128制的字串转换成十进制的数字
//     例 :
//         nNumber = 123456
//         pszData = 000007tp
//         nDataLen = 8
// 参数说明：
//     pszData[in]
//         用于存放待转换的128进制数值，其大小必须是p_iLength
//     nNumber[out]
//         转换后的十进制数
//     nDataLen[in]
//         标识pszData的长度
// 返回说明：
//     返回0正确，-1失败
  static int Decode(unsigned long long& nNumber, const unsigned char* pszData, int nDataLen)
  {
    unsigned char* pszNuber = (unsigned char*)pszData + nDataLen - 1;
    int nPos = 0;
    int nLen = nDataLen;
    while (pszNuber != NULL && *pszNuber >= 48 && --nLen >= 0)
    {
      nNumber += (*pszNuber - 48) * (1LL << (7 * nPos));
      ++nPos;
      --pszNuber;
    }
    return 0;
  }

};

END_NAMESPACE_XSDK
#endif       //__xsdk_randid_h__