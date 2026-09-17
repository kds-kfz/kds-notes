#ifndef SOCKET_CLIENT_PROTOCOL_H
#define SOCKET_CLIENT_PROTOCOL_H

#include "SocketClient.h"
#include <cstddef>
#include <string>

// 对握手与工厂共用的协议辅助函数进行集中声明。
namespace ClientProtocol
{
    // WebSocket 单帧接收上限，避免异常服务端无限占用内存。
    const size_t kMaxWebFrame = 16U * 1024U * 1024U;

    // 使用服务端工程复制的 base64_encode 编码二进制内容。
    std::string EncodeBase64(const unsigned char* p_byData, size_t p_uiLength);

    // 使用平台安全随机数源生成握手密钥或客户端帧掩码。
    bool RandomBytes(unsigned char* p_byOutput, size_t p_uiLength);

    // 使用 Windows CNG 计算 RFC 6455 握手应答值。
    bool WebAccept(const std::string& p_strKey, std::string& p_refOut);

    // HTTP 头名按 ASCII 规则进行不区分大小写比较。
    bool HeaderEquals(const char* p_szLeft, const char* p_szRight);

    // 校验不超过 63 字节的工厂 ASCII 标识符。
    bool ValidName(const char* p_szName);

    // 返回零初始化的公开事件结构体。
    ST_SOCKET_CLIENT_EVENT Event(EN_SOCKET_CLIENT_EVENT p_enType);
}

#endif
