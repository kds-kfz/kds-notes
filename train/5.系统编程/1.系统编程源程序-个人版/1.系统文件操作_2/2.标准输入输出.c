#include<unistd.h>//read
#include<string.h>//strlen
#include<stdlib.h>//exit
#include<stdio.h>//printf
//2.标准输入输出.c
int main(){
    char buf[1024];
    //标准输入
    int ret=read(0,buf,sizeof(buf)-1);
    if(ret==-1){
	printf("read error\n");
	exit(-1);
    }
    buf[ret]='\0';
    printf("----------------\n");

    //标准输出
    ret=write(1,buf,strlen(buf));
    if(ret==-1){
	printf("write error\n");
	exit(-1);
    }
    return 0;
}
