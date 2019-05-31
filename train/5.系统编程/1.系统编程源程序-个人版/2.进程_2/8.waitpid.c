#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include <sys/wait.h>
//8.waitpid.c
#if 0
    pid_t waitpid(pid_t pid, int *status, int options);
    pid参数：
	pid == -1：等待任意子进程(此时相当于wait)
	pid > 0：等待进程id与指定回收进程id相等
	pid == 0：等待组id与与指定子进程pid绝对值相等
	pid < -1：等待组id与指定进程组id相等
    status参数：
    整型的指针，如果不是空，则终止进程的终止状态旧存放在塔所指向的单元内。
    若不关心终止状态，可设为空，终止状态的宏定义在#include <sys/wait.h>
    ---------------------------------------------------------------------------
    4个终止状态互斥的宏都以WIF开头
	  宏				    说  明
    WIFEXITED(status)	    若是正常终止子进程返回的状态，则为真
			    此时可执行WEXITSTATUS(status),获取子进程传送给
			    exit或_exit参数的低8位
    ---------------------------------------------------------------------------
    WIFSIGNALED(status)	    若是异常终止子进程返回的状态，则为真
			    此时可执行WTERMSIG(status)，获取子进程终止的信号编号
    ---------------------------------------------------------------------------
    WIFSTOPPED(status)	    若是当前暂停子进程的返回的状态，则为真
			    此时可执行WSTOPSIG(status),获取使子进程暂停的信号编号
    ---------------------------------------------------------------------------
    WIFCONTINUED(status)    若是作业控制暂停后yiing继续的子进程返回了状态，则为真
    ---------------------------------------------------------------------------
    options参数：
	0：阻塞
	WCONTINUED：
	在作业控制下，由pid指定的子进程在停止后已经继续，状态未报告，则返回其状态
	WNOHANG：
	若由pid制定的子进程并不是立即可用的，则waitpid不阻塞，此时返回值是0
	WUNTRACED：
	在作业控制下，由pid指定的子进程已经是停止状态，状态未报告，则返回其状态
    ---------------------------------------------------------------------------
    返回值：成功返回进程id，错误返回0
#endif
int main(){
    pid_t pid;
    pid_t p;
    pid_t wai;
    int status;
    int flag=5;
    for(int i=0;i<5;i++){
	pid=fork();
	if(pid==-1){
	    printf("fork fail\n");
	    exit(-1);
	}
	if(pid==0)
	    break;
	if(i==3)
	    p=pid;
    }
    if(pid==0){
	sleep(1);
	printf("in child pid=%d,parent pid=%d\n",getpid(),getppid());
    }
    else{
	printf("in parent pid=%d\n",getpid());
	printf("before wait\n");
	sleep(5);
//	while(wai=wait(NULL));//阻塞等待，回收所有僵尸进程
	#if 0
	while(1){//循环阻塞回收僵尸进程
//	    wai=wait(NULL);//回收成功返回僵尸进程id，失败返回-1
	    wai=waitpid(p,NULL,0);//只回收指定进程的id，下移一次回收失败返回-1
	    printf("wait after\n");
	    printf("wait = %d\n",wai);
	    sleep(1);
	    if(wai==-1)//回收失败退出回收
		break;
	}
	#endif
	#if 1
	//轮循回收进程,非阻塞回收
	do{
	    wai=waitpid(-1,NULL,WNOHANG);
	    if(wai>0)
		flag--;
	    printf("wait after\n");
	    printf("wait = %d\n",wai);
	}while(flag);
	printf("end\n");
	#endif
	while(1);
    }
    return 0;
}
