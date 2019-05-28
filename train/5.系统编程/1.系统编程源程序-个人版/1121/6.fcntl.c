#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
//6.fcntl.c
#if 0
//int argc：命令行参数个数
//char *argv[]：二级指针，指针数组，下标从0开始
int main(int argc,char *argv[]){
    printf("argc=%d\n",argc);
    for(int i=0;i<argc;i++)
	printf("argv[%d] = %s\n",i,argv[i]);
    return 0;
}
#endif

#if 1
    #if 0
    int fcntl(int fd, int cmd, ... /* arg */ );
    获取/设置文件状态标志
    cmd = F_GETFL和F_SETFL
    F_GETFL：返回文件状态标志
    F_SETFL：将文件标志设置为第3个参数的值
    (取为整型数值)例如增加O_APPEND
    #endif
int main(int argc,char *argv[]){
    char buf[20]="hello";
    if(argc<2){
	printf("input error\n");
	exit(-1);
    }
    int fd=open(argv[1],O_WRONLY|O_CREAT|O_TRUNC,0640);
    if(fd==-1)
	printf("open error\n");

    int ret=write(fd,buf,strlen(buf));
    if(ret==-1)
	printf("write buf error\n");

    lseek(fd,0,SEEK_SET);
/*
    int flag=fcntl(fd,F_GETFL);
    flag|=O_APPEND;
    flag=fcntl(fd,F_SETFL,flag);
*/
    char buf1[]="world";
    ret=write(fd,buf1,strlen(buf1));
    if(ret==-1)
	printf("write buf1 error\n");

    close(fd);

    return 0;
#endif
}
