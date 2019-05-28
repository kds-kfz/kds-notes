#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

int main()
{
	int fd = open("mmap",O_RDONLY);
	if(fd == -1){
	
	}
	
	int *mm = mmap(NULL,4,PROT_READ,MAP_SHARED,fd,0);
	if(mm == MAP_FAILED){
		printf("mmap error");		
	}

	close(fd);
	printf("*mm = %d\n",*mm);
	
	int ret = munmap(mm,4);
	if(ret == -1){
		
	}
	
	return 0;
}

