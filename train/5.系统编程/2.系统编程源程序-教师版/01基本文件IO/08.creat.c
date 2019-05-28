#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char* argv[])
{	
	char buf[20] = "hello";
	int fd = creat(argv[1],0640);
	//open(argv[1],O_WRONLY|O_CREAT|O_TRUNC,0640)
	if(fd == -1){
		printf("creat error\n");		
	}
	int ret = write(fd,buf,strlen(buf));	
	if(ret == -1){
		printf("write error\n");
	}
	
	/*
	int ret = read(fd,buf,sizeof(buf)-1);
	if(ret == -1){
		printf("read error\n");
	}
	*/
	printf("fd = %d\n",fd);

	return 0;
}
