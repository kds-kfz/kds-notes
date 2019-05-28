#include<stdio.h>
#include<stdlib.h>
#include<strings.h>
#include<string.h>
#include<unistd.h>
#include<errno.h>
#include"thread_pool.h"
//thread_pool.c

void *thread_handle(void *arg){
    Pool *pool=(Pool *)arg;
    Task *temp=NULL;
    while(1){
	pthread_mutex_lock(&(pool->mutex));
	while(pool->queue_cur_num==0&&!pool->pool_close)
	    pthread_cond_wait(&(pool->queue_not_empty),&(pool->mutex));
	if(pool->pool_close){
	    pthread_mutex_unlock(&(pool->mutex));
	    pthread_exit(NULL);
	}

	temp=pool->head;
	pool->queue_cur_num--;

	if(pool->queue_cur_num==0)
	    pool->head=NULL;
	else
	    pool->head=temp->next;
	if(pool->queue_cur_num==0)
	    pthread_cond_signal(&(pool->queue_empty));
	if(pool->queue_cur_num==pool->queue_max_num-1)
	    pthread_cond_broadcast(&(pool->queue_not_full));//
//	    pthread_cond_broadcast(&(pool->queue_not_empty));
	    //可能任务已全部创建好，但在创建任务期间，
	    //有太多线程在阻塞等待被唤醒，故有可能在规定的时间内无法完成任务
	pthread_mutex_unlock(&(pool->mutex));
	(temp->pfunc)(temp->argv);
	free(temp);
	temp=NULL;
    }
}

Pool *pool_init(int thread_num,int queue_max_num){
    Pool *pool=(Pool *)malloc(sizeof(Pool));
    int ret;
    if(pool==NULL){
	perror("pool malloc fail\n");
	exit(-1);
    }
    pool->pth=malloc(sizeof(pthread_t)*thread_num);
    if(pool->pth==NULL){
	perror("pth malloc fail\n");
	exit(-1);
    }
    pool->queue_max_num=queue_max_num;
    pool->thread_num=thread_num;
    pool->queue_cur_num=0;
    pool->pool_close=0;
    pool->queue_close=0;
    pool->head=NULL;
    
    if(pthread_mutex_init(&(pool->mutex),NULL)==-1){
	perror("mutex init fail\n");
	    return NULL;
    }
    if(pthread_cond_init(&(pool->queue_empty),NULL)){
	perror("queue_empty init fail\n");
	    return NULL;
    }
    if(pthread_cond_init(&(pool->queue_not_empty),NULL)){
	perror("queue_not_empty init fail\n");
	    return NULL;
    }
    if(pthread_cond_init(&(pool->queue_not_full),NULL)){
	perror("queue_not_full init fail\n");
	    return NULL;
    }

    for(int i=0;i<thread_num;i++){
	ret=pthread_create(&(pool->pth[i]),NULL,thread_handle,(void *)pool);
	if(ret!=0){
	    printf("%dth pthread_create error : %s\n",i,strerror(ret));
	    return NULL;
	}
    }
    return pool;
}
int pool_add_task(Pool *pool,void *(*pfun)(void *arg),void *arg){
    pthread_mutex_lock(&(pool->mutex));
    while(pool->queue_max_num==pool->queue_cur_num&&
	    !(pool->pool_close||pool->queue_close))
	pthread_cond_wait(&(pool->queue_not_full),&(pool->mutex));
    if(pool->pool_close||pool->queue_close){
	pthread_mutex_unlock(&(pool->mutex));
	return -1;
    }


    Task *task=(Task *)malloc(sizeof(Task));
    if(task==NULL){
	perror("task malloc fail\n");
	return -1;
    }
    task->pfunc=pfun;
    task->argv=arg;
    task->next=NULL;
    
    Task *temp=pool->head;
    if(temp==NULL)
	pool->head=task;
    else{
	while(temp->next)
	    temp=temp->next;
	temp->next=task;
    }
    pool->queue_cur_num++;
    pthread_mutex_unlock(&(pool->mutex));
    return 0;
}

int pool_destroy(Pool *pool){
#if 0
    pthread_mutex_lock(&(pool->mutex));
    if(pool->pool_close||pool->queue_close){
	pthread_mutex_unlock(&(pool->mutex));
	return -1;
    }
    while(pool->queue_cur_num!=0){
//	printf("destroy\n");
	pthread_cond_wait(&(pool->queue_empty),&(pool->mutex));
    }
#endif
#if 1
    pthread_mutex_lock(&(pool->mutex));
    if(pool->pool_close||pool->queue_close){
	pthread_mutex_unlock(&(pool->mutex));
	return -1;
    }
    while(pool->queue_cur_num!=0)//任务没执行完
	pthread_cond_wait(&(pool->queue_empty),&(pool->mutex));
#endif
    pool->pool_close=1;
    pthread_mutex_unlock(&(pool->mutex));
    pthread_cond_broadcast(&(pool->queue_not_empty));//唤醒所有阻塞等待该条件变量的线程
    pthread_cond_broadcast(&(pool->queue_not_full));//唤醒添加任务的函数
    int ret;
    for(int i=0;i<pool->thread_num;i++){
	ret=pthread_join(pool->pth[i],NULL);
	if(ret!=0){
	    printf("pthread_join %dth,error : %s\n",i,strerror(ret));
	    return -1;
	}
    }
    pthread_mutex_destroy(&(pool->mutex));
    pthread_cond_destroy(&(pool->queue_empty));
    pthread_cond_destroy(&(pool->queue_not_empty));
    pthread_cond_destroy(&(pool->queue_not_full));
    free(pool->pth);
    Task *temp=NULL;
    while(pool->head){
	temp=pool->head;
	pool->head=temp->next;
	free(temp);
    }
    free(pool);
    return 0;
}
void *task(void *arg){
    printf("worke : %ld\n",(long)arg);
    sleep(1);
}
