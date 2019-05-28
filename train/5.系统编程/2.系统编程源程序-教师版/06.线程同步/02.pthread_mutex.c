#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

pthread_cond_t cond;
pthread_mutex_t mutex;

int pencil = 4;

void* threadProduce(void* arg)
{
	for(;;){
		sleep(1);
		pthread_mutex_lock(&mutex);
		pencil++;
		printf("produce a pencil[%d]\n",pencil);
		pthread_mutex_unlock(&mutex);
		
		pthread_cond_signal(&cond);
		sleep(1);
	}
	return NULL;
}

void* threadConsumer1(void* arg)
{
	for(;;){
		sleep(1);
		pthread_mutex_lock(&mutex);
		
		while(pencil<=0){
			pthread_cond_wait(&cond,&mutex);
			/*阻塞等待,释放已经掌握的锁*/
			/*被唤醒后,解除阻塞,重新拿锁*/
		}
		pencil--;
		printf("consumer1 a pencil[%d]\n",pencil);
		pthread_mutex_unlock(&mutex);
		sleep(1);
		}
	return NULL;
}


void* threadConsumer2(void* arg)
{
	for(;;){
		sleep(1);
		pthread_mutex_lock(&mutex);
		
		while(pencil<=0){
			pthread_cond_wait(&cond,&mutex);
			/*阻塞等待,释放已经掌握的锁*/
		}
		pencil--;
		printf("consumer2 a pencil[%d]\n",pencil);
		pthread_mutex_unlock(&mutex);
		sleep(1);
		}
	return NULL;
}

int main()
{
	pthread_t pth[3];
	int ret;

	pthread_mutex_init(&mutex,NULL);
	pthread_cond_init(&cond,NULL);

	ret = pthread_create(&pth[0],NULL,threadProduce,NULL);
	if(ret != 0)
		fprintf(stderr,"create error:%s\n",strerror(ret));
	
	ret = pthread_create(&pth[1],NULL,threadConsumer1,NULL);
	if(ret != 0)
		fprintf(stderr,"create error:%s\n",strerror(ret));
	
	ret = pthread_create(&pth[2],NULL,threadConsumer2,NULL);
	if(ret != 0)
		fprintf(stderr,"create error:%s\n",strerror(ret));
	
	pthread_join(pth[0],NULL);
	pthread_join(pth[1],NULL);
	pthread_join(pth[2],NULL);
	
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);

	return 0;
}





