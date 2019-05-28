#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

#include "threadpool.h"

static ThreadPool *pool = NULL;

void addThreadWork(void*(*process)(void*arg),void*arg)
{
	WORKER *work = (WORKER*)malloc(sizeof(WORKER));
	work->process = process;
	work->arg = arg;
	work->next = NULL;

	if(pool->shutdown)
		return;

	pthread_mutex_lock(&(pool->mutex));
	WORKER *temp = NULL;
	if(pool->workHead != NULL){
		temp = pool->workHead;
		while(temp->next != NULL)
			temp = temp->next;
		temp->next = work;
	}else{
		pool->workHead = work;
	}
	pthread_mutex_unlock(&(pool->mutex));
	pthread_cond_signal(&(pool->cond));
}

void* threadRun(void *arg)
{
	while(1){
		sleep(1);
		pthread_mutex_lock(&(pool->mutex));
		while((pool->workHead) == NULL && !(pool->shutdown)){
			printf("thread %lu is waiting...\n",pthread_self());
			pthread_cond_wait(&(pool->cond),&(pool->mutex));	
		}

		if(pool->shutdown){
			pthread_mutex_unlock(&(pool->mutex));
			pthread_exit(NULL);
		}

		WORKER *temp = NULL;
		temp = pool->workHead;
		pool->workHead = pool->workHead->next;
		pthread_mutex_unlock(&(pool->mutex));
		(temp->process)(temp->arg);
		free(temp);
	}

	pthread_exit(NULL);
	return NULL;
}

int poolDestroy()
{
	if(pool->shutdown)
		return -1;

	pool->shutdown = 1;
	pthread_cond_broadcast(&(pool->cond));
	int i;
	for(i=0; i<pool->maxThreadNum; i++){
		printf("thread %lu is exit...\n",pthread_self());
		pthread_join(pool->threadId[i],NULL);
	}
	free(pool->threadId);

	WORKER *temp;
	while(pool->workHead != NULL){
		temp = pool->workHead;
		pool->workHead = pool->workHead->next;
		free(temp);
	}
	pthread_mutex_destroy(&(pool->mutex));
	pthread_cond_destroy(&(pool->cond));

	free(pool);
	pool = NULL;

	return 0;
}


int poolInit(int maxThreadNum)
{
	pool = (ThreadPool*)malloc(sizeof(ThreadPool));

	pthread_mutex_init(&(pool->mutex),NULL);
	pthread_cond_init(&(pool->cond),NULL);

	pool->shutdown = 0;

	pool->maxThreadNum = maxThreadNum;

	pool->threadId = (pthread_t*)malloc(maxThreadNum*sizeof(pthread_t));

	pool->workHead = NULL;

	int i;
	for(i=0; i<maxThreadNum; i++){
		sleep(1);
		printf("thread %lu create...\n",pthread_self());
		pthread_create(&(pool->threadId[i]),NULL,threadRun,NULL);
	}
}


void* process(void* arg)
{
	printf("thread %lu is runing[%d]\n",pthread_self(),(int)arg);
}


int main()
{
	poolInit(4);

	int i;
	for(i=0; i<10; i++){
		sleep(1);
		printf("thread %lu addwork[%d]\n",pthread_self(),i);
		addThreadWork(process,(void*)i);
	}

	sleep(8);
	poolDestroy();

	return 0;
}
