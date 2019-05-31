#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>
//3.复制文件描述符.c
#if 0
int dup(int oldfd);
//返回当前可用最小文件描述符,这样可以多个文件描述符共享一个v节点i节点
int dup2(int oldfd, int newfd);//可以指定哪个文件描述符
int dup3(int oldfd, int newfd, int flags);
#endif

int main(int argc,char *argv[]){
    char buf1[20]="hello";
    char buf2[20]="world";
    if(argc!=2){
	printf("参数错误\n");
	exit(-1);
    }
    int fd=open(argv[1],O_WRONLY|O_CREAT|O_TRUNC,0640);
    if(fd==-1){
	printf("open error\n");
	exit(-1);
    }
    int ret=write(fd,buf1,strlen(buf1));
    if(ret==-1){
	printf("write buf1 error\n");
	exit(-1);
    }
    int newfd=dup(fd);
//    int newfd=open(argv[1],O_WRONLY|O_CREAT|O_APPEND,0640);
    if(newfd==-1){
	printf("udp error\n");
	exit(-1);
    }
    ret=write(newfd,buf2,strlen(buf2));
    if(ret==-1){
	printf("write buf2 error\n");
	exit(-1);
    }
    printf("fd=%d,newfd=%d\n",fd,newfd);//虽然文件描述符不同，但指向的同一个文件
    close(fd);
    close(newfd);
    return 0;
}

