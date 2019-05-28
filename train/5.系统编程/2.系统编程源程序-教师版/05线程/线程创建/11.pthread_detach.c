#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/*
	int pthread_detach(pthread_t thread);成功返回0;失败:错误号
	线程分离状态:指定该状态,线程主动与主控线程断开关系,线程结束后,其退出
	状态不由其他线程获取,而直接自动释放.
	也可以使用pthread_create的参数2来设置线程的属性,从而设置为分离线程
*/

void* thread_handle(void* arg)
{
	int n = 3;
	while(n--){
		printf("thread count %d\n",n);
		sleep(1);
	}
	return (void*)1;
}

int main()
{	
	pthread_t pth;
	void *pret;
	int err;

	pthread_create(&pth,NULL,thread_handle,NULL);
	//pthread_detach(pth);//设置创建的线程为分离状态,当线程退出时,无系统残留资源
	
	while(1){
		//它是阻塞等待子线程退出,回收(当线程没有被分离时)
		err = pthread_join(pth,&pret);
		printf("err = %d\n",err);
		if(err != 0)
			fprintf(stderr,"thread join %s\n",strerror(err));
		else
			fprintf(stdout,"thread exit code %d\n",(int)pret);
		sleep(1);		
	}
	
	return 0;
}



