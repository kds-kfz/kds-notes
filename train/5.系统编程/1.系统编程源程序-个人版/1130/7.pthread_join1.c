#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include <pthread.h>
//7.pthread_join1.c
#if 0
    int pthread_join(pthread_t thread, void **retval);
    thread：线程的id
    retval：
    返回值：成功返回0,否则返回错误编号
    
    创建1个线程，主控线程发送字符串，
    线程结束并将其打印,线程返回资源(整型数据),
    主控线程阻塞等待接收，当接收成功后将其打印
#endif
void *thread_handle(void *arg){
    int num=99;
    char *s=(char *)arg;
    printf("pth = %lu,pid = %d,rev %s\n",pthread_self(),getpid(),s);
    pthread_exit((void *)(long)num);
}
int main(){
    pthread_t pth;
    char buf[]="hello world";
    char *p="how are you";
//    void *status;
    long status;
    int ret=pthread_create(&pth,NULL,thread_handle,(void *)p);
    if(ret!=0){
	printf("pthread_creatr:%s\n",strerror(ret));
	exit(-1);
    }
    pthread_join(pth,(void **)&status);
    printf("num = %ld\n",(long)status);
    return 0;
}
