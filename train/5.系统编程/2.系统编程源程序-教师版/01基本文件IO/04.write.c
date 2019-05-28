#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
/*
	O_RDONLY   只读
	O_WRONLY   只写
	O_RDWR     读写
	O_TRUNC    清空
	O_APPEND   追加
	O_EXCL     判断文件是否存在
	O_CREAT    创建
 
 */
#define MAXLINE 512

int main()
{
	char buf[MAXLINE] = "qwerty";
	int fd = open("a.txt",O_WRONLY|O_CREAT|O_EXCL,0777);
	/*以为O_CREAT|O_EXCL创建一个文件时,如果文件存在则失败*/
	//int fd = open("a.txt",O_WRONLY|O_CREAT|O_TRUNC,0640);
	if(fd == -1){
		fprintf(stderr,"open failed\n");
		exit(-1);
	}
	
	int ret = write(fd,buf,strlen(buf));
	if(ret == -1){
		fprintf(stderr,"write error\n");
		exit(-1);
	}

	close(fd);
	return 0;
}
