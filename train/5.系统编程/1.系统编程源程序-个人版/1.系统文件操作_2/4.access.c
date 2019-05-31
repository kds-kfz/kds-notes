#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>
//4.access.c
#if 0
//所有用户
//当前用户u+分组用户g+其它用户o
//获取文件是否具有某种权限(当前用户)
    int access(const char *pathname, int mode);
    pathname:文件路径
    mode:
    R_OK:读
    W_OK:写
    X_OK:执行
    F_OK:存在
    return value:返回值：失败-1,成功0
#endif

int  main(){
    int ret=access("access.txt",F_OK);
    if(ret==-1)
	printf("no file\n");
    else
	printf("file ok\n");
    /*
    int fd=open("access.txt",O_RDONLY);
    if(fd==-1)
	printf("open error\n");
    else
	printf("read ok\n");
    */
    ret=access("access.txt",R_OK);
    if(ret==-1)
	printf("read error\n");
    else
	printf("read ok\n");

    ret=access("access.txt",W_OK);
    if(ret==-1)
	printf("write error\n");
    else
	printf("write ok\n");
    ret=access("access.txt",X_OK);
    if(ret==-1)
	printf("execute error\n");
    else
	printf("execute ok\n");
    return 0;
}
