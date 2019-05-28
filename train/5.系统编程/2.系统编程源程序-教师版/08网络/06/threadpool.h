#ifndef __THREADPOOL_H
#define __THREADPOOL_H
/*任务结构*/
typedef struct worker{
	void*(*process)(void* arg);/*回调函数*/
	void* arg;
	struct worker* next;
}WORKER;

typedef struct pool{
	pthread_mutex_t mutex;/*保护任务队列*/
	pthread_cond_t cond;/*条件变量*/

	WORKER* workHead;/*任务表头*/

	pthread_t *threadId;/*存放线程id*/
	
	int shutdown;/*线程池销毁: 0:运行 1:销毁*/

	int maxThreadNum;/*线程池里线程的数量*/
}ThreadPool;
void addThreadWork(void*(*process)(void*arg),void*arg);
void* threadRun(void *arg);
int poolDestroy();
int poolInit(int maxThreadNum);

#endif
