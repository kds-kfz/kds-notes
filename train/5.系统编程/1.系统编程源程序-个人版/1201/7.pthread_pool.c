#include<string.h>
#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include <pthread.h>
//7.pthread_pool.c
//第一次写，不完善
void *work(void *arg);

//作业链表结构体
typedef struct worker{
    void*(*pfunc)(void *);//回调函数,真正的执行函数的函数指针
    void* argv;//回调函数的入参
    struct worker* next;
}worker_t;

//线程池
typedef struct{
    int number;//线程池中线程的个数
    int shutdown;//线程池销毁的标志,1表销毁,0表还没销毁,即释放申请的空间
    worker_t *head;//任务队列的表头
    pthread_t *pth;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int work_count;
}pool_t;

pool_t my_pool;
worker_t my_worker;

void *thread_control(void *arg){
    while(1){
    pthread_mutex_lock(&my_pool.mutex);//加锁
    worker_t *w=my_pool.head;
    while(my_pool.work_count==0&&!my_pool.shutdown)
	pthread_cond_wait(&my_pool.cond,&my_pool.mutex);
    if(my_pool.shutdown){
	pthread_mutex_unlock(&my_pool.mutex);//解锁
	pthread_exit(NULL);
    }
    my_pool.work_count--;
    printf("work_count = %d\n",my_pool.work_count);
    if(my_pool.work_count==0)
	my_pool.head=NULL;
    else
	my_pool.head=w->next;
    if(my_pool.work_count==0)
	pthread_cond_signal(&my_pool.cond);
    (*(w->pfunc))(w->argv);
    free(w);
    w=NULL;
    pthread_mutex_unlock(&my_pool.mutex);//解锁
    sleep(1);
    }
}
void *work(void *arg){
    char *buf=(char *)arg;
    printf("work : %s\n",buf);
}

int pool_init(int number){//线程池初始化,里面要创建多少个线程
    my_pool.head=NULL;
    my_pool.number=number;
    my_pool.shutdown=0;
    my_pool.work_count=0;
    pthread_mutex_init(&my_pool.mutex,NULL);
    pthread_cond_init(&my_pool.cond,NULL);
    pthread_t pth[number];
    int ret;
    for(int i=0;i<number;i++){
	//创建线程
	ret=pthread_create(&pth[i],NULL,thread_control,NULL);
	if(ret!=0)
	    printf("creat[%d] error : %s\n",i,strerror(ret));
    }
    my_pool.pth=pth;
    return 1;
}

int pool_add_worker(void*(*pfunc)(void*),void* arg){//向任务队列里添加任务
    pthread_mutex_lock(&my_pool.mutex);//加锁
    if(my_pool.shutdown==1)//如果线程池被释放
	return -1;
    worker_t *w=(worker_t *)malloc(sizeof(worker_t));
    if(w==NULL){
	pthread_mutex_unlock(&my_pool.mutex);//解锁
	return -1;
    }
    w->argv=arg;//
    w->pfunc=pfunc;//传进来的任务
    w->next=NULL;

    w->next=my_pool.head;//单链表头创建
    my_pool.head=w;
    my_pool.work_count++;
    pthread_mutex_unlock(&my_pool.mutex);//解锁
    return 0;
    /*
    //判断两个线程pth是否相等
    */
}
int pool_destroy(){//销毁线程池  //pthread_cond_broadcast
    pthread_mutex_lock(&my_pool.mutex);//加锁
    my_pool.shutdown=1;
    printf("i am here\n");
    while(my_pool.work_count!=0)
	pthread_cond_wait(&my_pool.cond,&my_pool.mutex);
    my_pool.shutdown=1;
    pthread_mutex_unlock(&my_pool.mutex);//解锁
    pthread_cond_broadcast(&my_pool.cond);
    for(int i=0;i<my_pool.number;i++)
	pthread_join(my_pool.pth[i],NULL);
    pthread_mutex_destroy(&my_pool.mutex);
    pthread_cond_destroy(&my_pool.cond);
    worker_t *s=NULL;
    while(my_pool.head!=NULL){
	s=my_pool.head;
	my_pool.head=s->next;
	free(s);
    }	
    return 0;
}
int main(){
    pool_init(10);
    pool_add_worker(work,"1");
    pool_add_worker(work,"2");
    pool_add_worker(work,"3");
    pool_add_worker(work,"4");
    pool_add_worker(work,"5");
    pool_add_worker(work,"6");
    pool_add_worker(work,"7");
    pool_add_worker(work,"8");
    pool_add_worker(work,"9");
    pool_add_worker(work,"10");
    pool_add_worker(work,"11");
    pool_add_worker(work,"12");
    pool_add_worker(work,"13");
    pool_add_worker(work,"14");
    pool_add_worker(work,"15");
    pool_add_worker(work,"16");
    pool_add_worker(work,"17");
    pool_add_worker(work,"18");
    pool_add_worker(work,"19");
    pool_add_worker(work,"20");
  
    sleep(5);
    int ret=pool_destroy();
//    sleep(2);
//    for(;;){
//	pause();
//	sleep(1);
//	if(ret==0)
//	    printf("pool_destroy ok\n");
//    }
    return 0;
}


