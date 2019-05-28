#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main()
{
	pid_t pid;
	pid_t p;
	pid_t wid;
	int flag = 5;
	for(int i=0; i<5; i++){
		pid = fork();
		if(pid == 0)
			break;
		if(i == 3)
			p = pid;
	}
	
	if(pid == -1){
		printf("fork error\n");
	}else if(pid > 0){
		//in father
		sleep(1);
		printf("in father:pid = %d\n",getpid());
		printf("before wait...\n");
		sleep(20);
		//while(wait(NULL));
		
		//while(waitpid(-1,NULL,0)); // == wait(NULL)
		/*
		  参数pid:
			>0回收指定ＩＤ的进程
			-1回收任意子进程(相当于wait)
			<-1回收指定进程组内的任意子进程
		　参数3:0---->阻塞
		        WNOHANG----->非阻塞
		 */
		/*
		waitpid(p,NULL,0);
		while(1);
		*/
		do{
			wid = waitpid(-1,NULL,WNOHANG);		
			if(wid > 0){
				flag--;	
			}
		}while(flag);

		printf("after wait...\n");
	}else{
		//in child
		sleep(1);
		printf("in child:pid = %d,ppid = %d\n",getpid(),getppid());
	}

	return 0;
}
