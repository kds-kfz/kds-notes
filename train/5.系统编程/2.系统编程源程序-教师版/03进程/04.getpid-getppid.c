#include <stdio.h>
#include <unistd.h>

//fork 创建子进程
//getpid 获取自身进程id
//getppid  获取父进程id

//父进程循环创建５个子进程    (6)

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
			printf("I'am in child, pid = %d, ppid = %d\n",getpid(),getppid());
		}
	}else if(pid > 0){
		//父进程
		for(i=0; i<5; i++){
			sleep(2);
			printf("I'am in parent, pid = %d\n",getpid());
		}
	}

	return 0;
}

