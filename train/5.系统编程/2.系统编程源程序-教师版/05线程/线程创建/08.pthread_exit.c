/*
	pthread_exit和exit,return 的区别
*/
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void *thread_handle(void *arg)
{
	printf("in thread_handle:thread id = %lu\n",pthread_self());
	exit(1);/*当先调用这个函数时,整个进程退出*/
	return NULL;/*但在子线程中,仅仅是退出当前线程,返回到调用者*/
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

	sleep(10);
	exit(1);/*将整个进程退出*/
	
	return 0;/*将整个进程退出*/
}
