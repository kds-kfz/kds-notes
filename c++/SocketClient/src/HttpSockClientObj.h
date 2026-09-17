#ifndef HTTP_SOCK_CLIENT_OBJ_H
#define HTTP_SOCK_CLIENT_OBJ_H

#include "ClientInstance.h"

// HTTP 协议客户端对象；请求和响应均使用 HP-Socket HTTP 解析器。
class CHttpSockClientObj final : public ClientInstance
{
public:
    // 绑定工厂名称和唯一诊断编号，并构造 HTTP 底层组件。
    CHttpSockClientObj(const char* p_szName, unsigned long long p_ullId);
};

#endif
