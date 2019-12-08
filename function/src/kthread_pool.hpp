#ifndef _KTHREAD_POOL_H_
#define _KTHREAD_POOL_H_

#include<pthread.h>
#include<map>
#include<vector>

/************************************* 标 签 *************************************
 * 作者：kfz
 * 日期：2019-12-08
 * 描述：封装线程池类, 动态线程池
 * 简介: 大多数的网络服务器，包括 Web服务器都具有一个特点，就是单位时间内必须处理数目巨大的连接请求，但是处理时间却是比较短的。
 * 在传统的多线程服务器模型中是这样实现的：一旦有个请求到达，就创建一个新的线程，由该线程执行任务，任务执行完毕之后，线程就退出。
 * 这就是"即时创建，即时销毁"的策略。尽管与创建进程相比，创建线程的时间已经大大的缩短，但是如果提交给线程的任务是执行时间较短，
 * 而且执行次数非常频繁，那么服务器就将处于一个不停的创建线程和销毁线程的状态。这笔开销是不可忽略的，尤其是线程执行的时间非常
 * 非常短的情况。
 * 线程池就是为了解决上述问题的，它的实现原理是这样的：在应用程序启动之后，就马上创建一定数量的线程，放入空闲的队列中。这些线程
 * 都是处于阻塞状态，这些线程只占一点内存，不占用 CPU。当任务到来后，线程池将选择一个空闲的线程，将任务传入此线程中运行。当所有的
 * 线程都处在处理任务的时候，线程池将自动创建一定的数量的新线程，用于处理更多的任务。执行任务完成之后线程并不退出，而是继续在线程池中
 * 等待下一次任务。当大部分线程处于阻塞状态时，线程池将自动销毁一部分的线程，回收系统资源。
 * 下面是一个简单线程池的实现，这个线程池的代码是我参考网上的一个例子实现的，由于找不到出处了，就没办法注明参考自哪里了。
 * 它的方案是这样的：程序启动之前，初始化线程池，启动线程池中的线程，由于还没有任务到来，线程池中的所有线程都处在阻塞状态，当一有任务
 * 到达就从线程池中取出一个空闲线程处理，如果所有的线程都处于工作状态，就添加到队列，进行排队。如果队列中的任务个数大于队列的所能容纳
 * 的最大数量，那就不能添加任务到队列中，只能等待队列不满才能添加任务到队列中。
 **********************************************************************************/

//PTHREAD_MODE: 
//1: c 风格任务单链表
//2: c++ 风格
#define PTHREAD_MODE 2

#if PTHREAD_MODE == 1
typedef struct Job{
    void *(*pfunc)(void *arg);      //函数指针
    void *arg;		                //任务函数参数
    struct Job *next;	            //任务节点
}job_t;
#elif PTHREAD_MODE == 2
//函数指针，入参任意指针类型的地址，返回值是任意指针类型地址
//设计理念：查找函数地址，没有则该功能尚未实现；有的情况，每次任务参数可以不一样
typedef  void *(*pFunc)(void *);
//函数指针 + 任务节点
typedef std::map< pFunc, void * > fParam;
#endif

typedef struct{
    unsigned int thread_num;	    //线程数量
    unsigned int queue_max_num;	    //队列最大数量
    bool pool_close;	            //线程池关闭标准位

#if PTHREAD_MODE == 1
    unsigned int queue_num;	        //队列数量
    job_t *head;	                //任务头节点
#elif PTHREAD_MODE == 2
    std::vector<fParam> task_node;  //任务节点容器, 容器大小即队列数量
#endif

    pthread_t *pth;	                //线程id

    pthread_mutex_t mutex;          //互斥变量
    pthread_cond_t cond;            //条件变量
    pthread_cond_t queue_not_full;	//条件变量，判断队列是否达最大值
    pthread_cond_t queue_empty;	    //条件变量，判断是否已经执行完队列所有任务
}pool_t;

class KPTHREADPOOL{
private:
public:
    /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 线程池 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
    static pool_t *g_tphread_pool;                                      //线程池信息

    KPTHREADPOOL();                                                     //无参构造
    ~KPTHREADPOOL();                                                    //没有继承，这里不需要虚析构, 销毁线程池

    bool pool_init(unsigned int thread_num, unsigned int queue_max_num);//线程初始化
#if PTHREAD_MODE == 1
    bool pool_add_task(void *(*pfunc)(void *arg), void *arg);           //向线程池投递任务
#elif PTHREAD_MODE == 2
    bool pool_add_task(pFunc pf, void *arg);                            //向线程池投递任务
#endif
    bool pool_destroy();
};
extern void *thread_handle(void *);                                     //线程回调, 维护各个线程工作

#endif/* _KTHREAD_POOL_H_ */
