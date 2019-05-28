#include<string.h>
#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include <pthread.h>
#include <assert.h>
//10.newpool.c
//线程池
//双向链表任务队列

//队列任务结构体
typedef struct Job{
    void *(*pfunc)(void *);//指向回调函数的函数指针，指向任意参数，返回任意地址
    void *argv;//回调函数的传参，任意类型数据地址
    struct Job *next;//双向链表中指向下一节点
    struct Job *pre;//双向链表中指向上一节点
}job_t;

//线程池结构体
typedef struct{
    int thread_num;
    int queue_cur_num;
    int queue_max_num;
    int queue_close;
    int pool_close;
    pthread_t *pth;
    job_t *head;
    job_t *tail;

    pthread_mutex_t mutex;
    pthread_cond_t queue_empty;//当所有任务都被执行完
    pthread_cond_t queue_not_empty;//当队列中有任务
    pthread_cond_t queue_not_full;
    //当队列中数量达最大值-1，唤醒添加任务函数，添加任务
    //添加任务函数队列是否已满，满了就释放cpu使用权
}threadpool_t;

void *thread_handle(void *arg){
    threadpool_t *pool=(threadpool_t *)arg;
    job_t *pjob=NULL;
    while(1){
	pthread_mutex_lock(&(pool->mutex));
	//第一次条件满足，等待添加任务后，队列不是空(条件变量)
	while(pool->queue_cur_num==0&&!pool->pool_close){
	    printf("等待任务创建 cur_num = %d\n",pool->queue_cur_num);
	    printf("%lu 线程等待\n",pthread_self());
	    pthread_cond_wait(&(pool->queue_not_empty),&(pool->mutex));
	}
	printf("现有任务 = %d\n",pool->queue_cur_num);

	//有可能线程池关闭，是在所有任务都执行完后，线程池关闭，此时该线程需要退出
	if(pool->pool_close){
	    pthread_mutex_unlock(&(pool->mutex));
	    pthread_exit(NULL);
	}
	//否则执行队列里的任务
	pool->queue_cur_num--;
	printf("%lu 线程正在执行\n",pthread_self());
	printf("执行任务 剩余任务cur_num = %d\n",pool->queue_cur_num);
	pjob=pool->head;
	if(pool->queue_cur_num==0)
	    pool->head=pool->tail=NULL;
	else{
	    pool->head=pjob->next;
	    pjob->next=NULL;
	    pool->head->pre=NULL;
	}
	if(pool->queue_cur_num==0){
	    pthread_cond_signal(&(pool->queue_empty));
	    printf("signal\n");
	}
	if(pool->queue_cur_num==pool->queue_max_num-1){
//	    pthread_cond_broadcast(&(pool->queue_not_empty));//唤醒所有等待该条件的线程
	    pthread_cond_broadcast(&(pool->queue_not_full));//唤醒添加任务函数
	    printf("broadcast\n");
	}
	pthread_mutex_unlock(&(pool->mutex));//释放时间片，此时可能添加了2个任务
	(*(pjob->pfunc))(pjob->argv);
	free(pjob);
	pjob=NULL;
    }
}
//有2种情况可执行添加任务函数
//1.while里条件不满足
//2.while里条件满足，同时收到线程发来的条件变量pool->queue_not_full
int pool_add_task(threadpool_t *pool,void *(*pfunc)(void *),void *argv){
    pthread_mutex_lock(&(pool->mutex));
    //当队列中当前任务即将满时，执行了任务添加函数，此时就出现
    //pool->queue_cur_num==pool->queue_max_num
    while(pool->queue_cur_num==pool->queue_max_num&&
	    !(pool->pool_close||pool->queue_close)){
	pthread_cond_wait(&(pool->queue_not_full),&(pool->mutex));
    }
    if(pool->pool_close||pool->queue_close){
	pthread_mutex_unlock(&(pool->mutex));
	return -1;
    }
    job_t *job=(job_t *)malloc(sizeof(job_t));
    if(job==NULL){
	printf("pool_add_task fail\n");
	return -1;
    }
    job->pfunc=pfunc;
    job->argv=argv;
    job->pre=NULL;

    if(pool->head==NULL){
	job->next=NULL;
	pool->head=pool->tail=job;
    }
    else{
    job->next=pool->head;
    pool->head->pre=job;
    pool->head=job;
    }
    pool->queue_cur_num++;
    printf("已添加任务 cur_num = %d\n",pool->queue_cur_num);
    pthread_mutex_unlock(&(pool->mutex));
    return 0;
}

threadpool_t *pool_init(int thread_num,int queue_max_num){
    threadpool_t *pool=malloc(sizeof(threadpool_t));
    if(pool==NULL){
	printf("pool malloc fail\n");
	return pool;
    }
    pool->thread_num=thread_num;
    pool->queue_cur_num=0;
    pool->queue_max_num=queue_max_num;
    pool->queue_close=0;
    pool->pool_close=0;
    pool->head=NULL;
    pool->tail=NULL;
    
    int ret;
    ret=pthread_mutex_init(&(pool->mutex),NULL);
    if(ret!=0){
	printf("mutex init fail : %s\n",strerror(ret));
	return NULL;
    }
    ret=pthread_cond_init(&(pool->queue_empty),NULL);
    if(ret!=0){
	printf("queue_empty init fail : %s\n",strerror(ret));
	return NULL;
    }
    ret=pthread_cond_init(&(pool->queue_not_empty),NULL);
    if(ret!=0){
	printf("queue_not_empty init fail : %s\n",strerror(ret));
	return NULL;
    }
    ret=pthread_cond_init(&(pool->queue_not_full),NULL);
    if(ret!=0){
	printf("queue_not_full init fail : %s\n",strerror(ret));
	return NULL;
    }
    pool->pth=malloc(sizeof(pthread_t)*thread_num);
    if(pool->pth==NULL){
	printf("pool->pth malloc fail\n");
	return NULL;
    }

    for(int i=0;i<thread_num;i++){
	ret=pthread_create(&(pool->pth[i]),NULL,thread_handle,(void *)pool);
	if(ret!=0){
	    printf("pthread_create fail : %s\n",strerror(ret));
	    return NULL;
	}
    }
    return pool;
}

int pool_destroy(threadpool_t *pool){
    pthread_mutex_lock(&(pool->mutex));
    if(pool->pool_close||pool->queue_close){
	pthread_mutex_unlock(&(pool->mutex));
	return -1;
    }
    while(pool->queue_cur_num!=0){
	printf("destroy\n");
	pthread_cond_wait(&(pool->queue_empty),&(pool->mutex));
    }
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
    job_t *p=pool->head;
    while(pool->head){
	p=pool->head;
	pool->head=p->next;
	free(p);
    }
    free(pool);
    return 0;
}

void *work(void *arg){
    char *buf=(char *)arg;
    printf("work : %s\n",buf);
    sleep(1);
}

int main(){
    threadpool_t *pool=pool_init(10,20);
#if 1
    pool_add_task(pool,work,"1");
    pool_add_task(pool,work,"2");
    pool_add_task(pool,work,"3");
    pool_add_task(pool,work,"4");
    pool_add_task(pool,work,"5");
    pool_add_task(pool,work,"6");
    pool_add_task(pool,work,"7");
    pool_add_task(pool,work,"8");
    pool_add_task(pool,work,"9");
    pool_add_task(pool,work,"10");
    pool_add_task(pool,work,"11");
    pool_add_task(pool,work,"12");
    pool_add_task(pool,work,"13");
    pool_add_task(pool,work,"14");
    pool_add_task(pool,work,"15");
    pool_add_task(pool,work,"16");
    pool_add_task(pool,work,"17");
    pool_add_task(pool,work,"18");
    pool_add_task(pool,work,"19");
    pool_add_task(pool,work,"20");
    printf("cur_num : %d ,max_num = %d\n",pool->queue_cur_num,pool->queue_max_num);

    sleep(2);
    pool_destroy(pool);
//    if(pool_destroy(pool)==0)
//	printf("free ok\n");
#endif
    /*
    for(;;){
	pause();
    }
    */
    return 0;
}
