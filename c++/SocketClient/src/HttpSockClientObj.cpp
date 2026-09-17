#include "HttpSockClientObj.h"

// 为 HTTP 实例选择独立的协议组件和响应监听器。
CHttpSockClientObj::CHttpSockClientObj(const char* p_szName, unsigned long long p_ullId)
    : ClientInstance(EN_SOCKET_CLIENT_HTTP, p_szName, p_ullId) {}
