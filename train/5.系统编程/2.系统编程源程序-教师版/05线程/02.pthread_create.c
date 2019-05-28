#include <stdio.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int i = 0;

void* thread_handle(void* arg)
{	
	printf("in thread_handle:%lu,pid=%d, i = %d\n",pthread_self(),getpid(),++i);
	return NULL;
}

int main()
{
	int i;
	pthread_t pth;//线程标识符
	printf("in main:thread id = %lu,pid = %d\n",pthread_self(),getpid());
	for(i=0; i<5; i++){
		int ret = pthread_create(&pth,NULL,thread_handle,NULL);
		if(ret != 0)
			fprintf(stderr,"pthread_create:%s\n",strerror(ret));
	}

	sleep(3);
	return 0;
}
