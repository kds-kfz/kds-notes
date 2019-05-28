#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include <pthread.h>
//8.pthread_join2.c
#if 0
    int pthread_join(pthread_t thread, void **retval);
    thread：线程的id
    retval：
    返回值：成功返回0,否则返回错误编号
    
    创建1个线程，线程malloc一个结构体
    线程返回资源(结构体数据),
    主控线程阻塞等待接收该资源，
    当接收成功后将其打印，最后将其释放
#endif
typedef struct{
    int id;
    char name[20];
}Stu;

void *thread_handle(void *arg){
    Stu *s=(Stu *)malloc(sizeof(Stu));
    s->id=1001;
    strcpy(s->name,"lisi");
    printf("pth = %lu,pid = %d\n",pthread_self(),getpid());
    pthread_exit((void *)s);
}
int main(){
    pthread_t pth;
    Stu *s=NULL;
    int ret=pthread_create(&pth,NULL,thread_handle,NULL);
    if(ret!=0){
	printf("pthread_creatr:%s\n",strerror(ret));
	exit(-1);
    }
    printf("in main\n");
    //阻塞等待子线程返回
    pthread_join(pth,(void **)&s);
    printf("id = %d,name = %s\n",s->id,s->name);
    free(s);
    printf("主控线程释放ok\n");
    return 0;
}
