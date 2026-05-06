#if !defined(__XSDK_OEM_H__)
#define __XSDK_OEM_H__

#include "xsdk_define.h"

BGN_NAMESPACE_XSDK

#ifdef  __cplusplus
extern "C"
{
#endif


//------------------------------------------------------------------------------
// 功能描述：
//     OEM校验
// 参数说明：
//   p_pszOemName      厂商名称
//   p_pszOemCode      OEM代码
//   p_pszOemBzxx      OEM备注信息
//   p_pszZqszwmc      券商中文名称
//   p_iTrdDate        当前交易日期
//   p_refiValidDate   失效日期
//   p_iPinCode        PIN代码，缺省为6892
// 返回说明：
//   < 0 校验不通过
//   -1  OEM代码与厂商名称不符
//   -2  OEM非法！请向金证公司核实!(oldcrypt)
//   -3  OEM非法！请向金证公司核实!(newcrypt)
//   -4  备注信息有误,长度应为16/32位
//   >=0  校验通过, 返回值为尚余有效天数

int CheckOem(unsigned char *p_pszOemName,
             char *p_pszOemCode,
             char *p_pszOemBzxx,
             char *p_pszZqszwmc,
             int p_iTrdDate,
             int &p_refiValidDate,
             int p_iPinCode = 6892);

#ifdef __cplusplus
}
#endif

END_NAMESPACE_XSDK

#endif  // __XSDK_OEM_H__
