#include <stdio.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

void* thread_handle1(void* arg)
{	
	sleep(1);
	printf("in thread_handle1:%lu,pid=%d\n",pthread_self(),getpid());
	return NULL;
}

void* thread_handle2(void* arg)
{	
	sleep(2);
	printf("in thread_handle2:%lu,pid=%d\n",pthread_self(),getpid());
	return NULL;
}

void* thread_handle3(void* arg)
{	
	sleep(3);
	printf("in thread_handle3:%lu,pid=%d\n",pthread_self(),getpid());
	return NULL;
}

int main()
{
	int i;
	int ret;
	pthread_t pth[3];//线程标识符
	printf("in main:thread id = %lu,pid = %d\n",pthread_self(),getpid());
	//第一个
	ret = pthread_create(&pth[0],NULL,thread_handle1,NULL);
	if(ret != 0)
		fprintf(stderr,"pthread_create:%s\n",strerror(ret));
	//第二个
	ret = pthread_create(&pth[1],NULL,thread_handle2,NULL);
	if(ret != 0)
		fprintf(stderr,"pthread_create:%s\n",strerror(ret));
	//第三个
	ret = pthread_create(&pth[2],NULL,thread_handle3,NULL);
	if(ret != 0)
		fprintf(stderr,"pthread_create:%s\n",strerror(ret));

	sleep(4);
	return 0;
}
