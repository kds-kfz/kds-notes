#include<stdio.h>
#include <sys/time.h>
#include<signal.h>
#include<unistd.h>
//13.setitimer.c
#if 0
    int setitimer(int which, const struct itimerval *new_value,
	    struct itimerval *old_value);
     ITIMER_REAL：
     ITIMER_VIRTUAL：
     ITIMER_PROF：

     struct itimerval {
	 struct timeval it_interval; //第一次启动执行程序的时间
	 struct timeval it_value;    //间隔时间
     };
     struct timeval {
	 time_t      tv_sec;         //秒
	 suseconds_t tv_usec;        //微妙
     };
    ****************************************************************
    int pause(void);
    将进程挂起，知道捕捉到一个信号
    返回值：-1,errno设置为EINTR
    只有执行了一个信号处理程序并从该程序返回时，
    pause才返回-1,errno设置为EINTR

#endif
void fun(int n){
    printf("rcv : %d\n",n);
//    return ;
}
int main(){
    //信号注册
    signal(14,fun);
    struct itimerval t;
    t.it_interval.tv_sec=0;
    t.it_interval.tv_usec=500000;//500ms后执行程序
    t.it_value.tv_sec=2;//2秒后第一次执行程序
    t.it_value.tv_usec=0;
    setitimer(ITIMER_REAL,&t,NULL);
    for(;;){
//	sleep(1);
	pause();
    }
}
