#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include <pthread.h>
//12.pthread_cancel.c
#if 0
    int pthread_cancel(pthread_t thread);
    取消(杀死)线程
    thread：线程id
    返回值：成功返回0,否则返回错误代码
#endif
void *thread_handle(void *arg){
    #if 1
    while(1){
	sleep(1);
	printf("thread_handle pth = %lu,pid = %d\n",pthread_self(),getpid());
    }
    return (void *)1;//不会执行到这里，一直执行while循环里内容
    #endif
}
int main(){
    pthread_t pth;
    int ret=pthread_create(&pth,NULL,thread_handle,NULL);
    if(ret!=0){
	printf("pthread_creatr:%s\n",strerror(ret));
	exit(-1);
    }
    /*----------------------------------------------------------------*/
//    sleep(5);
    //让出时间片给线程,此时线程没有被杀死，在该时间片里可以执行线程回调
    pthread_cancel(pth);
    sleep(5);
    //让出时间片给线程，此时线程被杀死，不会再执行线程回调，
    //时间过后执行后面的程序，回收该线程的资源
    /*----------------------------------------------------------------*/
    //取消(杀死)线程,线程虽然杀死，
    //当线程里有死循环时，如果不让出cpu使用权，
    //这一直占用cpu使用，当让出cpu使用权
    //执行pthread_cancel杀死线程，
    //则之后该线程回调不会再被执行，但资源仍存在，需要回收
    /*----------------------------------------------------------------*/
    //阻塞等待回收线程资源
    int n=0;
    ret=pthread_join(pth,(void **)&n);//此时线程已被杀死，没有将资源返回
    if(ret!=0)
	printf("回收线程资源 : %s\n",strerror(ret));
    else{
	printf("n = %d\n",n);
	printf("回收线程资源 : %s\n",strerror(ret));
    }
    return 0;
}
