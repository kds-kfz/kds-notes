#include<stdio.h>
#include <arpa/inet.h>
#if 0
    uint32_t htonl(uint32_t hostlong);
    uint16_t htons(uint16_t hostshort);
    uint32_t ntohl(uint32_t netlong);
    uint16_t ntohs(uint16_t netshort);
#endif

int main(){
    unsigned int a=0x4030201;
    //测试本机器字节序：小端
    char *p=(char *)&a;
    if(*p==0x01)
	printf("little\n");
    else
	printf("big\n");
    //将小端转大端
    printf("host to net long\n");
    unsigned int b=htonl(a);
    p=(char *)&b;
    if(*p==0x04)
	printf("big\n");
    else
	printf("little\n");
    return 0;
}
