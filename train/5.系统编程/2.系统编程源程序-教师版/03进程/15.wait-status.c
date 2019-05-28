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
	
	int status;
	
	pid = fork();
	if(pid == -1){
		fprintf(stdout,"fork error\n");
		exit(-1);
	}else if(pid == 0){
		sleep(2);
		printf("child process is going to die..[pid = %d: ppid = %d]\n",getpid(),getppid());
		while(1);
		exit(67);
	}else {
		printf("parent process[pid = %d]\n",getpid());
		printf("before wait...\n");
		sleep(10);
		wid = wait(&status);//阻塞等待回收子进程资源

		if(WIFEXITED(status))
			printf("status = %d\n",WEXITSTATUS(status));
		
		if(WIFSIGNALED(status))	
			printf("status = %d\n",WTERMSIG(status));

		printf("aftern wait...\n");
		printf("wid = %d\n",wid);
		while(1);
	}

	return 0;
}
