#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
//4.文件io操作2.c
/*
 O_RDONLY   只读
 o_WRONLY   只写
 O_CREART   创建（文件加权限）
 O_TRUNC    清空
 O_EXCL	    判断文件是否存在
 O_APPEND   追加写
 O_RDWR	    读写
 stdin	STDIN_FIFONO	标准输入
 stdout	STDOUT_FIFENO	标准输出
 stderr	STDERR_FIFENO	标准错误
 */
int main(){
#if 1
    char buf[512];
    int fd=open("file2.txt",O_RDWR|O_APPEND);
    //读写追加形式，文件指针在文件末尾
//    int fd=open("2.txt",O_RDWR);
    //读写形式，文件指针在开头
    if(fd==-1){
	printf("open error\n");
	exit(-1);
    }

    strcpy(buf,"abcd");
    int ret=write(fd,buf,strlen(buf));
    if(ret==-1){
	printf("write error\n");
	exit(-1);
    }
    lseek(fd,0,SEEK_SET);//移动文件指针到文件开头
    ret=read(fd,buf,sizeof(buf)-1);
    if(ret==-1){
	printf("read error\n");
	exit(-1);
    }
    buf[ret]='\0';//需手动加'\0'，否则是随机字符
    printf("buf = %s\n",buf);
#endif
    return 0;
}
