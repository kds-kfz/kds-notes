#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

/*
	int pthread_equal(pthread_t t1,pthread_t t2);
	比较两个线程id是否相等(现在没用,直接==就可以)
*/

int main()
{
	return 0;
}

进程      线程
fork     pthread_create
exit     pthread_exit
wait     pthread_join  两个都是阻塞pthread_detach
kill     pthread_cancel
getpid   pthread_self


