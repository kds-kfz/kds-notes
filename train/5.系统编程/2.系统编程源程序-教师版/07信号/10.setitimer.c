#include <sys/time.h>
#include <stdio.h>

/*
int setitimer(int which, const struct itimerval *new_value, 
struct itimerval *old_value);


struct itimerval {
   struct timeval it_interval;//间隔时间
   struct timeval it_value;//第一次时间 
};

struct timeval {
    time_t      tv_sec;//秒 
   suseconds_t tv_usec;//微秒		
};
*/

void fun(int sig)
{
	printf("recv sig:%d\n",sig);
	return;
}

int main()
{
	//信号注册
	signal(14,fun);
	//结构体赋值
	struct itimerval t;
	t.it_interval.tv_sec = 1;
	t.it_interval.tv_usec =	500*1000; 

	t.it_value.tv_sec = 0;
	t.it_value.tv_usec = 500*1000;
	setitimer(ITIMER_REAL,&t,NULL);
	for(;;){
		sleep(1);
	}

	return 0;
}


