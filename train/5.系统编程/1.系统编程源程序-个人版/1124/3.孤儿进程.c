#include<stdio.h>
#include<unistd.h>
//3.孤儿进程.c
#if 0
    孤儿进程：父进程先于子进程结束，则子进程成为孤儿进程
    子进程的父进程成为了init进程，init领养孤儿进程
    ----------------------------------------------------
    孤儿进程：父进程已经终止的所有进程，他们的父进程都改变为init进程
    此时父进程的id是1(init进程的id)

    注意：由init收养的进程，即孤儿进程终止时，不会变成僵死进程，
    此时init会调用一个wait函数获得其终止状态，防止系统中塞满僵死进程
#endif
int main(){
    pid_t pid=fork();
    if(pid==-1){
    }
    else if(pid==0){
	while(1){//此时需要kill -9 子进程id，才可以结束该进程
	    sleep(1);
	    printf("in child pid=%d,parent pid=%d\n",getpid(),getppid());
	}
    }
    else{
	printf("in parent is going die pid=%d\n",getpid());
//	exit(-1);//父进程结束,这里不写exit(-1)也是父进程结束了
    }
    return 0;
}
