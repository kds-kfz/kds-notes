#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
/*
 O_RDONLY 　只读
 O_WRONLY　　只写　
 O_CREART　　创建　(加文件权限)
 O_TRUNC　　清空
 O_EXCL　　　判断文件是否存在
 O_APPEND  　追加写
 */
/*
stdin      STDIN_FIFENO   标准输入
stdout     STDOUT_FIFENO 　标准输出
stderr     STDERR_FIFENO　　标准错误
*/
#define MAXLINE 512

int main()
{
	//当前可用最小文件描述符
	char buf[MAXLINE];
	int fd = open("open",O_RDONLY|O_TRUNC);
	if(fd == -1){
		fprintf(stderr,"open error");
	}
	printf("fd = %d\n",fd);
	int ret = read(fd,buf,sizeof(buf)-1);
	if(ret == -1){
		fprintf(stderr,"read error\n");
	}
	buf[ret] = '\0';	
	printf("buf = %s\n",buf);

	close(fd);
	return 0;
}
