#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

pthread_mutex_t mutex;

int pencil = 0;

void* thread_handle1(void* arg)
{
	for(;;){
		pthread_mutex_lock(&mutex);
		printf("hello ");
		sleep(1);
		printf("world\n");
		pthread_mutex_unlock(&mutex);
		sleep(1);
	}
	return NULL;
}

void* thread_handle2(void* arg)
{
	for(;;){
		pthread_mutex_lock(&mutex);
		printf("HELLO ");
		sleep(1);
		printf("WORLD\n");
		pthread_mutex_unlock(&mutex);
		sleep(1);
	}
	return NULL;
}

int main()
{
	pthread_t pth[2];
	int ret;

	pthread_mutex_init(&mutex,NULL);

	ret = pthread_create(&pth[0],NULL,thread_handle1,NULL);
	if(ret != 0)
		fprintf(stderr,"create error:%s\n",strerror(ret));
	ret = pthread_create(&pth[1],NULL,thread_handle2,NULL);
	if(ret != 0)
		fprintf(stderr,"create error:%s\n",strerror(ret));
	
	pthread_join(pth[0],NULL);
	pthread_join(pth[1],NULL);
	pthread_mutex_destroy(&mutex);
	return 0;
}





