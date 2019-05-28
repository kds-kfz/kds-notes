#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

/*
  静态初始化一个互斥量和条件变量
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
*/


pthread_mutex_t mutex;
pthread_cond_t cond;

struct msg{
	int num;
	struct msg *next;
};
struct msg *head = NULL;
struct msg *temp = NULL;

void* produce(void* arg)
{
	for(;;){
		temp = (struct msg*)malloc(sizeof(struct msg));
		temp->num++;
		printf("produce:%d\n",temp->num);

		pthread_mutex_lock(&mutex);
		temp->next = head;
		head = temp;
		pthread_mutex_unlock(&mutex);

		pthread_cond_signal(&cond);
		//唤醒阻塞等待条件变量的线程
		sleep(2);
	}
}

void* consumer(void* arg)
{
	for(;;){
		pthread_mutex_lock(&mutex);
		/*加while循环是为了处理多个消费者*/
		while(head == NULL)
			/*阻塞等待条件变量满足*/
			/*解锁上面的lock*/

			/*当被唤醒后,加锁并返回*/
			pthread_cond_wait(&cond,&mutex);
		temp = head;
		head = temp->next;
		pthread_mutex_unlock(&mutex);
		
		printf("consumer:%d\n",temp->num);
		free(temp);
		sleep(2);
	}
}

int main()
{
	pthread_t pth[2];
	pthread_mutex_init(&mutex,NULL);
	pthread_cond_init(&cond,NULL);

	pthread_create(&pth[0],NULL,produce,NULL);
	pthread_create(&pth[1],NULL,consumer,NULL);	   
	pthread_join(pth[0],NULL);
	pthread_join(pth[1],NULL);
	
	return 0;
}
