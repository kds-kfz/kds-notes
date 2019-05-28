#include<string.h>
#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include <pthread.h>
#include <assert.h>
//8,pool.c
//网上参考
struct job{
    void* (*callback_function)(void *arg);    //线程回调函数
    void *arg;                                //回调函数参数
    struct job *next;
};
struct threadpool{
    int thread_num;                   //线程池中开启线程的个数
    int queue_max_num;                //队列中最大job的个数
    struct job *head;                 //指向job的头指针
    struct job *tail;                 //指向job的尾指针
    pthread_t *pthreads;              //线程池中所有线程的pthread_t
    pthread_mutex_t mutex;            //互斥信号量
    pthread_cond_t queue_empty;       //队列为空的条件变量
    pthread_cond_t queue_not_empty;   //队列不为空的条件变量
    pthread_cond_t queue_not_full;    //队列不为满的条件变量
    int queue_cur_num;                //队列当前的job个数
    int queue_close;                  //队列是否已经关闭
    int pool_close;                   //线程池是否已经关闭
};

int threadpool_add_job(struct threadpool* pool, 
	void*(*callback_function)(void *arg),void *arg);
void* threadpool_function(void* arg);
int threadpool_destroy(struct threadpool *pool);
void* work(void* arg);

struct threadpool* threadpool_init(int thread_num, int queue_max_num){
    struct threadpool *pool = NULL;
    do{
	pool = malloc(sizeof(struct threadpool));
	if (NULL == pool){
	    printf("failed to malloc threadpool!\n");
	    break;
	}
	pool->thread_num = thread_num;
	pool->queue_max_num = queue_max_num;
	pool->queue_cur_num = 0;
	pool->head = NULL;
	pool->tail = NULL;
	if (pthread_mutex_init(&(pool->mutex), NULL)){
	    printf("failed to init mutex!\n");
	    break;
	}
	if (pthread_cond_init(&(pool->queue_empty), NULL)){
	    printf("failed to init queue_empty!\n");
	    break;
	}
	if (pthread_cond_init(&(pool->queue_not_empty), NULL)){
	    printf("failed to init queue_not_empty!\n");
	    break;
	}
	if (pthread_cond_init(&(pool->queue_not_full), NULL)){
	    printf("failed to init queue_not_full!\n");
	    break;
	}
	pool->pthreads = malloc(sizeof(pthread_t) * thread_num);
	if (NULL == pool->pthreads){
	    printf("failed to malloc pthreads!\n");
	    break;
	}
	pool->queue_close = 0;
	pool->pool_close = 0;
	int i;
	for (i = 0; i < pool->thread_num; ++i){
	    pthread_create(&(pool->pthreads[i]),NULL,threadpool_function,(void *)pool);
	}
	return pool;    
    }while (0);
    return NULL;
}
int threadpool_add_job(struct threadpool* pool, 
	void*(*callback_function)(void *arg),void *arg){
    assert(pool != NULL);
    assert(callback_function != NULL);
    assert(arg != NULL);
    pthread_mutex_lock(&(pool->mutex));
    while((pool->queue_cur_num == pool->queue_max_num) 
	    && !(pool->queue_close || pool->pool_close)){
	pthread_cond_wait(&(pool->queue_not_full),&(pool->mutex));//队列满的时候等待    
    }
    if(pool->queue_close || pool->pool_close){//队列关闭或者线程池关闭就退出    
	pthread_mutex_unlock(&(pool->mutex));
	return -1;
    }
    struct job *pjob =(struct job*) malloc(sizeof(struct job));
    if (NULL == pjob){
	pthread_mutex_unlock(&(pool->mutex));
	return -1;
    } 
    pjob->callback_function = callback_function;    
    pjob->arg = arg;
    pjob->next = NULL;
    if (pool->head == NULL){
	pool->head = pool->tail = pjob;
	pthread_cond_broadcast(&(pool->queue_not_empty));  
	//队列空的时候，有任务来时就通知线程池中的线程：队列非空    
    }
    else{
	pool->tail->next = pjob;
	pool->tail = pjob;    
    }
    pool->queue_cur_num++;
    pthread_mutex_unlock(&(pool->mutex));
    return 0;
}
void* threadpool_function(void* arg){
    struct threadpool *pool = (struct threadpool*)arg;
    struct job *pjob = NULL;
    while (1){  //死循环    
	pthread_mutex_lock(&(pool->mutex));
	while ((pool->queue_cur_num == 0) && !pool->pool_close){
	    //队列为空时，就等待队列非空        
	    pthread_cond_wait(&(pool->queue_not_empty), &(pool->mutex));
	}
	if (pool->pool_close){   //线程池关闭，线程就退出
	    pthread_mutex_unlock(&(pool->mutex));
	    pthread_exit(NULL);
	}
	pool->queue_cur_num--;
	pjob = pool->head;
	if (pool->queue_cur_num == 0){
	    pool->head = pool->tail = NULL;
	}
	else {
	    pool->head = pjob->next;
	}
	if (pool->queue_cur_num == 0){
	    pthread_cond_signal(&(pool->queue_empty));        
	    //队列为空，就可以通知threadpool_destroy函数，销毁线程函数        
	}
	if (pool->queue_cur_num == pool->queue_max_num - 1){
	    pthread_cond_broadcast(&(pool->queue_not_full));  
	    //队列非满，就可以通知threadpool_add_job函数，添加新任务        
	}
	pthread_mutex_unlock(&(pool->mutex));
	(*(pjob->callback_function))(pjob->arg);   
	//线程真正要做的工作，回调函数的调用        
	free(pjob);
	pjob = NULL;    
    }
}
int threadpool_destroy(struct threadpool *pool){
    assert(pool != NULL);
    pthread_mutex_lock(&(pool->mutex));
    if (pool->queue_close || pool->pool_close){   //线程池已经退出了，就直接返回
	pthread_mutex_unlock(&(pool->mutex));
	return -1;
    }
    pool->queue_close = 1;        //置队列关闭标志
    while (pool->queue_cur_num != 0){
	pthread_cond_wait(&(pool->queue_empty), &(pool->mutex));  //等待队列为空    
    }    
    pool->pool_close = 1;      //置线程池关闭标志
    pthread_mutex_unlock(&(pool->mutex));
    pthread_cond_broadcast(&(pool->queue_not_empty));  
    //唤醒线程池中正在阻塞的线程
    pthread_cond_broadcast(&(pool->queue_not_full));   
    //唤醒添加任务的threadpool_add_job函数
    int i;
    for (i = 0; i < pool->thread_num; ++i){
	pthread_join(pool->pthreads[i], NULL);    //等待线程池的所有线程执行完毕    
    }
    pthread_mutex_destroy(&(pool->mutex));          //清理资源
    pthread_cond_destroy(&(pool->queue_empty));
    pthread_cond_destroy(&(pool->queue_not_empty));   
    pthread_cond_destroy(&(pool->queue_not_full));    
    free(pool->pthreads);
    struct job *p;
    while (pool->head != NULL){
	p = pool->head;
	pool->head = p->next;
	free(p);
    }
    free(pool);
    return 0;
}
void* work(void* arg){
    char *p = (char*) arg;
    printf("threadpool callback fuction : %s.\n", p);
    sleep(1);
}

int main(){
    struct threadpool *pool = threadpool_init(10, 20);
    threadpool_add_job(pool, work, "1");
    threadpool_add_job(pool, work, "2");
    threadpool_add_job(pool, work, "3");
    threadpool_add_job(pool, work, "4");
    threadpool_add_job(pool, work, "5");
    threadpool_add_job(pool, work, "6");
    threadpool_add_job(pool, work, "7");
    threadpool_add_job(pool, work, "8");
    threadpool_add_job(pool, work, "9");
    threadpool_add_job(pool, work, "10");
    threadpool_add_job(pool, work, "11");
    threadpool_add_job(pool, work, "12");
    threadpool_add_job(pool, work, "13");
    threadpool_add_job(pool, work, "14");
    threadpool_add_job(pool, work, "15");
    threadpool_add_job(pool, work, "16");
    threadpool_add_job(pool, work, "17");
    threadpool_add_job(pool, work, "18");
    threadpool_add_job(pool, work, "19");
    threadpool_add_job(pool, work, "20");
    sleep(5);
    threadpool_destroy(pool);
    return 0;
}
