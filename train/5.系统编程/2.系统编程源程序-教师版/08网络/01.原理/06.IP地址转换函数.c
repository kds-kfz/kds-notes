ip地址转换函数

#include <arpa/inet.h>
int inet_pton(int af,const char* src, void* dst);
af:指定所选定的ip地址的版本AF_INET

const char* src:
char* p = "192.168.12.13",这个参数传点分十进制

void* dst:传出参数,要转换成什么类型

--------------------------------------------------------------------------------
const char* inet_ntop(int af,const void* src, char* dst, socklen_t size);
af:指定所选定的ip地址的版本AF_INET
const void* src:网络字节序
char* dst:字符串的地址
socklen_t size:字符串的大小

--------------------------------------------------------------------------------
p实际上代表的是字符串的ip(常见的点分十进制)
示例:
192.168.12.13------>unsigned int --->htonl  转换成网路字节序
调用上面的函数可以直接转换成网络字节序
192.168.12.13----------------------->网络字节序列        inet_pton
网络字节序-------------------------->点分十进制的字符串  inet_ntop
