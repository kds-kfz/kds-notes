#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#if 0
    僵尸进程：进程终止，父进程尚未回收资源，
    子进程残留(PCB)存放在内核中，变成僵尸进程
    --------------------------------------------------------
    僵死进程：一个已经终止，但其父进程尚未对其善后处理的进程
	      (获取终止子进程的相关信息，释放塔仍占用的资源)
	      状态打印为Z，通过 ps -aux 看到
    --------------------------------------------------------
    只要父进程不退出，这个僵尸进程一直存在，
    当父进程退出了，会将它所有子进程都托管给别的进程
    (使之成为别的进程的子进程)
    托管给别的进程：
    可能是退出进程所在的进程组的下一个进程
    或者是1号进程(即init进程)，每个进程每时每刻都有父进程存在，
    出父它是1号进程(即init进程没有父进程)
    --------------------------------------------------------
    1号进程，pid为1的进程，也称init进程
    linux系统启动后，第一个被创建的用户进程就是init进程
    其使命是：
    1.执行系统初始化脚本，创建一系列的进程(他们都是init的子孙)
    2.在一个死循环中等待其子进程的退出事件，
    并调用waitid系统调用来完成"收尸"工作，即回收僵尸进程,释放其占用资源
#endif

int main(){
    pid_t pid;
    pid_t wai;
    int status;
    if((pid=fork())==-1){
	printf("fork fail\n");
	exit(-1);
    }
    else if(pid==0){
	printf("in child is going die pid=%d,parent pid=%d\n",getpid(),getppid());
    }
    else{
	/*僵尸进程id
	kaifazhe  5955  0.0  0.0    0   0 pts/12   Z+   14:05   0:00 [a.out] <defunct>
	*/
	printf("in parent pid=%d\n",getpid());
	printf("before wait ...\n");
	sleep(10);
	while(1);
    }
    return 0;
}
