/*用于write的mmap*/
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdlib.h>

struct STU{
	int id;
	char name[20];
	char sex;
};

void sys_err(char *str)
{
	perror("str");
	exit(-1);
}

//创建映射区的权限应该小于等于打开文件的权限
//创建映射区的时候，隐含这对文件的读操作
int main(int argc, char* argv[])
{
	int fd;
	struct STU student = {1001,"lisi",'m'};
	char *mm;
	fd = open(argv[1],O_RDWR|O_CREAT,0640);//也可以用lseek获取文件大小
	ftruncate(fd,sizeof(student));
	mm = mmap(NULL,sizeof(student),PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	if(mm == MAP_FAILED)
		sys_err("mmap");
	while(1){
		memcpy(mm,&student,sizeof(student));
		student.id++;
		sleep(1);
	}
	
	close(fd);//文件描述符关闭对mmap的操作没有影响	
	munmap(mm,sizeof(student));

	return 0;
}





















