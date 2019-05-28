#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include <pthread.h>
//2.循环创建线程.c
//创5个线程，每个时间片不一定5个线程同时执行同1个任务，
//有可能是6个子线程，有一个线程执行2次
void *thread_handle(void *arg){
    for(int i=0;i<5;i++){
	printf("in thread_handle %dth pth = %lu,pid = %d\n",i,pthread_self(),getpid());
	sleep(1);
    }
    return NULL;
}
int main(){
    pthread_t pth;
    for(int i=0;i<5;i++){
	int ret=pthread_create(&pth,NULL,thread_handle,NULL);
	if(ret!=0){
	printf("pthread_creatr:%s\n",strerror(ret));
	exit(-1);
	}
    }
    for(int i=0;i<5;i++){
	printf("in main pth = %lu,pid = %d\n",pthread_self(),getpid());
	sleep(1);//在这1s里，主线程让出cpu使用权，子线程在这1s里获取时间片，
	//5个子线程不一定同时执行，
    }
    return 0;
}
