#include "WebSockClientObj.h"

// 为 WebSocket 实例选择独立的握手及帧解析组件。
CWebSockClientObj::CWebSockClientObj(const char* p_szName, unsigned long long p_ullId)
    : ClientInstance(EN_SOCKET_CLIENT_WEB, p_szName, p_ullId) {}
