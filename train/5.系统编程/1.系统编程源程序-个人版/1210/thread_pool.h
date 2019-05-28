#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H
#include<pthread.h>
//thread_pool.h

//任务结构体
typedef struct TASK{
    void *(*pfunc)(void *);
    void *argv;
    struct TASK *next;
}Task;

//线程池
typedef struct{
    int thread_num;
    int queue_cur_num;
    int queue_max_num;
    int queue_close;
    int pool_close;
    Task *head;

    pthread_t *pth;
    pthread_mutex_t mutex;
    pthread_cond_t queue_empty;
    pthread_cond_t queue_not_empty;
    pthread_cond_t queue_not_full;
}Pool;

//函数声明
extern void *thread_handle(void *arg);//线程回调函数
extern Pool *pool_init(int thread_num,int queue_max_num);//线程池初始化
extern int pool_add_task(Pool *pool,void *(*pfun)(void *arg),void *arg);//添加任务
extern int pool_destroy(Pool *pool);//销毁线程池
extern void *task(void *arg);//任务回调

#endif
