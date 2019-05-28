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

	int fd = open("temp",O_RDWR|O_CREAT|O_TRUNC,0640);
	if(fd == -1){
		perror("open");
		exit(-1);
	}
	unlink("temp");//删除临时文件目录项,使之具备释放条件
	ftruncate(fd,4);
	mm = (int*)mmap(NULL,4,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	//mm = (int*)mmap(NULL,4,PROT_READ|PROT_WRITE,MAP_PRIVATE,fd,0);在进程间可以使用private
	if(mm == MAP_FAILED)
		sys_err("mmap");
	close(fd);//当映射区创建完毕,即可关闭文件描述符

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

		int ret = munmap(mm,4);//释放映射区
		if(ret == -1){
			printf("munmap error");
			exit(-1);
		}
	}
	
	return 0;
}

















