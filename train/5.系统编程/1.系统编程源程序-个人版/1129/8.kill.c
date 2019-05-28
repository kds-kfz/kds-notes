#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<sys/types.h>
#include<unistd.h>
//8.kill.c
#if 0
    int kill(pid_t pid, int sig);
    pid：有以下几种取值
	>0：将信号发送给指定pid的进程
	=0：将信号发送给和当前进程在同一进程组的所有进程
	=-1：将信号发送给系统内所有的进程
	<0：将信号发送给进程组号PGID为pid绝对值的所有进程
    sig：发送的信号值(1-64)，kill -l 可查看
    返回值：成功返回0,错误返回-1
#endif
int main(int argc,char *argv[]){
    if(argc<3){
	perror("input error\n");
	exit(-1);
    }
    int sig=atoi(argv[1]);
    pid_t pid=atoi(argv[2]);
    //给指定进程发送信号
    kill(pid,sig);

    printf("sleep 5s ...\n");
    sleep(5);
    //杀死自己
    kill(getpid(),9);
    return 0;
}
