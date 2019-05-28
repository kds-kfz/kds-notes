#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void *thread_handle(void *arg)
{
	while(1)
		sleep(1);
}

int main()
{
	pthread_t pth;
	int ret, count;
	while(1){
		ret = pthread_create(&pth,NULL,thread_handle,NULL);
		if(ret != 0)
		{
			fprintf(stderr,"%s\n",strerror(ret));
			break;
		}	
		printf("------%d\n",++count);
	}
	return 0;	
}

