#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

pthread_t pth[2];

struct stu{
	char name[20];
	int id;
};

void* thread_handle1(void* arg)
{
	int i = (int)arg;
	struct stu *s = (struct stu*)malloc(sizeof(struct stu));
	s->id = 1001;
	strcpy(s->name,"lisi");
	printf("in thread[%d]:[%lu]:[%d]\n",i,pthread_self(),getpid());
	pthread_exit((void*)s);
}

void* thread_handle2(void* arg)
{
	int i = (int)arg;
	struct stu *s;
	pthread_join(pth[0],(void**)&s);
	printf("in thread[%d]:name=%s,id=%d\n",i,((struct stu*)s)->name,((struct stu*)s)->id);
	return NULL;
}
int main()
{
	int ret;
	struct stu *s;

	printf("in main thread...\n");
	ret = pthread_create(&pth[0],NULL,thread_handle1,(void*)1);
	if(ret != 0)
		fprintf(stderr,"thread ceate error:%s\n",strerror(ret));		
	
	ret = pthread_create(&pth[1],NULL,thread_handle2,(void*)2);
	if(ret != 0)
		fprintf(stderr,"thread ceate error:%s\n",strerror(ret));		
	
	sleep(10);
	return 0;
}
