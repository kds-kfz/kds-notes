#include <stdio.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

struct stu{
	char name[20];
	int id;
};

void* thread_handle(void* arg)
{	
	struct stu *s = (struct stu*)arg; 
	printf("in thread_handle:%lu,pid=%d\n",pthread_self(),getpid());
	printf("name = %s, id = %d\n",s->name,s->id);
	return NULL;
}

int main()
{
	int i;
	struct stu st = {"xiaoming",1001};
	pthread_t pth;//线程标识符
	printf("in main:thread id = %lu,pid = %d\n",pthread_self(),getpid());
	for(i=0; i<5; i++){
		int ret = pthread_create(&pth,NULL,thread_handle,(void*)&st);
		if(ret != 0)
			fprintf(stderr,"pthread_create:%s\n",strerror(ret));
	}

	sleep(5);
	return 0;
}
