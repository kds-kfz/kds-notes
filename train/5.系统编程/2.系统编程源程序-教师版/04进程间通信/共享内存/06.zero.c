#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

int var = 100;

void sys_err(char *str)
{
	perror(str);
	exit(-1);
}

int main()
{
	int *mm;
	pid_t pid;
	int fd = open("/dev/zero",O_RDWR);
	mm = (int*)mmap(NULL,4,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	if(mm == MAP_FAILED)
		sys_err("mmap");

	pid = fork();
	if(pid == 0){
		*mm = 200;
		var = 300;
		printf("in child:pid = %d,ppid = %d\n",getpid(),getppid());
		printf("in child:*p = %d,var = %d\n",*mm,var);
	}else if(pid > 0){
		sleep(1);
		printf("in parent:pid = %d\n",getpid());
		printf("in child:*p = %d,var = %d\n",*mm,var);
		wait(NULL);
		close(fd);
		int ret = munmap(mm,4);//释放映射区
		if(ret == -1){
			printf("munmap error");
			exit(-1);
		}
	}
	
	return 0;
}

