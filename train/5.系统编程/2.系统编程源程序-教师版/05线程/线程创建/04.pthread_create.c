#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
/*
int pthread_create(pthread_t *thread,传出参数,保存系统为我们分配好的线程id 
                   const pthread_attr_t *attr,通常传NULL,表示线程默认属性
				   void *(*start_routine) (void *),函数指针,指向线程主函数
				   (线程体,该函数运行结束,则线程结束)
				   void *arg);线程主函数执行期间所使用的参数

返回值:
成功反回0,错误返回错误编号
*/

void *thread_handle(void *arg)
{
	printf("in thread_handle\n");
	return NULL;
}

int main()
{
	pthread_t pid;
	
	int ret = pthread_create(&pid,NULL,thread_handle,NULL);
	if(ret != 0){
		char *buf = strerror(ret);
		fprintf(stderr,"%s",buf);
		//perror("pthread_create");可以用上面的替换
		exit(-1);
	}
	sleep(10);
	/*
		线程是程序执行的一条路径,主函数结束,线程执行结束,进程结束,所有线程都会结束,进程的任何地方调用exit都会结束整个进程
	*/
	return 0;
}
