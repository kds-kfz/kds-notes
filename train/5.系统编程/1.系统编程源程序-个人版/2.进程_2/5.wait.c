#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#if 0
    pid_t wait(int *status);
    //详细情况看8.waitpid.c
    返回值：成功返回终止进程的id，错误返回-1
#endif
//5.wait.c
int main(){
    pid_t pid;
    pid_t wai;
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
	wai=wait(NULL);//回收僵尸进程,阻塞等待回收子进程资源
	printf("wait = %d\n",wai);
	while(1);
    }
    return 0;
}
