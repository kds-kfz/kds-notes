#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>

#define MAXLINE 512

int main(int argc, char* argv[])
{
	char buf[MAXLINE];

	if(argc != 3){
		printf("缺少操作数\n");
		exit(-1);
	}
	//读
	int fdR = open(argv[1],O_RDONLY);
	if(fdR == -1){
		printf("open argv[1] error\n");
		exit(-1);
	}
	//写
	int fdW = open(argv[2],O_WRONLY|O_CREAT|O_TRUNC,0640);
	if(fdW == -1){
		printf("open argv[2] error\n");
		exit(-1);
	}
	
	int count = 0;

	while(1){
		printf("count = %d\n",++count);
		int ret = read(fdR,buf,sizeof(buf));
		if(ret == 0){
			printf("读到文件末尾\n");
			break;
		}else if(ret == -1){
			printf("read error\n");
		}
		printf("ret = %d\n",ret);
		ret = write(fdW,buf,ret);
		if(ret == -1){
			printf("write error\n");
		}		
	}

	close(fdR);
	close(fdW);
	
	return 0;
}
