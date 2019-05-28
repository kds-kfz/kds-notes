#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
/*
	O_RDONLY  
	O_WRONLY
	O_RDWR
	O_CREAT
	O_TRUNC
	O_EXCL
	O_APPEND
 */

#define MAXLINE 512

int main()
{
	char buf[MAXLINE];
	int fd = open("a.txt",O_RDWR|O_APPEND);
	//int fd = open("a.txt",O_RDWR);
	if(fd == -1){
		printf("open error\n");
		exit(-1);
	}

	strcpy(buf,"abcd");
	int ret = write(fd,buf,strlen(buf));	
	if(ret == -1){
		printf("write error\n");
		exit(-1);
	}



	lseek(fd,0,SEEK_SET);


	ret = read(fd,buf,sizeof(buf)-1);
	if(ret == -1){
		printf("read error");
		exit(-1);
	}
	buf[ret] = '\0';
	printf("buf = %s\n",buf);

	return 0;
}
