/*
	低地址存低字节,高地址存高字节是小端字节序
	低地址存高字节,高地址存低字节是大端字节序
	
	内存中的多字节数据相对于内存地址有大端和小端之分,磁盘文件的多字节数据相对于文件中的偏移地址
	也有大端和小端之分.网络数据流同样有大端和小端之分,发送主机通常将发送缓冲区的数据按内存地址
	从低到高的顺序发出,接收主机把从网络上接收到的字节依次保存在接收缓冲区中,也是按内存地址从低到
	高的顺序保存,因此,网络数据流的地址这样规定:先发出的数据是低地址,后发出的数据是高地址

*/
#include <stdio.h>
#include <arpa/inet.h>

/*
int main()
{	
	short tmp = 0x1234;
	tmp = htons(tmp);
	if((*(char*)&tmp) == 0x34){
		printf("It's little\n");
	}else if((*(char*)&tmp) == 0x12){
		printf("It's big\n");
	}

	return 0;
}
*/
int main()
{	
	unsigned int i = 0x01020304;
	unsigned char *p;
	p = (unsigned char*)&i;
	if(*p == 0x01)
		printf("it's big\n");
	else
		printf("it's little\n");

	return 0;
}
