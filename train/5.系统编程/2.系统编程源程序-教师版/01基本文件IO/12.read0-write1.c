#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main()
{
	char buf[20];
	int ret = read(0,buf,sizeof(buf)-1);
	if(ret == -1){
		printf("read error\n");
	}
	buf[ret] = '\0';
	printf("buf = %s",buf);
	
	printf("----------------\n");

	ret = write(1,buf,strlen(buf));
	if(ret == -1){
	
	}

	return 0;
}
