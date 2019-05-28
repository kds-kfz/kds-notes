#ifndef __POOL_H
#define __POOL_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>

typedef struct worker
{
	void *(*pfunc)(void*);	//回调函数，真正的执行函数的函数指针
	void *argv;				//回调函数的入参
	struct worker *next;	
}worker_t;

typedef struct 
{
	int number;				//线程池中线程的个数
	int shutdown;			//线程池销毁的标志
	worker_t *head;			//“产品”链表的头指针
	pthread_t *pth;			//保存线程标识符
	pthread_mutex_t mutex;
	pthread_cond_t cond;
}pool_t;

int pool_destroy();
int pool_add_worker(void *(*pfunc)(void*), void *argv, size_t size);
int pool_init(int number);
#endif
