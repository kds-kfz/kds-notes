#include<stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
//11.abort.c
#if 0
    void abort(void);
#endif
void fun(int n){
    printf("rcv : %d\n",n);
    return ;
}
int main(){
    signal(SIGABRT,fun);
    sleep(2);
    abort();
    for(;;){
	sleep(1);
    }
    return 0;
}
