#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<errno.h>
#include<string.h>
#include"thread_pool.hpp"

#include<iostream>
using namespace std;

int pool_init(int thread_num,int queue_max_num){//线程池初始化
    int ret;
    pool=NULL;
    pool=(pool_t *)malloc(sizeof(pool_t));
    if(pool==NULL){
        perror("pool malloc error\n");
        exit(-1);
    }

    if(pthread_mutex_init(&(pool->mutex),NULL)){
        perror("mutex init error\n");
        return -1;
    }
    if(pthread_cond_init(&(pool->cond),NULL)){
        perror("cond init error\n");
        return -1;
    }
    if(pthread_cond_init(&(pool->queue_empty),NULL)){
        perror("queue_empty init error\n");
        return -1;
    }
    if(pthread_cond_init(&(pool->queue_not_full),NULL)){
        perror("queue_full init error\n");
        return -1;
    }

    pool->head=NULL;
    pool->thread_num=thread_num;
    pool->queue_max_num=queue_max_num;
    pool->queue_num=0;
    pool->pool_close=0;

    pool->pth=(pthread_t *)malloc(sizeof(pthread_t)*thread_num);
    if(pool->pth==NULL){
        printf("pth malloc error\n");
        exit(-1);
    }

    for(int i=0;i<thread_num;i++){
        ret=pthread_create(&(pool->pth[i]),NULL,thread_handle,NULL);
        if(ret!=0){
            printf("pthread_create[%d] error %s\n",i,strerror(ret));
            return -1;
        }
    }
    return 0;
}

int pool_add_task(void *(*pfunc)(void *arg),void *arg){
    pthread_mutex_lock(&(pool->mutex));
    while(pool->queue_num==pool->queue_max_num&&!pool->pool_close)
        pthread_cond_wait(&(pool->queue_not_full),&(pool->mutex));
    if(pool->pool_close){
        pthread_mutex_unlock(&(pool->mutex));
        perror("pool close\n");
        return -1;
    }

    job_t *pjob=(job_t *)malloc(sizeof(job_t));
    if(pjob==NULL){
        perror("task malloc error\n");
        return -1;
    }

    pjob->pfunc=pfunc;
    pjob->arg=arg;
    pjob->next=NULL;

    pjob->next=pool->head;
    pool->head=pjob;

    pool->queue_num++;
    pthread_mutex_unlock(&(pool->mutex));
    pthread_cond_signal(&(pool->cond));

    return 0;
}

int pool_destroy(pool_t *pool){

    pthread_mutex_lock(&(pool->mutex));
    while(pool->queue_num!=0)
        pthread_cond_wait(&(pool->queue_empty),&(pool->mutex));
    printf("aaaaaaaaaaaaaaaaaaa   %d\n", pool->queue_num);
    if(pool->pool_close){
        pthread_mutex_unlock(&(pool->mutex));
        return -1;
    }

    pool->pool_close=1;
    pthread_mutex_unlock(&(pool->mutex));

    pthread_cond_broadcast(&(pool->cond));
    pthread_cond_broadcast(&(pool->queue_empty));
    pthread_cond_broadcast(&(pool->queue_not_full));

    for(int i=0;i<pool->thread_num;i++)
        pthread_join(pool->pth[i],NULL);

    free(pool->pth);

    job_t *temp=NULL;
    while(pool->head){
        temp=pool->head;
        pool->head=temp->next;
        free(temp);
    }

    pthread_mutex_destroy(&(pool->mutex));
    pthread_cond_destroy(&(pool->cond));
    pthread_cond_destroy(&(pool->queue_empty));
    pthread_cond_destroy(&(pool->queue_not_full));

    free(pool);
    return 0;
}

void *thread_handle(void *arg){
    while(1){
        pthread_mutex_lock(&(pool->mutex));
        while(pool->queue_num==0&&!pool->pool_close)
            pthread_cond_wait(&(pool->cond),&(pool->mutex));

        if(pool->pool_close){
            pthread_mutex_unlock(&(pool->mutex));
            pthread_exit(NULL);
        }

        pool->queue_num--;
        job_t *pjob=pool->head;
        if(pool->queue_num==0)
            pool->head=NULL;
        else
            pool->head=pjob->next;

        if(pool->queue_num==pool->queue_max_num-1)
            pthread_cond_signal(&(pool->queue_not_full));

        if(pool->queue_num==0)
            pthread_cond_signal(&(pool->queue_empty));


        pthread_mutex_unlock(&(pool->mutex));
        (pjob->pfunc)(pjob->arg);
        free(pjob);
        pjob=NULL;
    }
}

void *task(void *arg){
    printf("tasking[%ld] ...\n",(long)arg);
    sleep(1);
    return NULL;
}
#if 0
//测试
int main(){
    int ret;
    if((ret=pool_init(10,20))!=0){
        perror("pool init fail\n");
        exit(-1);
    }
    for(int i=0;i<50;i++){
        ret=pool_add_task(task,(void *)(long)i);
        if(ret!=0){
            printf("add task[%d] fail\n",i);
            exit(-1);
        }
    }
    //    sleep(1);
    if(pool_destroy(pool)!=0)
        perror("pool destroy fail\n");
    else
        printf("pool destroy ok\n");

    return 0;
}
#endif
