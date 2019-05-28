#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

/*
	pthread_self()用于获取线程id,其作用对应进程中的getpid()函数
	pthread_t pthread_self()   返回值:成功返回0,失败:无
	线程id:pthread_t 类型,本质:在linux下为无符号整数
	线程id是进程内部识别标志(不同的进程间,线程的id是允许相同的)
*/

void *thread_handle(void *arg)
{
	printf("in thread: thread id = %lu,pid = %d\n",pthread_self(),getpid());
	return NULL;
}

int main()
{	
	pthread_t pid;
	printf("in main: thread id = %lu,pid = %d\n",pthread_self(),getpid());
	/*我们一般把调用pthread_create叫做主控线程*/
	int ret = pthread_create(&pid,NULL,thread_handle,NULL);
	if(ret != 0){
		perror("pthread_create");
		exit(-1);
	}
	printf("in main: thread id = %lu,pid = %d\n",pthread_self(),getpid());
	
	sleep(10);
	return 0;
}







