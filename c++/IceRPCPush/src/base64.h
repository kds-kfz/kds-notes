#pragma once

#include <stddef.h>

// Base64Encode
// 将原始二进制数据编码为 Base64，调用方需要提前准备足够大的 dest。
size_t Base64Encode(char* p_pDest,const char* p_szSrc,size_t p_uLen);

// Base64Decode
// 将 Base64 文本解码为原始字节，返回写入 dest 的有效长度。
//
size_t Base64Decode(char* p_pDest,const char* p_szSrc,size_t p_uLen);
