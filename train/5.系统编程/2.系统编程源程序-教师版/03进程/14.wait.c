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
	pid_t wid;
	pid = fork();
	if(pid == -1){
		fprintf(stdout,"fork error\n");
		exit(-1);
	}else if(pid == 0){
		sleep(20);
		printf("child process is going to die..[pid = %d: ppid = %d]\n",getpid(),getppid());
	}else {
		printf("parent process[pid = %d]\n",getpid());
		printf("before wait...\n");
		wid = wait(NULL);//阻塞等待回收子进程资源
		printf("aftern wait...\n");
		printf("wid = %d\n",wid);
	}

	return 0;
}
