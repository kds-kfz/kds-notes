#ifndef base64_h
#define base64_h

// 将二进制数据编码到调用方提供的缓冲区，末尾附加零字符。
// 缓冲区容量至少为 ((p_uiDataLength + 2) / 3) * 4 + 1。
int base64_encode(const unsigned char* p_bySourceData, unsigned int p_uiDataLength,
    char* p_szBase64);

#endif // Base64 头文件保护结束。
