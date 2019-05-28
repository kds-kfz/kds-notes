#include <stdio.h>
#include <unistd.h>

//父进程循环创建５个子进程    (6)

int main()
{
	printf("before fork...\n");
	int i;
	pid_t pid;
	
	for(i=0; i<5; i++){	
		pid = fork();//创建子进程
		if(pid == 0)
			break;	
	}	
	if(pid == 0){
		sleep(1);
		printf("in child:pid = %d, ppid = %d\n",getpid(),getppid());
		sleep(10);
	}else{
		sleep(1);
		printf("in parent:pid = %d\n",getpid());
		sleep(10);
	}
	return 0;
}

