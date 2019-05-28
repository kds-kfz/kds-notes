#include<unistd.h>
#include<sys/types.h>
#include<stdio.h>
//5.truncate.c
#if 0
    //文件截断
    int truncate(const char *path, off_t length);
    参数1：文件路径，参数2：文件字节截断大小
    如，原文件有"helloword",共计11字节(加上回车，打开文件时末尾是\n)，
    截断字节5后，文件只剩下5字节，hello
    
    返回值：0成功，-1失败
#endif
int main(){

    int ret=truncate("access.txt",5);
    if(ret==-1)
	printf("fail\n");
    return 0;
}

