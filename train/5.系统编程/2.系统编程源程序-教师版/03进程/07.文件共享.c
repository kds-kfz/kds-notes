#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

/*
	在父进程(fork之前)打开一个文件
	...
 */

int num = 10;

int main()
{
	printf("before fork...\n");
	int i;
	int ret;
	int fd = open("fork",O_RDWR|O_CREAT|O_TRUNC,0640);
	if(fd == -1){
		printf("open error\n");
	}

	pid_t pid = fork();
	if(pid == -1){
		printf("创建子进程失败\n");
		exit(-1);
	}else if(pid == 0){
		//子进程
		printf("in child: pid = %d, ppid = %d,fd = %d\n",getpid(),getppid(),fd);
		char buf1[20] = "hello";
		ret = write(fd,buf1,strlen(buf1));
		if(ret == -1){
		
		}
	}else{
		//父进程
		sleep(1);
		printf("in parent: pid = %d, fd = %d\n",getpid(),fd);
		char buf2[20] = "world";
		ret = write(fd,buf2,strlen(buf2));
		if(ret == -1){
		
		}

	}
	
	if(pid > 0){
		sleep(5);
		char buf3[20];
		lseek(fd,0,SEEK_SET);
		ret = read(fd,buf3,sizeof(buf3)-1);
		if(ret == -1){
		
		}
		buf3[ret] = '\0';
		printf("fork:%s",buf3);
	}
	close(fd);
	
	return 0;
}

