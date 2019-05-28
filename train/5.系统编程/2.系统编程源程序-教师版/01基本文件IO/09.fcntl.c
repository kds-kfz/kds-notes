#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

/*
 fcntl(fd,cmd,.../cmd /);
 获取/设置文件状态标志
 cmd = F_GETFL和F_SETFL
 F_GETFL返回文件状态标志
 F_SETFL将文件标志设置为第３个参数的值
 (取为整形数值)　例如增加O_APPEND
 */

int main(int argc, char* argv[])
{
	char buf[20] = "hello";

	if(argc < 2){
		printf("input error\n");
		exit(-1);
	}
	int fd = open(argv[1],O_WRONLY|O_CREAT|O_TRUNC,0640);
	if(fd == -1){
		printf("open error\n");
	}
	int ret = write(fd,buf,strlen(buf));
	if(ret == -1){
		printf("write error\n");
	}
	lseek(fd,0,SEEK_SET);
	
	/*
	int flag = fcntl(fd,F_GETFL);
	flag |= O_APPEND;
	flag = fcntl(fd,F_SETFL,flag);
	*/

	char buf1[20] = "world";
	ret = write(fd,buf1,strlen(buf1));
	if(ret == -1){
		printf("write1 error\n");
	}

	close(fd);
	return 0;
}
