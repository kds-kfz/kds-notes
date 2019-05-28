#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include <pthread.h>
//10.pthread_jion4.c
#if 0
    int pthread_join(pthread_t thread, void **retval);
    thread：线程的id
    retval：
    返回值：成功返回0,否则返回错误编号

    循环创建3个线程，主线程向线程发送整型数据，
    各自线程接收到数据后将其打印，同时返回1个结构体
    主线程循环阻塞等待各线程返回的资源，接收后将其打印，并释放
#endif
typedef struct{
    int id;
    char name[20];
}Stu;
void *thread_handle(void *arg){
    long id=(long)arg;
    Stu *s=(Stu *)malloc(sizeof(Stu));
    s->id=1001;
    strcpy(s->name,"lisi");
    printf("第%ld个线程,pth = %lu,pid = %d\n",id,pthread_self(),getpid());
    pthread_exit((void *)s);
    // == return (void *)s;
}
int main(){
    pthread_t pth[3];
    Stu *s=NULL;
    for(int i=0;i<3;i++){
    int ret=pthread_create(&pth[i],NULL,thread_handle,(void *)(long)i);
    if(ret!=0){
	printf("pthread_creatr:%s\n",strerror(ret));
	exit(-1);
	}
    }
    printf("sleep 2s ...\n");
    sleep(2);
    printf("in main\n");
    //阻塞等待子线程返回
    for(int i=0;i<3;i++){
    pthread_join(pth[i],(void **)&s);
    printf("id = %d,name = %s\n",s->id,s->name);
    free(s);
    printf("主控线程释放线程资源[%d]ok\n",i);
    }
    return 0;
}
