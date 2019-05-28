#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include <pthread.h>
//3.pthread_cond_init.c
#if 0
    int pthread_cond_init(pthread_cond_t *restrict cond,
	    const pthread_condattr_t *restrict attr);
    cond：条件变量
    attr：设置为NULL
    返回值：成功返回0,错误返回错误编号
    ***********************************************************
    int pthread_cond_destroy(pthread_cond_t *cond);
    对条件变量反初始化
    cond：条件变量
    返回值：成功返回0,错误返回错误编号
    ***********************************************************
    int pthread_cond_wait(pthread_cond_t *restrict cond,
	    pthread_mutex_t *restrict mutex);
    1.阻塞等待条件满足
    2.释放已经掌握的互斥量
    这两个是一个原子操作
    3.当被唤醒时，解除阻塞，获取互斥量
    cond：条件变量
    mutex：互斥量
    返回值：成功返回0,错误返回错误编号
    ***********************************************************
    int pthread_cond_timedwait(pthread_cond_t *restrict cond,
	    pthread_mutex_t *restrict mutex,
	    const struct timespec *restrict abstime);
    指定等待时间，如果超时条件变量还没有出现，则将获取互斥量
    cond：条件变量
    mutex：互斥量
    abstime：通过struct timespec结构体定义等待时间
    返回值：成功返回0,错误返回错误编号
    ***********************************************************
    int pthread_cond_signal(pthread_cond_t *cond);
    唤醒至少一个阻塞等待该条件变量的线程
    cond：条件变量
    返回值：成功返回0,错误返回错误编号
    ***********************************************************
    int pthread_cond_broadcast(pthread_cond_t *cond);
    唤醒阻塞等待该条件变量的所有线程
    cond：
    返回值：
#endif

pthread_mutex_t mutex;//互斥量
pthread_cond_t cond;//条件变量
int pencil=5;
int ret;
void *thread_Produce(void *arg){
    while(1){
    sleep(1);
    ret=pthread_mutex_lock(&mutex);
//    ret=pthread_mutex_trylock(&mutex);//有锁则等待，没有加锁则不等待
    if(ret!=0)
	printf("lock %s\n",strerror(ret));
    pencil++;
    printf("produce %dth pencil\n",pencil);
    ret=pthread_cond_signal(&cond);
    if(ret!=0)
	printf("lock %s\n",strerror(ret));
    pthread_mutex_unlock(&mutex);
    sleep(1);
    }
    return NULL;
}
void *thread_Consumer1(void *arg){
    while(1){
    sleep(1);
    ret=pthread_mutex_lock(&mutex);
//    ret=pthread_mutex_trylock(&mutex);
    if(ret!=0)
	printf("lock %s\n",strerror(ret));
    while(pencil<=0)
	pthread_cond_wait(&cond,&mutex);
    pencil--;
    printf("consumer1 a pencil rest : %d\n",pencil);

    pthread_mutex_unlock(&mutex);
    sleep(1);
    }
    return NULL;
}

void *thread_Consumer2(void *arg){
    while(1){
    sleep(1);
    ret=pthread_mutex_lock(&mutex);
    if(ret!=0)
	printf("lock %s\n",strerror(ret));
    while(pencil<=0)
//	pthread_mutex_unlock(&mutex);
	pthread_cond_wait(&cond,&mutex);
    pencil--;
    printf("consumer2 a pencil rest : %d\n",pencil);

    pthread_mutex_unlock(&mutex);
    sleep(1);
    }
    return NULL;
}
int main(){
    pthread_t pth[3];
    pthread_mutex_init(&mutex,NULL);
    pthread_cond_init(&cond,NULL);

    ret=pthread_create(&pth[0],NULL,thread_Produce,NULL);
    if(ret!=0)
	printf("create %s\n",strerror(ret));
//    sleep(1);
    ret=pthread_create(&pth[1],NULL,thread_Consumer1,NULL);
    if(ret!=0)
	printf("create %s\n",strerror(ret));
//    sleep(1);
    ret=pthread_create(&pth[2],NULL,thread_Consumer2,NULL);
    if(ret!=0)
	printf("create %s\n",strerror(ret));
    ret=pthread_join(pth[0],NULL);
    ret=pthread_join(pth[1],NULL);
    ret=pthread_join(pth[2],NULL);
    
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    if(ret!=0)
	printf("join %s\n",strerror(ret));
    ret=pthread_join(pth[1],NULL);
    if(ret!=0)
	printf("join %s\n",strerror(ret));
    return 0;
}


