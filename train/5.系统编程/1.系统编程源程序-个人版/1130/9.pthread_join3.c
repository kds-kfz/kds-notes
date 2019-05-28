#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include <pthread.h>
//9.pthread_join3.c
#if 0
    int pthread_join(pthread_t thread, void **retval);
    thread：线程的id
    retval：
    返回值：成功返回0,否则返回错误编号

    创建2个线程，线程1返回一个结构体数据，
    线程2阻塞等待线程1的资源，接收后将其打印，
    释放其数据，同时返回另一个结构体数据
    主线程阻塞等待线程2的资源，接收后将其打印，最后释放其数据
#endif
typedef struct{
    int id;
    char name[20];
}Stu;

pthread_t pth[2];

void *thread_handle1(void *arg){
    Stu *s=(Stu *)malloc(sizeof(Stu));
    s->id=1001;
    strcpy(s->name,"lisi");
    printf("线程1,pth = %lu,pid = %d\n",pthread_self(),getpid());
    pthread_exit((void *)s);
    return NULL;
}
void *thread_handle2(void *arg){
    Stu *s=(Stu *)malloc(sizeof(Stu));
    s->id=1002;
    strcpy(s->name,"lisi");
    Stu *s1=NULL;
    printf("线程2,pth = %lu,pid = %d\n",pthread_self(),getpid());
    pthread_join(pth[0],(void **)&s1);
    printf("线程2接收线程1资源1\n");
    printf("id = %d,name = %s\n",s1->id,s1->name);
    free(s1);
    printf("线程2释放线程1资源ok\n");
    
    pthread_exit((void *)s);
    return NULL;
}
int main(){
    Stu *s=NULL;
    int ret=pthread_create(&pth[0],NULL,thread_handle1,NULL);
    if(ret!=0){
	printf("pthread_creatr:%s\n",strerror(ret));
	exit(-1);
    }
    sleep(2);
    printf("线程1创建好资源1\n");

    ret=pthread_create(&pth[1],NULL,thread_handle2,NULL);
    if(ret!=0){
	printf("pthread_creatr:%s\n",strerror(ret));
	exit(-1);
    }
    sleep(2);
    printf("主线程接收线程2资源2\n");

    //阻塞等待子线程返回
    pthread_join(pth[1],(void **)&s);
    printf("id = %d,name = %s\n",s->id,s->name);
    free(s);
    printf("主控线程释放线程2资源ok\n");
    return 0;
}
