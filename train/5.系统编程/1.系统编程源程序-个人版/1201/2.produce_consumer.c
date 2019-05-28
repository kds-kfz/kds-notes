#include <pthread.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
//2.produce_consumer.c
#if 0
    int pthread_mutex_init(pthread_mutex_t *restrict mutex,
	    const pthread_mutexattr_t *restrict attr);
    mutex：互斥量
    attr：设置为NULL，用默认的属性初始化互斥量
    返回值：成功返回0,错误返回错误编号
    ***************************************************************
    int pthread_mutex_lock(pthread_mutex_t *mutex);
    mutex：互斥量
    返回值：成功返回0,失败返回错误编号
    ***************************************************************
    int pthread_mutex_unlock(pthread_mutex_t *mutex);
    mutex：互斥量
    返回值：成功返回0,错误返回错误编号
    ***************************************************************
    int pthread_mutex_trylock(pthread_mutex_t *mutex);
    mutex：互斥量
    返回值：成功返回0,错误返回错误编号
    ***************************************************************
    int pthread_mutex_destroy(pthread_mutex_t *mutex);
    mutex：互斥量
    返回值：成功返回0,错误返回错误编号
#endif
pthread_mutex_t mutex;
int pencil=5;
int ret;
void *thread_handle1(void *arg){
    while(1){
    sleep(1);
    ret=pthread_mutex_lock(&mutex);
//    ret=pthread_mutex_trylock(&mutex);
    if(ret!=0)
	printf("lock %s\n",strerror(ret));
    pencil++;
    printf("produce %dth pencil\n",pencil);
    pthread_mutex_unlock(&mutex);
    sleep(1);
    }
    return NULL;
}
void *thread_handle2(void *arg){
    while(1){
//    sleep(1);
    ret=pthread_mutex_lock(&mutex);
//    ret=pthread_mutex_trylock(&mutex);
    if(ret!=0)
	printf("lock %s\n",strerror(ret));
    if(pencil<=0)
	pthread_mutex_unlock(&mutex);
    else{
	pencil--;
	printf("consumer a pencil rest : %d\n",pencil);
    }
    pthread_mutex_unlock(&mutex);
    sleep(1);
    }
    return NULL;
}

int main(){
    pthread_t pth[2];
    pthread_mutex_init(&mutex,NULL);
    ret=pthread_create(&pth[0],NULL,thread_handle1,NULL);
    if(ret!=0)
	printf("create %s\n",strerror(ret));
    sleep(1);
    ret=pthread_create(&pth[1],NULL,thread_handle2,NULL);
    if(ret!=0)
	printf("create %s\n",strerror(ret));
    sleep(1);
    ret=pthread_join(pth[0],NULL);
    pthread_mutex_destroy(&mutex);
    if(ret!=0)
	printf("join %s\n",strerror(ret));
    ret=pthread_join(pth[1],NULL);
    if(ret!=0)
	printf("join %s\n",strerror(ret));
    return 0;
}
