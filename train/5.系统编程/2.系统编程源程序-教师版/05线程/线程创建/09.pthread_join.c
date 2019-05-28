#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/*
	pthread_join(pthread_t thread, void**retval)
	参数1:线程id
	参数2:存储线程结束状态
*/

typedef struct stu{
	char name[20];
	int id;
}STU;

void *thread_handle(void *arg)
{
	int num = 1;
	STU *s = malloc(sizeof(STU));
	s->id = 1001;
	strcpy(s->name,"lisi");
	//int i = *((int*)arg)
	int i = (int)arg;
	printf("in thread[%d]: thread id = %lu,pid = %d\n",i,pthread_self(),getpid());
	pthread_exit((void*)s);
	pthread_exit((void*)num);
}

int main()
{
	int i;
	int ret;
	int *status;
	pthread_t pid;
	STU *rs;
	ret = pthread_create(&pid,NULL,thread_handle,(void*)i);/*可以传参*/
	if(ret != 0){
		char *buf = strerror(ret);
		fprintf(stderr,"pthread_create error:%s\n",buf);
	}

	pthread_join(pid,(void**)&rs);
	printf("name = %s,id = %d\n",rs->name,rs->id);
	//pthread_join(pid,(void**)&status);
	//printf("%d\n",(int)status);
	return 0;
}
