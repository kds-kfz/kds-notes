#ifndef TCP_SOCK_CLIENT_OBJ_H
#define TCP_SOCK_CLIENT_OBJ_H

#include "ClientInstance.h"

// TCP 协议客户端对象；每个对象独占一条 HP-Socket TCP 连接。
class CTcpSockClientObj final : public ClientInstance
{
public:
    // 绑定工厂名称和唯一诊断编号，并构造 TCP 底层组件。
    CTcpSockClientObj(const char* p_szName, unsigned long long p_ullId);
};

#endif
