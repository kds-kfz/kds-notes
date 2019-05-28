#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

void* thread_handle(void* arg)
{
	while(1){
		sleep(1);
	}	
	//sleep(1);
	//printf("int thread_handle...\n");
	//pthread_exit(NULL);//退出	
}

int main()
{
	pthread_t pth;
	//线程创建
	int ret = pthread_create(&pth,NULL,thread_handle,NULL);
	if(ret != 0)
		fprintf(stderr,"pthread_create error:%s\n",strerror(ret));
	pthread_cancel(pth);//取消(杀死线程)
		
	ret = pthread_join(pth,NULL);//阻塞等待回收线程的资源
	if(ret != 0)
		fprintf(stderr,"pthread_join error:%s\n",strerror(ret));
	else
		fprintf(stderr,"pthread_join :%s\n",strerror(ret));
		
	return 0;
}
