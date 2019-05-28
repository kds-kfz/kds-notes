#include<stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
//10.alarm.c
#if 0
    unsigned int alarm(unsigned int seconds);
    指定seconds秒后，给进程本身发送一个SIGALRM信号
    seconds：设置秒值
    返回值：0,或者以前设置的闹钟时间的余留秒数
#endif
void fun(int n){
    printf("rcv : %d\n",n);
    alarm(1);
    return ;
}
int main(){
    signal(SIGALRM,fun);
    printf("alarm before 5s\n");
    alarm(5);
    for(;;){
	sleep(1);
    }
    return 0;
}
