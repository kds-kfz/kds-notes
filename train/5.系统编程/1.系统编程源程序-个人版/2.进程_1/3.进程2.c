#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>
//3.进程2.c
#if 0
    ps aux 或 ps lax
    -------------------------------------------------------
    USER：进程的属主
    PID：进程的id
    PPID：呋喃进程id
    %CPU：进程占用的CPU百分比
    %MEM：占用内存的百分比
    NI：进程的NICE值，数值越大，表示较少占用CPU时间
    VSZ：进程虚拟大小
    RSS：驻留中页的数量
    TTY：终端id
    -------------------------------------------------------
    STAT进程状态：
    D：无法中断的休眠状态(通常IO的进程)
    R：正在运行，可在队列中可过行的
    S：处于休眠状态，可中断的睡眠状态
    T：停止或被追踪
    Z：僵死进程
    X：退出状态，进程即将被销毁
    <：优先级高的进程
    N：优先级低的进程
    L：有些页被锁进内存
    s：进程的领导者(在它之下有子进程)
    l：多进程的(使用CLONE_THREAD,类似NPTL pthreads)
    +：位于后台的进程组
    -------------------------------------------------------
    每个进程有一个tast_struct数据结构
    tast_struct数据结构庞大复杂，可分为一些功能组成部分
    1.进程状态
	运行状态(R)
	等待状态(S)
	停止状态(T)
	僵死状态(Z)
    2.进行调度信息
	进程优先级
    3.标识符
	仅仅是个数字，不是task数组的引索
	用来控制进程对系统中文件和设备的存取权限
    4.进程间通信
	信号，管道
	System V IPC：共享内存，信号灯，消息队列

#endif
int main(){
#if 0
    printf("before fork\n");
    pid_t pid1,pid2,pid3,pid4,pid5;
    pid1=fork();
    if(pid1==-1){
	printf("fork fail\n");
	exit(-1);
    }
    if(pid1==0){
	printf("child id1=%d\n",getpid());
	pid2=fork();
	if(pid2==0){
	    printf("child id2=%d\n",getpid());
	    pid3=fork();
	    if(pid3==0){
		printf("child id3=%d\n",getpid());
	    }
	    else if(pid3>0){
		printf("parent id3=%d\n",getppid());	
	    }
	}
	else if(pid2>0){
	    printf("parent id2=%d\n",getppid());	
	}
    }
    else if(pid1>0){
	printf("parent id1=%d\n",getppid());
    }
#endif
#if 0
    //实现同一个父亲，多个孩子
    printf("before fork\n");
    pid_t pid1,pid2,pid3,pid4,pid5;
    pid1=fork();
    if(pid1==-1){
	printf("fork fail\n");
	exit(-1);
    }
    if(pid1==0){
	printf("child id1=%d\n",getpid());
    }
    else if(pid1>0){
	printf("parent id1=%d\n",getppid());
	pid2=fork();
	if(pid2==0){
	    printf("child id2=%d\n",getpid());
	}
	else if(pid2>0){
	    printf("parent id2=%d\n",getppid());
	    pid3=fork();
	    if(pid3==0){
		printf("child id3=%d\n",getpid());
	    }
	    else if(pid3>0){
		printf("parent id3=%d\n",getppid());
	    }
	}
    }
#endif
    //循环创建多个子进程
    //实现同一个父亲，多个孩子
    /*
	    父
    子	子  子	子  子
       */
    pid_t pid1,pid2,pid3,pid4,pid5;
    printf("before fork\n");
    for(int i=0;i<5;i++){
	    sleep(1);
	    pid1=fork();
	    if(pid1==0){//当子进程返回0时，跳出
	        break;
        }
    }
    if(pid1==-1){
	    printf("fork fail\n");
	    exit(-1);
    }
    if(pid1==0){
	    printf("child pid1=%d,ppid=%d\n",getpid(),getppid());
    }else if(pid1>0){
	    printf("parent pid=%d\n",getpid());
    }
    return 0;
}
