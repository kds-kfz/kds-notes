#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
//6.wait-status.c
#if 0
    详情请看8.waitpid.c
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
	sleep(2);
	printf("in child is going die pid=%d,parent pid=%d\n",getpid(),getppid());
//	while(1);//kill -9 子进程id,注意：这里不是孤儿进程
	exit(35);
    }
    else{
	/*僵尸进程id
	kaifazhe  5955  0.0  0.0    0   0 pts/12   Z+   14:05   0:00 [a.out] <defunct>
	*/
	printf("in parent pid=%d\n",getpid());
	printf("before wait ...\n");
	sleep(10);
//	wai=wait(NULL);//回收僵尸进程,阻塞等待回收子进程资源
	
	wai=wait(&status);//判断是否以exit形式退出
	//如果是以exit退出，即正常退出,接收子进程退出信号
	if(WIFEXITED(status))
	    printf("status = %d\n",WEXITSTATUS(status));
	
	//kill -l有1-64种非正常退出
	//非正常退出，接收子进程退出信号
	if(WIFSIGNALED(status))
	    printf("status = %d\n",WTERMSIG(status));
	printf("after wait ...\n");
	printf("wait = %d\n",wai);
	while(1);
    }
    return 0;
}
