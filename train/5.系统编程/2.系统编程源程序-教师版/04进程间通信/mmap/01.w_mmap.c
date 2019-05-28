#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>

struct STU{
	char name[20];
	int id;
};

int main()
{
	struct STU st = {"lisi",1001};

	int fd = open("mmap",O_RDWR|O_CREAT,0777);
	if(fd == -1){
		perror("open");
		exit(-1);
	}
	ftruncate(fd,sizeof(st));
	struct STU *mm = mmap(NULL,sizeof(st),PROT_WRITE,MAP_SHARED,fd,0);
	if(mm == MAP_FAILED){
		perror("mmap");
		exit(-1);
	}
	close(fd);
	while(1){
			st.id++;
			memcpy(mm,&st,sizeof(st));
			printf("name = %s,id = %d\n",mm->name,mm->id);
			sleep(2);
	}
	int ret = munmap(mm,sizeof(st));
	if(ret == -1){
		perror("munmap");
		exit(-1);
	}

	return 0;
}
