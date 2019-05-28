#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

void fun(int sig)
{
	printf("recv sig:%d\n",sig);
	wait(NULL);
	return;
}

int main()
{
	signal(17,fun);
	pid_t pid;
	int i;
	pid = fork();
	if(pid == -1){
		printf("fork error...\n");
		exit(-1);
	}else if(pid == 0){
		for(i=0; i<5; i++){
			printf("in child:pid = %d,ppid = %d\n",getpid(),getppid());
		sleep(1);
		}
	}else {
		for(;;){
			printf("in father:pid=%d\n",getpid());
			sleep(1);
		}
	}

	return 0;
}
