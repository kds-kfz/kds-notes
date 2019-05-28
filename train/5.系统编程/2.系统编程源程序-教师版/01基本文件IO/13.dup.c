#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char* argv[])
{
	char buf1[] = "hello";
	char buf2[] = "world";

	if(argc < 2){
		printf("输入参数缺少\n");
	}

	int fd = open(argv[1],O_WRONLY|O_CREAT|O_TRUNC,0640);
	if(fd == -1){
		printf("open error\n");
	}
	int ret = write(fd,buf1,strlen(buf1));
	if(ret == -1){
		printf("write1 error\n");
	}

	int newFd = dup(fd);
	
	printf("oldfd = %d\n",fd);
	printf("newfd = %d\n",newFd);
	
	ret = write(newFd,buf2,strlen(buf2));
	if(ret == -1){
		printf("write2 error\n");
	}
	close(fd);
	close(newFd);
	
	return 0;	
}
