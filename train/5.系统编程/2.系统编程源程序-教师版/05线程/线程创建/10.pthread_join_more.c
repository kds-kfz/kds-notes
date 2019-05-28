/*线程和线程之间也是可以调用回收,不一定是主控线程回收,他们之间是同级*/
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int var = 10;

void* thread_handle(void *arg)
{
	int i = (int)arg;
	sleep(i);
	if(i == 1){
		var = 1;
		printf("var = %d\n",var);
		return (void*)var;
	}else if(i == 3){
		var = 777;
		printf("in thread[%d], id = %lu, var = %d\n",i+1,pthread_self(),var);
		pthread_exit((void*)var);
	}else{
		printf("in thread[%d], id = %lu, var = %d\n",i+1,pthread_self(),var);
		pthread_exit((void*)var);
	}

	return NULL;
}

int main()
{
	pthread_t pth[5];
	int i;
	int* ret[5];

	for(i=0; i<5; i++)
		pthread_create(&pth[i],NULL,thread_handle,(void*)i);
	
	for(i=0; i<5; i++){
		pthread_join(pth[i],(void**)&ret[i]);
		printf("thread i = %d,ret = %d\n",i,(int)ret[i]);
	}
	printf("in main thread pth = %lu, var = %d\n",pthread_self(),var);
	
	sleep(10);

	return 0;
}
