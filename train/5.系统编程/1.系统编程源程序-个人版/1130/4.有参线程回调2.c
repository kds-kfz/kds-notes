#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include <pthread.h>
//4.有参线程回调2.c
//循环创建线程，执行格子线程的回调
//主控线程发送结构体，各自线程接收后打印该结构体信息
typedef struct{
    int id;
    char name[20];
}Stu; 
void *thread_handle(void *arg){
    Stu *s=(Stu *)arg;
    printf("id = %d,name = %s\n",s->id,s->name);
    return NULL;
}
int main(){
    pthread_t pth;
    Stu s1={1001,"lisi"};
    for(int i=0;i<5;i++){
	int ret=pthread_create(&pth,NULL,thread_handle,(void *)&s1);
	if(ret!=0){
	printf("pthread_creatr:%s\n",strerror(ret));
	exit(-1);
	}
    }
    sleep(2);
    return 0;
}
