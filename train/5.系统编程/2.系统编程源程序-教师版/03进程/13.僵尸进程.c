/*
	僵尸进程:进程终止,父进程尚未回收资源,子进程
	残留(PCB)存放在内核中,变成僵尸进程
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main()
{
	pid_t pid;
	pid = fork();
	if(pid == -1){
		fprintf(stdout,"fork error\n");
		exit(-1);
	}else if(pid == 0){
		printf("child process is going to die..[pid = %d: ppid = %d]\n",getpid(),getppid());
	}else {
		sleep(1);
		printf("parent process[pid = %d]\n",getpid());
		while(1);
	}

	return 0;
}
