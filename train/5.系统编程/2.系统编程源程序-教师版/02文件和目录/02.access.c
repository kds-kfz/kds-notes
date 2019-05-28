#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

int main()
{
	//获取文件是否具有某种权限
	//int ret = access("access",F_OK);
	int ret = access("access",R_OK);
	if(ret == -1){
		printf("read access error\n");
	}else{
		printf("read access ok\n");
	}
	int fd = open("access",O_RDONLY);
	if(fd == -1){
		printf("open error\n");
	}else{
		printf("open ok\n");
	}

	return 0;
}
