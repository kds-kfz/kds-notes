#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

/*
	杀死(取消)线程函数
	int pthread_cancel(pthread_t thread) 成功:0 失败:错误号
*/

void *thread_handle1(void *arg)
{
	printf("thread 1 running\n");
	return (void*)1;
}

void *thread_handle2(void *arg)
{
	printf("thread 2 running\n");
	pthread_exit((void*)2);
}

void *thread_handle3(void *arg)
{
	while(1){
		//printf("thread 3 running is going to die\n");
		//sleep(1);
		//pthread_testcancell();//自动添加取消点(如果线程没有取消点)
	/*
	   线程的取消并不是实时的,而是有一定延时的
	   需要等待线程到达某个取消点(检查点)
	*/
	}
	return (void*)3;
}

int main()
{
	pthread_t pth;
	void *pret = NULL;

	pthread_create(&pth,NULL,thread_handle1,NULL);
	pthread_join(pth,&pret);
	printf("thread 1 exit code = %d\n",(int)pret);

	pthread_create(&pth,NULL,thread_handle2,NULL);
	pthread_join(pth,&pret);
	printf("thread 2 exit code = %d\n",(int)pret);

	pthread_create(&pth,NULL,thread_handle3,NULL);
	sleep(3);
	pthread_cancel(pth);
	pthread_join(pth,&pret);
	printf("thread 3 exit code = %d\n",(int)pret);
	
	return 0;
}









