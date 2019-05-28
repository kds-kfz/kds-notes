#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int count = 0;
pthread_mutex_t mutex;
pthread_cond_t cond;

struct msg{
	int id;
	struct msg *next;
};

struct msg *head = NULL;
struct msg *temp = NULL;

void* produce(void* arg)
{
	while(1){
		struct msg *ms = (struct msg*)malloc(sizeof(struct msg));
		ms->id = count++;
		//加锁
		pthread_mutex_lock(&mutex);
		//头插
		ms->next = head;
		head = ms;
		printf("produce a point[%d]\n",ms->id);
		pthread_mutex_unlock(&mutex);
		//唤醒等待线程
		pthread_cond_signal(&cond);
		sleep(2);
	}
}

void* consumer(void* arg)
{
	while(1){
		pthread_mutex_lock(&mutex);
		while(head == NULL){
			//阻塞等待条件变量满足
			printf("thread[%lu] is waitting\n",pthread_self());
			pthread_cond_wait(&cond,&mutex);
			printf("thread[%lu] is not waitting\n",pthread_self());
		}
		//摘除结点
		temp = head;
		head = head->next;
		free(temp);
		printf("consumer a point\n");
		pthread_mutex_unlock(&mutex);
		sleep(1);
	}
}

int main()
{
	int i = 0;
	int ret;
	pthread_t pth[4];

	//初始化条件变量和互斥锁
	pthread_mutex_init(&mutex,NULL);
	pthread_cond_init(&cond,NULL);

	//创建线程
	for(i=0; i<2; i++){
		ret = pthread_create(&pth[i],NULL,consumer,NULL);
		if(ret != 0)
			fprintf(stderr,"create error:%s\n",strerror(ret));
	}
	ret = pthread_create(&pth[2],NULL,produce,NULL);
	if(ret != 0)
		fprintf(stderr,"create error:%s\n",strerror(ret));
	ret = pthread_create(&pth[3],NULL,produce,NULL);
	if(ret != 0)
		fprintf(stderr,"create error:%s\n",strerror(ret));


	//线程回收
	for(i=0; i<3; i++){
		pthread_join(pth[i],NULL);
	}
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);

	return 0;
}
