/*
	孤儿进程:父进程先于子进程结束,则子进程成为孤儿进程
	子进程的父进程成为init进程,init领养孤儿进程
 */

#include <stdio.h>
#include <unistd.h>

int main()
{
	pid_t pid;
	pid = fork();
	if(pid == -1){
	
	}else if(pid == 0){
		while(1){
			sleep(1);
			printf("in child:pid = %d, ppid = %d\n",getpid(),getppid());
		}
	}else {
		printf("in father...die,pid = %d\n",getpid());
	}

	return 0;
}


