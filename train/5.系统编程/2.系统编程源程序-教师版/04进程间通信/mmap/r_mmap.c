#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

int var = 100;

int main()
{
	int fd = open("mmap",O_RDONLY);
	if(fd == -1){
		perror("open");
		exit(-1);
	}

	char *mm = mmap(NULL,5,PROT_READ,MAP_SHARED,fd,0);
	if(mm == MAP_FAILED){
		perror("mmap");
		exit(-1);
	}
	close(fd);
	
	while(1){
		sleep(2);
		printf("mm ->= %s\n",mm);
	}
	int ret = munmap(mm,5);
	if(ret == -1){
		printf("munmap error\n");
		exit(-1);
	}

	return 0;
}
