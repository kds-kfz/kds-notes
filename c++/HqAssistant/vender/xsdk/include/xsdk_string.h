#if !defined(__XSDK_STRING_H__)
#define __XSDK_STRING_H__

#include "xsdk_define.h"

#if defined(OS_IS_AIX) || defined(OS_IS_LINUX)
  #include <stddef.h>
#endif

BGN_NAMESPACE_XSDK

#ifdef  __cplusplus
extern "C"
{
#endif

#if defined(OS_IS_AIX) || defined(OS_IS_LINUX)

int memicmp(
  const void *p_pvdFirst,
  const void *p_pvdLast,
  size_t p_llCount
);
/*
int stricmp(
  const char *p_pszFirst,
  const char *p_pszLast
);

int strnicmp(
  const char *p_pszFirst,
  const char *p_pszLast,
  size_t p_llCount
);
*/
char * strrev(
  char *p_pszString
);

char * strlwr(
  char *p_pszString
);

char * strupr(
  char *p_pszString
);

int getch(void);

#endif  // defined(OS_IS_AIX) || defined(OS_IS_LINUX)


//------------------------------------------------------------------------------
// 功能描述：
//     滤除给定字符串首部的空格和制表符（或指定字符）
// 参数说明：
//     p_pszString[inout]
//         给定待滤除首部空格和制表符（或指定字符）的指针
//     p_chTrimChar[in]
//         指定过滤字符，不指定，则过滤空格和制表符
// 返回说明：
//     直接返回p_pszString
char * TrimLeft(
  char *p_pszString,
  char p_chTrimChar = 0x00
);

//------------------------------------------------------------------------------
// 功能描述：
//     滤除给定字符串尾部的空格和制表符（或指定字符）
// 参数说明：
//     p_pszString[inout]
//         给定待滤除尾部空格和制表符（或指定字符）的指针
//     p_chTrimChar[in]
//         指定过滤字符，不指定，则过滤空格和制表符
// 返回说明：
//     直接返回p_pszString
char * TrimRight(
  char *p_pszString,
  char p_chTrimChar = 0x00
);

//------------------------------------------------------------------------------
// 功能描述：
//     滤除给定字符串首尾的空格和制表符（或指定字符）
// 参数说明：
//     p_pszString[inout]
//         给定待滤除首尾空格和制表符（或指定字符）的指针
//     p_chTrimChar[in]
//         指定过滤字符，不指定，则过滤空格和制表符
// 返回说明：
//     直接返回p_pszString
char * Trim(
  char *p_pszString,
  char p_chTrimChar = 0x00
);

//------------------------------------------------------------------------------
// 功能描述：
//     滤除给定字符串中所有空格和制表符（或指定字符）
// 参数说明：
//     p_pszString[inout]
//         给定待滤除所有空格和制表符（或指定字符）的指针
//     p_chFilterChar[in]
//         指定过滤字符，不指定，则过滤空格和制表符
// 返回说明：
//     直接返回p_pszString
char * FilterChar(
  char *p_pszString,
  char p_chFilterChar = 0x00
);

//------------------------------------------------------------------------------
// 功能描述：
//     在字符串查找子字符串，查找方式支持大小写敏感以及整字匹配的组合方式。
//     整字定义：连续的由下划线、数字、字母组成的单词。
// 参数说明：
//     p_pszString[in]
//         被查找字符串
//     p_pszSubStr[in]
//         待查找字符串
//     p_bMatchCase[in]
//         大小写匹配开关参数：true大小写敏感，false大小写不敏感
//     p_bMatchWord[in]
//         整字匹配开关参数：true按整字匹配，false不按整字匹配
// 返回说明：
//     若被查找字符串中不包含待查找字符串，则返回空值(NULL)
//     若被查找字符串中包含待查找字符串，则返回待查字符串在被查字符串中的位置指针(!NULL)
char * FindString(
  const char *p_pszString,
  const char *p_pszSubStr,
  bool p_bMatchCase = true,
  bool p_bMatchWord = false
);

//------------------------------------------------------------------------------
// 功能描述：
//     给定字符串与带有通配符（'?'和'*'）的字符串进行匹配
// 参数说明：
//     p_pszSource[in]
//         需要匹配的字符串字符串指针
//     p_pszMatch[in]
//         带有通配符（'?'和'*'）的字符串指针
//     p_bMatchCase[in]
//         true，匹配是大小写敏感，否则不敏感，缺省值：true
// 返回说明：
//     匹配成功返回true，否则返回false
bool MatchString(
  const char *p_pszSource, 
  const char *p_pszMatch, 
  bool p_bMatchCase = true
);

//------------------------------------------------------------------------------
// 功能描述：
//     查找字符串并替换。
//     如果输出存储地址(p_pszDestBuff)与待操作字符串地址(p_pszString)相同，则在原字符串上进行替换操作。
// 参数说明：
//     p_pszDestBuff[out]
//         输出数据的空间地址，空间必须足够大，否则替换失败
//     p_iDestBuffSize[in]
//         p_pszDestBuff的空间大小
//     p_pszString[in]
//         待操作字符串
//     p_pszFindStr[in]
//         被替换字符串
//     p_pszReplStr[in]
//         将替换字符串
//     p_bMatchCase[in]
//         大小写匹配开关参数：true大小写敏感，false大小写不敏感
//     p_bMatchWord[in]
//         整字匹配开关参数：true按整字匹配，false不按整字匹配
// 返回说明：
//     若替换失败，则返回空值(NULL)
//     若替换成功，则返回被替换位置之后替换字符串长度的偏移地址(!NULL)
char * ReplaceString(
  char *p_pszDestBuff,
  int p_iDestBuffSize,
  const char *p_pszString,
  const char *p_pszFindStr,
  const char *p_pszReplStr,
  bool p_bMatchCase = true,
  bool p_bMatchWord = false
);

//------------------------------------------------------------------------------
// 功能描述：
//     从格式字符串中提取指定标签的字符串
//     格式字符串中各标签字符串，以逗号或指定字符分隔，如："NAME=ZhongGQ, TEL=13808808699"
//     按标签"NAME"提取的字符串是"ZhongGQ"
// 参数说明：
//     p_pszDestBuff[out]
//         输出数据的空间地址，空间必须足够大，否则会被自动截除
//     p_iDestBuffSize[in]
//         p_pszDestBuff的空间大小
//     p_pszString[in]
//         格式字符串
//     p_pszTagStr[in]
//         标签字符串
//     p_chMidChar[in]
//         中间分隔符，缺省值：'='
//     p_chEndChar[in]
//         结束分隔符，缺省值：','
//     p_bMatchCase[in]
//         查找标签时大小写匹配，缺省值：false
// 返回说明：
//     p_pszDestBuff
char * SubTagString(
  char *p_pszDestBuff,
  int p_iDestBuffSize,
  const char *p_pszString,
  const char *p_pszTagStr,
  char p_chMidChar = '=',
  char p_chEndChar = ',',
  bool p_bMatchCase = false
);

//------------------------------------------------------------------------------
// 功能描述：
//     从具有固定分隔符字符串(Delimited string)中，提取第N节的字符串
//     如："ZhongGQ,13808808699"，以逗号分隔，第0节的字符串是："ZhongGQ"，
//     第1节的字符串是："13808808699"
// 参数说明：
//    p_pszDestBuff[out]
//        输出数据的空间地址，空间必须足够大，否则会被自动截除
//    p_iDestBuffSize[in]
//        p_pszDestBuff的空间大小
//    p_pszString[in]
//        格式字符串
//    p_iNodeIdx[in]
//        第N节，以：0为起点
//    p_chDelimiter[in]
//        分隔符，缺省值：','
// 返回说明：
//     p_pszDestBuff
char * SubDelString(
  char *p_pszDestBuff,
  int p_iDestBuffSize,
  const char *p_pszString,
  int p_iNodeIdx,
  char p_chDelimiter = ','
);

//------------------------------------------------------------------------------
// 功能描述：
//     将金额数字转换为中文读数
// 参数说明：
//     p_pszReadingStr[out]
//         转换后的金额读数，其中p_pszReadingStr的空间大小要求大于256字节。
//     p_dNumber[in]
//         待转换的数字
// 返回说明：
//     返回p_lpszReadingStr
char * MoneyReading(
  char *p_pszReadingStr,
  double p_dNumber
);

//------------------------------------------------------------------------------
// 功能描述：
//     将十六进制的数值转换成十六进制的字串
//     例 :
//         p_pszString = 010203040A0B0C0D
//         p_pszHex = 0x01 0x02 0x03 0x04 0x0A 0x0B 0x0C 0x0D
//         p_iLength = 8
// 参数说明：
//     p_pszString[out]
//         用于存放转换后的十六进制字串，其大小必须是p_iLength的2倍
//     p_pszHex[in]
//         将要转换的十六进制数值
//     p_iLength[in]
//         标识p_lpszHex的长度
// 返回说明：
//     返回p_pszString
unsigned char * H2A(
  unsigned char *p_pszString,
  const unsigned char *p_pszHex,
  int p_iLength
);

//------------------------------------------------------------------------------
// 功能描述：
//     将十六进制的字串转换成十六进制的数值
//     例 :
//         p_pszHex = 0x01 0x02 0x03 0x04 0x0A 0x0B 0x0C 0x0D
//         p_pszString = 010203040A0B0C0D
//         p_iLength = 8
// 参数说明：
//     p_pszHex[out]
//         用于存放转换后的十六进制数值，其大小必须是p_iLength
//     p_pszString[in]
//         将要转换的十六进制表示的字串，其大小必须是p_iLength的2倍
//     p_iLength[in]
//         标识p_lpszHex的长度
// 返回说明：
//     返回p_pszHex
unsigned char * A2H(
  unsigned char *p_pszHex,
  const unsigned char *p_pszString,
  int p_iLength
);

#ifdef __cplusplus
}
#endif

END_NAMESPACE_XSDK

#endif  // __XSDK_STRING_H__
