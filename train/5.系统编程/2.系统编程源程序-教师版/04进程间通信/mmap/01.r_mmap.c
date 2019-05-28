#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

struct STU{
	char name[20];
	int id;
};

int main()
{
	int fd = open("mmap",O_RDONLY);
	if(fd == -1){
		perror("open");
		exit(-1);
	}

	struct STU *mm = mmap(NULL,sizeof(struct STU),PROT_READ,MAP_SHARED,fd,0);
	if(mm == MAP_FAILED){
		perror("mmap");
		exit(-1);
	}
	close(fd);
	
	while(1){
		sleep(2);
		printf("name = %s,id = %d\n",mm->name,mm->id);
	}
	int ret = munmap(mm,sizeof(struct STU));
	if(ret == -1){
		printf("munmap error\n");
		exit(-1);
	}

	return 0;
}
