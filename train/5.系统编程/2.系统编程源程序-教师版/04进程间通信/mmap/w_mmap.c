#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>

int main()
{
	int i;
	char *buf[5] = {"liyi","lier","lisan","lisi","liwu"};
	int fd = open("mmap",O_RDWR|O_CREAT,0777);
	if(fd == -1){
		perror("open");
		exit(-1);
	}
	ftruncate(fd,sizeof(buf));
	char *mm = mmap(NULL,sizeof(buf),PROT_WRITE,MAP_SHARED,fd,0);
	if(mm == MAP_FAILED){
		perror("mmap");
		exit(-1);
	}
	close(fd);
	while(1){
		for(i=0; i<5; i++){
			strcpy(mm,buf[i]);
			printf("mm = %s\n",mm);
			sleep(2);
		}
	}
	int ret = munmap(mm,sizeof(buf));
	if(ret == -1){
		perror("munmap");
		exit(-1);
	}

	return 0;
}
