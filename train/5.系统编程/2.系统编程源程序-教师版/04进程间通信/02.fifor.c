#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>


int main()
{
	int fd = open("fifo",O_RDONLY);
	if(fd == -1){
	
	}
	char buf[20];
	while(1){
		int ret = read(fd,buf,sizeof(buf)-1);
		if(ret == -1){
			perror("read");	
		}
		if(ret == 0)
			break;
		printf("ret = %d\n",ret);
		buf[ret] = '\0';
		printf("buf=%s\n",buf);
		sleep(1);
	}
	return 0;
}
