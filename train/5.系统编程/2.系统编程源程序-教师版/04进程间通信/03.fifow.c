#include <sys/types.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

int main()
{

	int ret = mkfifo("fifo",0640);
	if(ret == -1){
		printf("mkfifo error\n");
	}
	printf("mkfifo success...\n");
	
	int fd = open("fifo",O_WRONLY);
	if(fd == -1){
	
	}
	char buf[20] = "hello";
	
	while(1){
		ret = write(fd,buf,strlen(buf));
		if(ret == -1){
	
		}
		sleep(2);
	}
	return 0;
}
