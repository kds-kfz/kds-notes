#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include <sys/wait.h>
//7.回收多个僵尸进程.c
int main(){
    pid_t pid;
    pid_t wai;
    int status;
    for(int i=0;i<5;i++){
	pid=fork();
	if(pid==-1){
	    printf("fork fail\n");
	    exit(-1);
	}
	if(pid==0)
	    break;
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
	while(1){//循环回收僵尸进程
	    wai=wait(NULL);//回收成功返回僵尸进程id，失败返回-1
	    printf("wait after\n");
	    printf("wait = %d\n",wai);
	    sleep(1);
	    if(wai==-1)//回收失败退出回收
		break;
	}
	while(1);
    }
    return 0;
}
