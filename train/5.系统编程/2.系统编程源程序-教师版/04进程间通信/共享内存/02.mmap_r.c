/*用于read的mmap*/
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

struct STU{
	int id;
	char name[20];
	char sex;
};	

void sys_err(char* str)
{
	perror("str");
	exit(-1);
}

int main(int argc, char* argv[])
{
	int fd;
	struct STU student;
	struct STU *mm;

	if(argc < 2){
		printf("input less than two\n");
		exit(-1);
	}
	fd = open(argv[1],O_RDONLY);
	if(fd == -1)
		sys_err("open");
	mm = mmap(NULL,sizeof(student),PROT_READ,MAP_SHARED,fd,0);
	if(mm == MAP_FAILED)
		sys_err("mmap");
	while(1){
		printf("id = %d,name = %s, sex = %c\n",mm->id,mm->name,mm->sex);
		sleep(2);
	}

	close(fd);
	munmap(mm,sizeof(student));

	return 0;
}
