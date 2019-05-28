#include <string.h>
#include <sys/mman.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

//void *mmap(NULL,4,MAP_READ|MAP_WRITE,MAP_SHARED,fd,0)

int main(int argc, char* argv[])
{
	if(argc < 2){
		fprintf(stdout,"argc less\n");
		exit(-1);
	}
	int fd = open(argv[1],O_RDWR|O_CREAT,0640);
	if(fd == -1){
		fprintf(stdout,"open error\n");
		exit(-1);
	}
	ftruncate(fd,4);
	int *mm = mmap(NULL,4,PROT_WRITE,MAP_SHARED,fd,0);
	if(mm == MAP_FAILED){
		printf("mmap error\n");
		exit(-1);
	}
	close(fd);

	*mm = 20;

	int ret = munmap(mm,4);
	if(ret == -1){
		printf("munmap error");
	}

	return 0;
}
