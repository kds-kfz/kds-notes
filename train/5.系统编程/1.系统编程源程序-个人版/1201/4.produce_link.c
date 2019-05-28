#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include <pthread.h>
//4.produce_link.c
struct Msg{
    int num;
    struct Msg *next;
};

pthread_mutex_t mutex;//互斥量
pthread_cond_t cond;//条件变量
int pencil=5;
int ret;
int count=1000;
struct Msg *head=NULL;
struct Msg *temp=NULL;
void *thread_Produce(void *arg){
    while(1){
    sleep(1);
    struct Msg *ms=(struct Msg *)malloc(sizeof(struct Msg));
    ms->num=count++;
    pthread_mutex_lock(&mutex);
    ms->next=head;
    head=ms;
    printf("produce a point %d\n",ms->num);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    sleep(1);
    }
}
void *thread_Consumer(void *arg){
    while(1){
    sleep(1);
    pthread_mutex_lock(&mutex);
    while(head==NULL){
	printf("line is waiting %lu\n",pthread_self());
	pthread_cond_wait(&cond,&mutex);
	printf("line is not waiting %lu\n",pthread_self());
    }
    temp=head;
    head=head->next;
    free(temp);
    printf("consumer a point\n");
    pthread_mutex_unlock(&mutex);
    sleep(1);
    }
}

int main(){
    pthread_t pth[3];
    pthread_mutex_init(&mutex,NULL);
    pthread_cond_init(&cond,NULL);
    //2个消费线程
    for(int i=0;i<2;i++){
	ret=pthread_create(&pth[i],NULL,thread_Consumer,NULL);
	if(ret!=0)
	    printf("create %s\n",strerror(ret));
    }
    //1个生产线程
    ret=pthread_create(&pth[2],NULL,thread_Produce,NULL);
    if(ret!=0)
	printf("create %s\n",strerror(ret));
    for(int i=0;i<3;i++){
	ret=pthread_join(pth[i],NULL);
	if(ret!=0)
	    printf("create %s\n",strerror(ret));
    }
    for(;;)
	pause();
    //取消互斥量
    pthread_mutex_destroy(&mutex);
    //取消条件变量
    pthread_cond_destroy(&cond);
    return 0;
}


