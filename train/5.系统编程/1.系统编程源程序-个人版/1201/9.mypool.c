#include<string.h>
#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include <pthread.h>
#include <assert.h>
//9.mypool.c
#if 0
	大多数的网络服务器，包括Web服务器都具有一个特点，就是单位时间内必须处理数
    目巨大的连接请求，但是处理时间却是比较短的。在传统的多线程服务器模型中是这样
    实现的：一旦有个请求到达，就创建一个新的线程，由该线程执行任务，任务执行完毕
    之后，线程就退出。这就是"即时创建，即时销毁"的策略。尽管与创建进程相比，
    创建线程的时间已经大大的缩短，但是如果提交给线程的任务是执行时间较短，
    而且执行次数非常频繁，那么服务器就将处于一个不停的创建线程和销毁线程的状态。
    这笔开销是不可忽略的，尤其是线程执行的时间非常非常短的情况。
    ----------------------------------------------------------------------------
	线程池就是为了解决上述问题的，它的实现原理是这样的：在应用程序启动之后，
    就马上创建一定数量的线程，放入空闲的队列中。这些线程都是处于阻塞状态，这些
    线程只占一点内存,不占用CPU。当任务到来后，线程池将选择一个空闲的线程，将任
    务传入此线程中运行。当所有的线程都处在处理任务的时候，线程池将自动创建一定
    的数量的新线程，用于处理更多的任务。执行任务完成之后线程并不退出，而是继续
    在线程池中等待下一次任务。当大部分线程处于阻塞状态时，线程池将自动销毁一部
    分的线程，回收系统资源。
    ----------------------------------------------------------------------------
	它的方案是这样的：程序启动之前，初始化线程池，启动线程池中的线程，由于
    还没有任务到来，线程池中的所有线程都处在阻塞状态，当一有任务到达就从线程池
    中取出一个空闲线程处理，如果所有的线程都处于工作状态，就添加到队列，进行排队
    如果队列中的任务个数大于队列的所能容纳的最大数量，那就不能添加任务到队列中，
    只能等待队列不满才能添加任务到队列中。
#endif
#if 0
    创建10个线程，可能有部分线程被唤醒，剩下的部分线程处于阻塞等待
    在处理任务中，被唤醒的线程中有可能都在工做，
    当队列中待处理的任务小于被唤醒的任务时，在唤醒的线程中，
    有部分是工作，有部分空闲

    pthread_cond_wait
    有两种情况可以执行以后代码
    1.判断条件不满足时
    2.收到所等待的条件变量的信号
    否则就让出时间片    
    分析过程：
    1.初始化线程池
	1.thread_num = 0;
	2.head = NULL;
	3.申请thread_num个连续空间存储线程id，返回空间首地址保存在pth
	4.初始化互斥变量mutex
	(用于线程持续占用cpu权限，直到解锁，才让出时间片)
	5.初始化条件变量queue_empty
	(当所有开启的线程都在工作或是所有线程开启，有部分线程在工做，
	 有部分线程处于空闲，同时队列中已经没有任务时，
	 让出时间片给添加任务的函数，说明此时所有任务以被线程处理完或是正在被处理，
	 队列带处理的任务为0时)
	6.初始化条件变量queue_not_empty
	(在队列中待处理的任务个数即将到达最大队列数时，
	 让出时间片，给其它线程或是添加任务函数)
	7.初始化条件变量queue_not_full
	()
	8.queue_max_num = 指定的最大队列数
	(当所有启动线程都处于工作状态时，向作业队列添加任务的最大值)
	9.queue_cur_num = 0;表示开启
	10.queue_close = 0;表示开启
	11.pool_close = 0;表示开启
#endif
void *work(void *arg);

//作业链表结构体
typedef struct worker{
    void*(*pfunc)(void *);//回调函数,真正的执行函数的函数指针
    void* argv;//回调函数的入参
    struct worker* next;
}worker_t;

//线程池
typedef struct{
    int thread_num;		    //线程池中开启线程的个数
    int queue_max_num;		    //队列中最大作业的个数
    worker_t *head;		    //任务队列的表头
    pthread_t *pth;		    //线程池所有线程pth
    pthread_mutex_t mutex;	    //互斥信号量

    pthread_cond_t queue_empty;	    //队列为空条件变量
    pthread_cond_t queue_not_empty; //队列不为空条件变量
    pthread_cond_t queue_not_full;  //队列不为满条件变量
    
    int queue_cur_num;	//当前队列数量
    int queue_close;	//队列是否已经关闭
    int pool_close;	//线程池是否已经关闭
}pool_t;

void *thread_control(void *arg){
    pool_t *pool=(pool_t *)arg;
    worker_t *w=NULL;
    while(1){
	pthread_mutex_lock(&(pool->mutex));//加锁
	//如果队列没有任务，则阻塞等待条件满足，让出时间片
	//如果条件不满足则执行while后面代码
	while((pool->queue_cur_num==0)&&!pool->pool_close)
	    pthread_cond_wait(&(pool->queue_not_empty),&(pool->mutex));
	if(pool->pool_close){//如果线程池关闭
	    pthread_mutex_unlock(&(pool->mutex));//解锁，让出时间片
	    pthread_exit(NULL);//退出该线程
	}
	pool->queue_cur_num--;//队列任务减1
	w=pool->head;//保存队列任务链表头
	if(pool->queue_cur_num==0)//如果当前队列没有任务
	    pool->head=NULL;
	else
	    pool->head=w->next;//队列链表头向下移动
	if(pool->queue_cur_num==0)//如果当前队列没有任务
	    pthread_cond_signal(&(pool->queue_empty));
	//此时发送队列空信号，给pool_destroy函数，回收该线程
	//当前队列即将到最大队列数时，让出时间片，此时可以再添加1个任务到队列，
	//当当前队列数到达最大值时，执行后面代码
	if(pool->queue_cur_num==pool->queue_max_num-1)
	    pthread_cond_broadcast(&(pool->queue_not_full));
	//队列没有满时，就可以通知pool_add_worker函数，添加新任务
	pthread_mutex_unlock(&(pool->mutex));//解锁，让出时间片
	(*(w->pfunc))(w->argv);
	//线程执行回调函数
	free(w);
	w=NULL;
    }
}
void *work(void *arg){
    char *buf=(char *)arg;
    printf("work : %s\n",buf);
    sleep(1);
}

pool_t * pool_init(int thread_num,int queue_max_num){
    //线程池初始化,里面要创建多少个线程
    pool_t *pool=NULL;
    pool=malloc(sizeof(pool_t));//申请一个线程池
    if(pool==NULL){
	printf("malloc pool fail\n");
	return NULL;
    }
    //初始化线程池
    pool->thread_num=thread_num;//开启线程个数
    pool->queue_max_num=queue_max_num;//队列中最大作业个数
    pool->queue_cur_num=0;//当前队列数
    pool->head=NULL;//队列头置空
    if(pthread_mutex_init(&pool->mutex,NULL)){//初始化互斥变量
	printf("init mutex fail\n");
	return NULL;
    }
    if(pthread_cond_init(&pool->queue_empty,NULL)){//初始化条件变量queue_empty
	printf("init queue_empty fail\n");
	return NULL;
    }
    if(pthread_cond_init(&pool->queue_not_empty,NULL)){//初始化条件变量queue_not_empty
	printf("init queue_not_empty fail\n");
	return NULL;
    }
    if(pthread_cond_init(&pool->queue_not_full,NULL)){//初始化条件变量queue_not_full
	printf("init queue_not_full fail\n");
	return NULL;
    }
    pool->pth=malloc(sizeof(pthread_t)*thread_num);
    //申请thread_num个大小的空间存储线程pth
    if(pool->pth==NULL){
	printf("malloc pth fail\n");
	return NULL;
    }
    pool->queue_close=0;//初始化队列标志位
    pool->pool_close=0;//初始化线程池标志位
    for(int i=0;i<pool->thread_num;i++)//循环创建线程
	pthread_create(&(pool->pth[i]),NULL,thread_control,(void *)pool);
    //开启线程，主线程向线程传递线程池地址
    return pool;
}

int pool_add_worker(pool_t *pool,void*(*pfunc)(void*),void* arg){
    //向任务队列里添加任务
    assert(pool!=NULL);//条件为假，即线程池申请失败时，才触发断言
    assert(pfunc!=NULL);//条件为假，即线程池申请失败时，才触发断言
    assert(arg!=NULL);//条件为假，即线程池申请失败时，才触发断言
    //阻塞等待条件满足，若条件不满足则执行while后面代码，否则让出时间片给其它线程
    //当前队列数已经到大最大队列数，同时队列和线程池都没有关闭时,
    //让出时间片，这时才执行线程回调函数,在线程回调中有1s时间，让出时间片，
    //如果是pool_add_worker抢到该时间片，此时该条件不满足，则执行添加任务到队列的代码
    //添加任务到队列的前提是，启动的所有线程都处于工作状态，将任务添加到队列，
    //以待空闲线程处理
    //如果是线程回调thread_control抢到时间片，则处理队列任务减1,任务减少，指针向下移，
    //然后再让出时间片，给其它线程或是pool_add_worker添加任务函数，
    //如此循环，直到所有任务被处理结束，
    //即所有启动线程都处于阻塞等待状态，或是空闲状态
    pthread_mutex_lock(&(pool->mutex));//加锁
    while((pool->queue_cur_num==pool->queue_max_num)&&
	!(pool->queue_close||pool->pool_close))
	pthread_cond_wait(&(pool->queue_not_full),&(pool->mutex));
    if(pool->pool_close||pool->queue_close){//如果线程池关闭或队列关闭
	pthread_mutex_unlock(&(pool->mutex));//加锁，让出时间片
	return -1;
    }
    worker_t *w=(worker_t *)malloc(sizeof(worker_t));
    if(w==NULL){//作业任务申请失败
	pthread_mutex_unlock(&(pool->mutex));//解锁，让出时间片
	return -1;
    }
    w->pfunc=pfunc;//传进来的回调函数地址
    w->argv=arg;//回调函数参数
    w->next=pool->head;//单链表头创建
    pool->head=w;

    pool->queue_cur_num++;
    pthread_mutex_unlock(&(pool->mutex));//解锁
    return 0;
}
int pool_destroy(pool_t *pool){//销毁线程池  
    assert(pool!=NULL);//如果线程池地址是空，则条件为假，触发断言
    pthread_mutex_lock(&(pool->mutex));//加锁
    if(pool->queue_close||pool->pool_close){//如果队列关闭或是线程池关闭
	pthread_mutex_unlock(&(pool->mutex));//解锁
	return -1;
    }
    while(pool->queue_cur_num!=0)//如果当前队列还有任务
	pthread_cond_wait(&(pool->queue_empty),&(pool->mutex));
    //等待队列是空的条件变量，阻塞等待该条件变量，让出时间片
    pool->pool_close=1;//此时队列已经没有任务，将线程池关闭
    pthread_mutex_unlock(&(pool->mutex));//解锁
    pthread_cond_broadcast(&(pool->queue_not_empty));
    //唤醒线程池中等待该条件变量的所有阻塞线程
    pthread_cond_broadcast(&(pool->queue_not_full));
    //唤醒添加任务的pool_add_worker函数
    for(int i=0;i<pool->thread_num;i++)//阻塞回收线程池中已启用所有的线程
	pthread_join(pool->pth[i],NULL);//等待线程池的所有线程执行完毕

    pthread_mutex_destroy(&(pool->mutex));//销毁互斥变量
    pthread_cond_destroy(&(pool->queue_empty));
    pthread_cond_destroy(&(pool->queue_not_empty));
    pthread_cond_destroy(&(pool->queue_not_full));
    free(pool->pth);//释放线程池中所有线程pth空间
    worker_t *s;
    while(pool->head!=NULL){//释放线程池中任务队列链表
	s=pool->head;
	pool->head=s->next;
	free(s);
    }	
    free(pool);//释放线程池这块空间
    return 0;
}
int main(){
    pool_t *pool=pool_init(10,20);
    pool_add_worker(pool,work,"1");
    pool_add_worker(pool,work,"2");
    pool_add_worker(pool,work,"3");
    pool_add_worker(pool,work,"4");
    pool_add_worker(pool,work,"5");
    pool_add_worker(pool,work,"6");
    pool_add_worker(pool,work,"7");
    pool_add_worker(pool,work,"8");
    pool_add_worker(pool,work,"9");
    pool_add_worker(pool,work,"10");
    pool_add_worker(pool,work,"11");
    pool_add_worker(pool,work,"12");
    pool_add_worker(pool,work,"13");
    pool_add_worker(pool,work,"14");
    pool_add_worker(pool,work,"15");
    pool_add_worker(pool,work,"16");
    pool_add_worker(pool,work,"17");
    pool_add_worker(pool,work,"18");
    pool_add_worker(pool,work,"19");
    pool_add_worker(pool,work,"20");
    sleep(2);
    if(pool_destroy(pool)==0)
	printf("free ok!\n");
    return 0;
}
