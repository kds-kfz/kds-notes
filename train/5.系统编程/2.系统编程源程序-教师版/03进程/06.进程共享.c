#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

/*
	在父进程(fork之前)打开一个文件
	...
 */

int num = 10;

int main()
{
	printf("before fork...\n");
	int i;
	pid_t pid = fork();
	if(pid == -1){
		printf("创建子进程失败\n");
		exit(-1);
	}else if(pid == 0){
		//子进程
		num = 13;
		printf("in child: num = %d\n",num);
	}else{
		//父进程
		sleep(1);
		printf("in parent: num = %d\n",num);
	}

	return 0;
}

