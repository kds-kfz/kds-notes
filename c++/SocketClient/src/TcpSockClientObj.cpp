#include "TcpSockClientObj.h"

// 为 TCP 实例选择独立的原始字节流组件和 TCP 监听器。
CTcpSockClientObj::CTcpSockClientObj(const char* p_szName, unsigned long long p_ullId)
    : ClientInstance(EN_SOCKET_CLIENT_TCP, p_szName, p_ullId) {}
