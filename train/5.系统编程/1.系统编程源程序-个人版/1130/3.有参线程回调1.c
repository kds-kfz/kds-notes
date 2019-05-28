#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include <pthread.h>
//3.有参线程回调1.c
#if 0
    循环创建线程，执行各自线程的回调
    主控线程发送数字，线程接收后打印其值
    
    lu == unsigned long int
    
    创5个线程，每个时间片不一定5个线程同时执行同1个任务，
    有可能是6个子线程，有一个线程执行2次
#endif
void *thread_handle(void *arg){
    #if 0
    //情况1
    int i=*((int *)arg);
    //传入的是(void *)地址，是8字节的，
    //需要将其强转成(int *)类型地址，然后再取地址的值
    printf("in thread_handle pth = %lu,pid = %d,i = %d\n",pthread_self(),getpid(),i);
    #endif
    #if 1
    //情况2
    long int i=(long int)arg;
    //传入的是值，此时含需要强转成需要的类型
//    long int i=*(long int *)arg;
    //传入的是地址，此时将(void *)强转成long int *类型的地址，再取值
//    long int i=*(long int *)arg;
    //传入的是地址，此时将(void *)强转成long int *类型的地址，再取值
    printf("in thread_handle pth = %lu,pid = %d,i = %ld\n",pthread_self(),getpid(),i);
    #endif
    return NULL;
}
int main(){
    pthread_t pth;
    long int x=9;
    //当创建进程是有参回调，有以下情况
    //1.参数地址固定，地址指向的内容可变，此时不要传参数地址
    //2.参数地址固定，地址里的内容固定的，此时即可以传参数地址也可以传参数的值
    //本机器是64位，任何指针的大小都是8字节，如果传入的值不是8字节需要加强转
    for(int i=0;i<5;i++){
    #if 0
    //情况1
//	int ret=pthread_create(&pth,NULL,thread_handle,(void *)&i);
	//传入i地址，i的地址固定，但值是可变的
//	int ret=pthread_create(&pth,NULL,thread_handle,(void *)x);
	//传入固定的值x，传入x值
//	int ret=pthread_create(&pth,NULL,thread_handle,(void *)&x);
	//==ret=pthread_create(&pth,NULL,thread_handle,(long int *)&x);
	//传入固定的值x，传入x地址
    #endif
    #if 1
    //情况2
	int ret=pthread_create(&pth,NULL,thread_handle,(void *)(long int)i);
	//传入i的值，i是整型，需要强转成长整型
    #endif
	if(ret!=0){
	printf("pthread_creatr:%s\n",strerror(ret));
	exit(-1);
	}
    }
    sleep(2);
    return 0;
}
