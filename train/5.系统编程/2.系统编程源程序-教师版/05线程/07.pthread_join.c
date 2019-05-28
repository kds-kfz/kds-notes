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
	struct stu *s = (struct stu*)malloc(sizeof(struct stu));
	s->id = 1001;
	strcpy(s->name,"lisi");
	printf("in thread:[%lu]:[%d]",pthread_self(),getpid());
	pthread_exit((void*)s);
}

int main()
{
	pthread_t pth;
	int ret;
	struct stu *s;

	ret = pthread_create(&pth,NULL,thread_handle,NULL);
	if(ret != 0)
		fprintf(stderr,"thread ceate error:%s\n",strerror(ret));		
	
	pthread_join(pth,(void**)&s);	 
	printf("name=%s,id=%d\n",((struct stu*)s)->name,((struct stu*)s)->id);
	free(s);
	return 0;
}
