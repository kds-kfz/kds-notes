#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>

struct STU{
	char name[20];
	int id;
};

int main()
{
	pid_t pid;
	//STU student = {"lisi",1001};
	
	char buf[] = "hello";
	
	int fd = open("mmap",O_RDWR|O_CREAT,0640);
	if(fd == -1){
		perror("mmap");
		exit(-1);
	}
	ftruncate(fd,sizeof(buf));
	char* mm = mmap(NULL,sizeof(buf),PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	if(mm == MAP_FAILED){
		perror("mmap");
		exit(-1);
	}
	close(fd);
	//创建进程
	pid = fork();
	if(pid == -1){
		printf("fork error\n");
		exit(-1);
	}else if(pid == 0){
		student.id = 1002;
		//memcpy(mm,&student,sizeof(student));
		//mm->id = 1002;
		strcpy(mm,"world");
		printf("in child:pid = %d,ppid = %d\t",getpid(),getppid());
		printf("buf = %s\n",buf);
	}else {
		sleep(2);
		printf("buf = %s\n",buf);
		munmap(mm,sizeof(buf));
	}
	return 0;
}
