#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include <pthread.h>
//1.pthread_create.c
#if 0
    线程有自己独立栈空间，有自己的cpu寄存器
    int pthread_equal(pthread_t t1, pthread_t t2);
    t1：线程id
    t2：线程id
    返回值：比较两个线程id是否相等，相等返回非0,不等返回0
    int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
	    void *(*start_routine) (void *), void *arg);
    thread：线程id
    attr：置NULL，默认系统配置
    start_routine：传参是任意地址，返回值是任意地址的函数指针
    arg：任意类型地址，即参数
    返回值：成功返回0,错误返回错误编号

    char *strerror(int errnum);
    errnum：错误编号
    返回值：返回对应错误编号的字符串
#endif
//    lu == unsigned long int
void *thread_handle(void *arg){
    for(int i=0;i<5;i++){
	printf("in thread_handle pth = %lu,pid = %d\n",pthread_self(),getpid());
	sleep(1);
    }
    return NULL;
}
int main(){
    pthread_t pth;
    int ret=pthread_create(&pth,NULL,thread_handle,NULL);
    if(ret!=0){
	printf("pthread_creatr:%s\n",strerror(ret));
	exit(-1);
    }
    for(int i=0;i<5;i++){
	printf("in main pth = %lu,pid = %d\n",pthread_self(),getpid());
	sleep(1);
    }
    return 0;
}
