#include <stdio.h>
#include <unistd.h>

//fork 创建子进程
//wait
//waitpid
//execve函数
//getpid
//getppid

//循环5次先后打印　父进程　子进程
/*
in parent
in child
in parent
in child
...
*/
int main()
{
	printf("before fork...\n");
	int i;
	
	pid_t pid = fork();
	//在父进程返回子进程的ID
	//子进程返回0
	if(pid == -1){
		printf("创建子进程失败\n");
	}else if(pid == 0){
		//子进程
		for(i=0; i<5; i++ ){
			sleep(2);
			printf("I'am in child\n");
		}
	}else if(pid > 0){
		//父进程
		for(i=0; i<5; i++){
			sleep(2);
			printf("I'am in parent\n");
		}
	}

	return 0;
}

