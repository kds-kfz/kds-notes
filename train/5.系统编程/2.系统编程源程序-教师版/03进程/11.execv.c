/*
	执行新程序:
	当调用一种exec函数时,将丢弃旧有的程序,用磁盘上的新程序替换了
	当前进程的正文段,数据段,堆段和栈段,新函数则从其main函数开始执行
	调用exec并不创建新进程,前后的进程id并未改变
	execl
	execlp
	execle
	execv
	execve
	execvp

	(1)最后２个字母中有l,传给新程序主函数第二个参数为:argv0,argv1...NULL(默认是从当前文件夹下查找,如果找不到,依旧执行之前的进程)
	(2).................p,传"可执行程序"参数时不用写路径,只写程序名(默认是在环境变量中寻找)
	(3).................v,传给新程序主函数第二个参数为argv[]
	(4)................e,传给新程序环境表
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>

int main(int argc, char* argv[])
{
	pid_t pid;
	pid = fork();
	
	if(pid == -1){
		printf("create process failed\n");
		exit(-1);
	}else if(pid == 0){
		sleep(1);
		printf("before execl...\n");			
		//execlp("hello","./hello",NULL);		
		//char* argv[] = {"hello","./hello",NULL};
		//execv("ls",argv);
		//execv("hello",argv);
		//char *argv[] = {"ls","-l",NULL};
		//execvp("ls",argv);
		int fd = open("a.txt",O_WRONLY|O_CREAT|O_TRUNC,0640);
		if(fd == -1){
			printf("open error\n");
			exit(-1);
		}
		//dup(oldfd);
		int newfd =  dup2(fd,STDOUT_FILENO);
		printf("newfd = %d\n",newfd);
		/*
		dup2(oldfd,STDOUT_FILENO);
		0 STDIN_FILENO 
		1 STDOUT_FILENO  标准输出
		2 STDERR_FILENO
		*/
		execlp("ls","ls","-l",NULL);

		printf("child die...\n");	
		printf("after execl...\n");
	}else {
		printf("in father");
	}
	
	return 0;
}	






