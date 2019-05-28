#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
/*
uint32_t htonl(uint32_t hostlong);

uint16_t htons(uint16_t hostshort);

uint32_t ntohl(uint32_t netlong);

uint16_t ntohs(uint16_t netshort);
*/
int main()
{
	unsigned int a = 0x01020304;
	unsigned int b = htonl(a);

	char *p = (char*)&b;

	if(*p == 0x01){
		printf("big...\n");
	}else{
		printf("little...\n");
	}
}
