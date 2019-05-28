#include<stdio.h>
#include<signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
//7.signal-fork.c
void fun(int n){
    //捕捉僵尸进程，然后释放资源
    printf("rcv : %d\n",n);
    int wai=wait(NULL);
    if(wai==-1){
	printf("wait fail\n");
	exit(-1);
    }
    printf("wait child pid = %d\n",wai);
}

int main(){
    signal(SIGCHLD,fun);
    pid_t pid;
    if((pid=fork())==-1){
	printf("fork fail\n");
	exit(-1);
    }
    else if(pid==0){
	for(int i=0;i<5;i++){
	printf("in child is going die ... [ pid=%d,ppid=%d\n",getpid(),getppid());
	sleep(1);
	}
    }
    else{
	while(1){
	sleep(1);
	printf("in parent pid = %d\n",getpid());
	}
    }
}
