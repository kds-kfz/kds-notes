#include "Base64.h"
#include <stdio.h>
#include <string.h>

// 全局常量定义
const char * base64char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const char padding_char = '=';

// 沿用 SocketServer 的编码算法，将源字节写入调用方提供的输出缓冲区。
int base64_encode(const unsigned char* p_bySourceData, unsigned int p_uiDataLength,
    char* p_szBase64)
{
	int i = 0, j = 0;
	unsigned char trans_index = 0;    // 索引是8位，但是高两位都为0
	for (; i < p_uiDataLength; i += 3) {
		// 每三个一组，进行编码
		// 要编码的数字的第一个
		trans_index = ((p_bySourceData[i] >> 2) & 0x3f);
		p_szBase64[j++] = base64char[(int)trans_index];
		// 第二个
		trans_index = ((p_bySourceData[i] << 4) & 0x30);
		if (i + 1 < p_uiDataLength) {
			trans_index |= ((p_bySourceData[i + 1] >> 4) & 0x0f);
			p_szBase64[j++] = base64char[(int)trans_index];
		}
		else {
			p_szBase64[j++] = base64char[(int)trans_index];

			p_szBase64[j++] = padding_char;

			p_szBase64[j++] = padding_char;

			break;   // 超出总长度，可以直接break
		}
		// 第三个
		trans_index = ((p_bySourceData[i + 1] << 2) & 0x3c);
		if (i + 2 < p_uiDataLength) { // 有的话需要编码2个
			trans_index |= ((p_bySourceData[i + 2] >> 6) & 0x03);
			p_szBase64[j++] = base64char[(int)trans_index];

			trans_index = p_bySourceData[i + 2] & 0x3f;
			p_szBase64[j++] = base64char[(int)trans_index];
		}
		else {
			p_szBase64[j++] = base64char[(int)trans_index];

			p_szBase64[j++] = padding_char;

			break;
		}
	}

	p_szBase64[j] = '\0';

	return 0;
}


// 在指定字符串中查询字符位置，不存在时返回 -1。
inline int num_strchr(const char* p_szText, char p_chValue)
{
	const char *pindex = strchr(p_szText, p_chValue);
	if (nullptr == pindex) {
		return -1;
	}
	return pindex - p_szText;
}
// 沿用 SocketServer 的解码算法，将 Base64 码字还原到输出缓冲区。
int base64_decode(const char* p_szBase64, unsigned char* p_byDecoded)
{
	int i = 0, j = 0;
	int trans[4] = { 0, 0, 0, 0 };
	for (; p_szBase64[i] != '\0'; i += 4) {
		// 每四个一组，译码成三个字符
		trans[0] = num_strchr(base64char, p_szBase64[i]);
		trans[1] = num_strchr(base64char, p_szBase64[i + 1]);
		// 1/3
		p_byDecoded[j++] = ((trans[0] << 2) & 0xfc) | ((trans[1] >> 4) & 0x03);
		if (p_szBase64[i + 2] == '=') {
			continue;
		}
		else {
			trans[2] = num_strchr(base64char, p_szBase64[i + 2]);
		}
		// 2/3
		p_byDecoded[j++] = ((trans[1] << 4) & 0xf0) | ((trans[2] >> 2) & 0x0f);
		if (p_szBase64[i + 3] == '=') {
			continue;
		}
		else {
			trans[3] = num_strchr(base64char, p_szBase64[i + 3]);
		}

		// 3/3
		p_byDecoded[j++] = ((trans[2] << 6) & 0xc0) | (trans[3] & 0x3f);
	}

	p_byDecoded[j] = '\0';

	return 0;
}
