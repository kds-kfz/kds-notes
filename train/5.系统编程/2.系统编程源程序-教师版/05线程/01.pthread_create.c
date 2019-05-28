#include <stdio.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

void* thread_handle(void* arg)
{
	int i;
	for(i=0; i<4; i++){
		printf("in thread_handle:%lu,pid=%d\n",pthread_self(),getpid());
		sleep(1);
	}
	return NULL;
}

int main()
{
	int i;
	pthread_t pth;//线程标识符
	int ret = pthread_create(&pth,NULL,thread_handle,NULL);
	if(ret != 0)
		printf("pthread_create:%s\n",strerror(ret));

	for(i=0; i<4; i++){
		printf("in main...:%lu,pid=%d\n",pthread_self(),getpid());
		sleep(1);
	}
	
	return 0;
}
