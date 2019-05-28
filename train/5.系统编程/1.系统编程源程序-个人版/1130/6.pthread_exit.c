#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include <pthread.h>
//6.pthread_exit.c
#if 0
    void pthread_exit(void *retval);
    retval：是一个无类型指针，与传给启动例程的单个参数类似
    *********************************************************************
    结束进程函数：exit
    结束线程方式有3种：
    1.线程可以简单的从启动例程中返回，返回值是线程的退出码
    2.线程可以被同一进程中的其它线程取消
    3.线程调用pthread_exit
    4.return 与pthread_exit类似
    循环创建多个线程，主线程向线程发送字符串，
    各自线程接收后将字符串打印，并结束线程
#endif
void *thread_handle(void *arg){
    char *s=(char *)arg;
    printf("pth = %lu,rev %s\n",pthread_self(),s);
//    pthread_exit(NULL);//结束子线程，不会影响其它子线程
    exit(-1);//结束进程,最多只执行1个线程后，进程结束
    return NULL;
}
int main(){
    pthread_t pth;
    char buf[]="hello world";
    char *p="how are you";
    for(int i=0;i<5;i++){
	int ret=pthread_create(&pth,NULL,thread_handle,(void *)p);
	if(ret!=0){
	printf("pthread_creatr:%s\n",strerror(ret));
	exit(-1);
	}
    }
    printf("man end ...\n");
    exit(-1);//结束进程，在此之前非常短暂的时间片，要是哪个线程抢到就执行哪个线程
    pthread_exit(NULL);//结束主控线程，不会影响子线程
    return 0;
}
