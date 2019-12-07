#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

typedef struct Job{
    void *(*pfunc)(void *arg); //函数指针
    void *arg;		    //任务函数参数
    struct Job *next;	    //任务节点
}job_t;

typedef struct{
    int thread_num;	    //线程数量
    int queue_num;	    //队列数量
    int queue_max_num;	    //队列最大数量
    int pool_close;	    //线程池关闭标准位

    job_t *head;	    //任务头节点
    pthread_t *pth;	    //线程id

    pthread_mutex_t mutex;  //互斥变量
    pthread_cond_t cond;    //条件变量
    pthread_cond_t queue_not_full;	    //条件变量，判断队列是否达最大值
    pthread_cond_t queue_empty;	    //条件变量，判断是否已经执行完队列所有任务
}pool_t;

static pool_t *pool;//初始化任务头节点

//函数声明
extern int pool_add_task(void *(*pfunc)(void *arg),void *arg);//线程池添加任务
extern void *thread_handle(void *);//线程回调
extern int pool_init(int thread_numnt,int queue_max_num);//线程池初始化
extern int pool_destroy();//释放线程池
extern void *task(void *arg);//任务回调


#endif 
