#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include <pthread.h>
//11.pthread_detach.c
#if 0
    int pthread_detach(pthread_t thread);
    将线程分离成分离状态
    thread：线程id
    返回值：成功返回0,否则返回错误代码
#endif
void *thread_handle(void *arg){
    printf("thread_handle pth = %lu,pid = %d\n",pthread_self(),getpid());
    return (void *)1;
}
int main(){
    pthread_t pth;
    int ret=pthread_create(&pth,NULL,thread_handle,NULL);
    if(ret!=0){
	printf("pthread_creatr:%s\n",strerror(ret));
	exit(-1);
    }
    pthread_detach(pth);
    //将线程设置为分离状态，此时系统已经回收了该线程号的资源
    //后面不用再写pthread_join阻塞等待回收资源了
    int n=0;
    ret=pthread_join(pth,(void **)&n);//阻塞等待回收线程资源
    
    //当设置为分离状态后，已经没有可回收的资源
    //Invalid argument//无效的参数
    if(ret!=0)
	printf("回收线程资源 : %s\n",strerror(ret));
    else{
	printf("n = %d\n",n);
	printf("回收线程资源 : %s\n",strerror(ret));
    }
    return 0;
}
