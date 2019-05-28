/*
	将单个线程退出
	void pthread_exit(void *retval);retval表示线程退出状态,通常传NULL
	在前一个例子中演示了exit调用后,导致进程内所有线程全部退出
	进程结束,所有线程都会结束
*/
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void *thread_handle(void *arg)
{
	printf("in thread_handle:thread id = %lu\n",pthread_self());
	return NULL;
}

int main()
{
	pthread_t pid;
	int ret = pthread_create(&pid,NULL,thread_handle,NULL);
	if(ret != 0){
		fprintf(stderr,"pthread_create:%s\n",strerror(ret));
	}
	/*在循环创建多个线程,并且传参时,可以再说明(void*)&i 它传递过去的是i地址,当线程区访问这个地址时,
	其他的线程会被创建,此时地址空间中的i值会改变
	*/
	pthread_exit(NULL);

	return 0;
}
