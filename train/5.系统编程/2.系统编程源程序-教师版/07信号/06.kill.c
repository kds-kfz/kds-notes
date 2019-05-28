#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
	if(argc < 3){
		printf("input error...\n");
		exit(-1);
	}
	int sig = atoi(argv[1]);
	pid_t pid = atoi(argv[2]);
	//给指定进程发送信号
	kill(pid,sig);
	printf("before sleep...\n");
	sleep(5);
	kill(getpid(),9);
	return 0;
}
