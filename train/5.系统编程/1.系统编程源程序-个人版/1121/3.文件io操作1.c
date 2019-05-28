#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
//3.文件io操作1.c
/*
 O_RDONLY   只读
 o_WRONLY   只写
 O_CREART   创建（文件加权限）
 O_TRUNC    清空
 O_EXCL	    判断文件是否存在
 O_APPEND   追加写

 stdin	STDIN_FIFONO	标准输入
 stdout	STDOUT_FIFENO	标准输出
 stderr	STDERR_FIFENO	标准错误
 */
/*
    int open(const char *pathname, int flags);
    (文件路径，不写路径默认当前路径文件)文件名，打开形式
    flags:

    int open(const char *pathname, int flags, mode_t mode);
    (文件路径，不写路径默认当前路径文件)文件名，打开形式，是创建需加文件权限
    
    size_t read(int fd, void *buf, size_t count);
    文件描述符，读到buf，读的字节数
    读取成功，返回读写字节个数，错误返回-1

    size_t write(int fd, const void *buf, size_t count);
    文件描述符，从buf写到文件(文件描述符所指向的文件)，写入字节数
    写入成功，返回写入的字节数，错误返回-1
 */
int main(){
#if 1
    char buf[512];
    //当前可影最小文件描述符
//    int fd=open("file1.txt",O_WRONLY|O_CREAT|O_TRUNC,0777);
//    int fd=open("file1.txt",O_RDONLY);
    int fd=open("file1.txt",O_WRONLY|O_CREAT|O_EXCL,0777);
    //O_EXCL 文件存在返回-1
    if(fd==-1){
	fprintf(stderr,"open error\n");
	exit(-1);
    }
    printf("fd = %d\n",fd);
    
    printf("size_buf = %ld\n",sizeof(buf));//12
    int ret=read(fd,buf,sizeof(buf)-1);
    if(ret==-1){
	fprintf(stderr,"read error\n");
	exit(-1);
    }
    /*
     implicit
    缺少头文件，警告
    */
    printf("ret = %d\n",ret);//12
    buf[ret]='\0';
    printf("buf = %s",buf);
    close(fd);
#endif
#if 0
    char str[]="12345";
    char buf[512]="red 2017/11/21";
    int fd1=open("1.txt",O_RDONLY);
    int fd2=open("2.txt",O_WRONLY|O_CREAT|O_TRUNC,0640);
    if(fd2==-1){
	fprintf(stderr,"open error\n");
	exit(-1);
    }
    printf("fd1 = %d\n",fd1);//3
    printf("fd2 = %d\n",fd2);//4
    int ret=write(fd2,buf,strlen(buf));
    if(ret==-1){
	fprintf(stderr,"write error\n");
	exit(-1);
    }
    close(fd1);
    close(fd2);

    printf("strlen(str)=%ld\n",strlen(str));//5
    printf("sizeof(str)=%ld\n",sizeof(str));//6
    printf("str = %s\n",str);//6

#endif
    return 0;
}
