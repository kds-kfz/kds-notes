#include<stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
//9.raise.c
#if 0
    int raise(int sig);
    sig：给自身进程发送的信号
    返回值：成功返回0,错误返回-1
#endif
void fun(int n){
    printf("rcv : %d\n",n);
}
int main(){
//    sleep(3);
    signal(2,fun);
    while(1){
    int ret=raise(2);
    if(ret==-1){
	printf("raise fail\n");
	exit(-1);
    }
    sleep(1);
    }
    return 0;
}
