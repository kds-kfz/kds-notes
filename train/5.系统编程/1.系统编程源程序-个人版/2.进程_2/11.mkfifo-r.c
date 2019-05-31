#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<string.h>
#include<fcntl.h>
//11.mkfifo-r.c
#if 0
    //有名管道
    int mkfifo(const char *pathname, mode_t mode);
    返回值:成功0,错误-1
#endif
//gcc 11.mkfifo-r.c -o r
//开两个终端，第一个终端运行./w
//第二个终端运行./r
//当运行./w时，会等待./r读取，最后结束
//mkfifo，共享同一片物理内存里的文件，实现多进程间通信
int main(){
    int ret;
    int fd=open("fifo.txt",O_RDONLY);
    if(fd==-1){
	printf("open fail\n");   
    }
    char buf[20]={};
    ret=read(fd,buf,sizeof(buf)-1);
    if(ret==-1){
	printf("read fail\n");   
    }
    close(fd);
    buf[ret]='\0';
    printf("buf[] = %s\n",buf);
    return 0;
}

