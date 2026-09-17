#ifndef WEB_SOCK_CLIENT_OBJ_H
#define WEB_SOCK_CLIENT_OBJ_H

#include "ClientInstance.h"

// WebSocket 客户端对象；先验证 HTTP 升级再发送或接收帧。
class CWebSockClientObj final : public ClientInstance
{
public:
    // 绑定工厂名称和唯一诊断编号，并构造升级用 HTTP 组件。
    CWebSockClientObj(const char* p_szName, unsigned long long p_ullId);
};

#endif
