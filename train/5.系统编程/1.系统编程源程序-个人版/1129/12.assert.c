#include<stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
//12.assert.c
#if 0
    void assert(scalar expression);
    expression：表达式为假，则发送信号，段错误
#endif
void fun(int n){
    printf("rcv : %d\n",n);
    return ;
}
int main(){
    signal(SIGABRT,fun);
    sleep(2);
    int i=0;
    assert(i==1);//断言
    for(;;){
	sleep(1);
    }
    return 0;
}
