#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include <pthread.h>
//5.有参线程回调3.c
//循环创建线程，执行各自线程的回调
//主控线程发送字符串，各自线程接收字符串后将其打印
void *thread_handle(void *arg){
    char *s=(char *)arg;
    printf("rev %s\n",s);
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
    sleep(2);
    return 0;
}
