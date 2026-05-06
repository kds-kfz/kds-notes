#if !defined(__xsdk_base64_h__)

#include "xsdk_define.h"


BGN_NAMESPACE_XSDK

#ifdef  __cplusplus
extern "C"
{
#endif

//------------------------------------------------------------------------------
// 功能描述：
//     Base64算法的编码
// 参数说明：
//     p_pszText[out]
//         存储编码后数据的内存空间指针
//     p_refiTextLen[out]
//         返回编码后数据的字节大小
//     p_iTextSize[in]
//         p_pszText指向空间的字节数，空间大小至少满足以下公式：
//         (((p_iDataSize / 3) + max(1, (p_iDataSize % 3))) * 4) + 1
//     p_pszData
//         指向待编码数据的内存空间指针
//     p_iDataSize
//         待编码数据的有效字节大小
// 返回说明：
//     XSDK_OK
//         函数执行成功
//     XSDK_KO
//         函数执行失败
//     XSDK_INVALID_PARAMTER
//         无效的参数
int Base64Encrypt(
  char *p_pszText,
  int &p_refiTextLen,
  int p_iTextSize,
  const char *p_pszData,
  int p_iDataSize
);

//------------------------------------------------------------------------------
// 功能描述：
//     Base64算法的解码
// 参数说明：
//     p_pszData[out]
//         存储解码后数据的内存空间指针
//     p_refiDataLen[out]
//         返回解码后数据的字节大小
//     p_iDataSize[in]
//         p_pszData指向空间的字节数，空间大小至少满足以下公式：
//         ((p_iTextSize / 4) * 3) + 1
//     p_pszText[in]
//         指向待解码数据的内存空间指针
//     p_iTextSize
//         待解码数据的有效大小，大小必须是4的倍数
// 返回说明：
//     XSDK_OK
//         函数执行成功
//     XSDK_KO
//         函数执行失败
//     XSDK_INVALID_PARAMTER
//         无效的参数
int Base64Decrypt(
  char *p_pszData,
  int &p_refiDataLen,
  int p_iDataSize,
  const char *p_pszText,
  int p_iTextSize
);

#ifdef  __cplusplus
}
#endif

END_NAMESPACE_XSDK

#endif  // __xsdk_base64_h__
