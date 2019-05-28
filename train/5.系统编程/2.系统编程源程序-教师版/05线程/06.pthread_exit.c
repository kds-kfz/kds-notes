#include <stdio.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

void* thread_handle(void* arg)
{
	int i = (int)arg;
	printf("in thread_handle[%d]:%lu,pid=%d\n",i,pthread_self(),getpid());
	//exit(1);进程退出函数
	pthread_exit(NULL);
	return NULL;
}

int main()
{
	int i;
	pthread_t pth;//线程标识符
	printf("in main:thread id = %lu,pid = %d\n",pthread_self(),getpid());
	for(i=0; i<5; i++){
		int ret = pthread_create(&pth,NULL,thread_handle,(void*)i);
		if(ret != 0)
			fprintf(stderr,"pthread_create:%s\n",strerror(ret));
	}
	printf("main id dying...\n");

	pthread_exit(NULL);//线程退出函数	
	
	return 0;
}
