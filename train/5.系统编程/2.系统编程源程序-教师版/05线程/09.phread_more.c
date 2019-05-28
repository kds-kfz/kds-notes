#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

struct stu{
	char name[20];
	int id;
};

void* thread_handle(void* arg)
{
	int i = (int)arg;
	if(i == 1){
		sleep(i);
		printf("in thread[%d]:[%lu]:[%d]\n",i,pthread_self(),getpid());
		pthread_exit((void*)i);
	}else if(i == 2){
		sleep(i);
		printf("in thread[%d]:[%lu]:[%d]\n",i,pthread_self(),getpid());
		return (void*)i;
	}else{
		sleep(i);
		printf("in thread[%d]:[%lu]:[%d]\n",i,pthread_self(),getpid());
		pthread_exit((void*)i);		
	}
}

int main()
{
	pthread_t pth[3];
	int ret,i;
	int *status;

	//循环创建３个线程
	for(i=0; i<3; i++){
		ret = pthread_create(&pth[i],NULL,thread_handle,(void*)i);
		if(ret != 0)
			fprintf(stderr,"thread ceate error:%s\n",strerror(ret));		
	}
	//主控线程回收多个退出线程(３)
	for(i=0; i<3; i++){
		//阻塞等待回收线程资源
		pthread_join(pth[i],(void**)&status);
		printf("pthread_join[%d]\n",(int)status);
	}

	return 0;
}
