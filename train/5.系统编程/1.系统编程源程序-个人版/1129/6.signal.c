#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include <signal.h>
//6.signal.c
#if 0
    void *signal(int sig, void( *func)(int));
    sig：要安装的信号值
    func：信号的处理函数
	SIG_IGN：忽略信号
	SIG_DFL：采用系统默认的处理信号，执行缺省操作
	不是以上2种，也可以是自己的处理函数
    返回值：成功返回以前的信号处理配置，错误返回SIG_ERR
    ------------------------------------------------------------
    注意：
	SIGKILL和SIGSTOP这两种信号不能被忽略
	因为他们向内核和超级用户提供了使进程终止或停止的可靠方法
	这两种信号也不能捕捉
#endif
void fun(int n){
//    sleep(1);
    printf("rcv sig : %d\n",n);
}
int main(){
#if 1
    //情况1：
    //ctrl+c：SIGINT(2)
//    signal(SIGINT,fun);//执行fun函数
    signal(2,fun);//执行fun函数
    for(;;){
	sleep(1);
    }
#endif
#if 0
    //情况2：
    //杀死进程：SIGKILL(9)
    //kill -9 pid
//    signal(9,fun);//杀死进程，不执行fun函数
//    signal(9,SIG_DFL);//默认系统处理，杀死进程
    signal(9,SIG_IGN);//忽略处理，杀死进程
    for(;;){
	sleep(1);
    }
#endif
#if 0
    //情况3：
    //段错误：SIGSEGV(11)
    int *p=NULL;
//    signal(SIGSEGV,fun);
    signal(11,fun);//执行fun函数，每秒打印一次
//    signal(11,SIG_DFL);//默认系统处理，立马段错误，结束进程
//    signal(9,SIG_IGN);//忽略处理，立马段错误，结束进程
    *p=25;
#endif
    return 0;
}

