#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<string.h>
#include<fcntl.h>
//10.mkfifo-w.c
#if 0
    //有名管道
    int mkfifo(const char *pathname, mode_t mode);
    返回值:成功0,错误-1
#endif
//gcc 10.mkfifo-w.c -o w
int main(){
    int ret=mkfifo("fifo.txt",0640);//创建一个管道文件
    if(ret==-1){//如果已经有对应文件，则返回错误-1
	printf("mkfifo fail\n");
    
    }
    printf("mkfifo success\n");
//    int fd=open("fifo.txt",O_WRONLY|O_CREAT|O_TRUNC,0640);
    int fd=open("fifo.txt",O_WRONLY);//管道文件已经创建，只需只写打开
    if(fd==-1){
	printf("open fail\n");
    }
    char buf[]="hello world";
    ret=write(fd,buf,strlen(buf));
    if(ret==-1){
	printf("write fail\n");
    }
    close(fd);//写完后关闭文件
    printf("write ok\n");
    return 0;
}
