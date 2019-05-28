#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void *thread_handle(void *arg)
{
	//int i = *((int*)arg)
	int i = (int)arg;
	printf("in thread[%d]: thread id = %lu,pid = %d\n",i,pthread_self(),getpid());
	return NULL;
}

int main()
{
	int i;
	int ret;
	pthread_t pid;
	for(i=0; i<5; i++){                              //(void*)&i      
		ret = pthread_create(&pid,NULL,thread_handle,(void*)i);/*可以传参*/
		if(ret != 0){
			char *buf = strerror(ret);
			fprintf(stderr,"pthread_create error:%s\n",buf);
			exit(1);
		}
	}

	sleep(10);

	return 0;
}
