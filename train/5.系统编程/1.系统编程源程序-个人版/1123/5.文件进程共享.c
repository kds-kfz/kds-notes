#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>
//5.文件进程共享.c
#if 0
    在父进程fork之前打开一个文件
    在父进程中写入hello
    在子进程中写入world
    查看文件的内容
#endif
    //父进程和子进程共享同一个文件偏移量
int main(){
    char buf1[]="hello";
    char buf2[]="world";
    char buf3[20];
    int ret=0;
    int fd=open("file.txt",O_RDWR|O_CREAT|O_TRUNC,0640);
    if(fd==-1){
	printf("open error\n");
	exit(-1);
    }
    pid_t pid=fork();
    if(pid==-1){
	printf("fork error\n");
	exit(-1);
    }
    else if(pid==0){
	ret=write(fd,buf2,sizeof(buf2)-1);
	printf("in child pid = %d,parent pid = %d,fd = %d\n",getpid(),getppid(),fd);
	if(ret==-1){
	    printf("write buf1 error\n");
	    exit(-1);
	}
    }
    else{
	ret=write(fd,buf1,sizeof(buf1)-1);
	printf("parent pid = %d,fd = %d\n",getpid(),fd);
	if(ret==-1){
	    printf("write buf2 error\n");
	    exit(-1);
	}
	sleep(3);
	lseek(fd,0,SEEK_SET);
	ret=read(fd,buf3,sizeof(buf3)-1);
	if(ret==-1){
	    printf("read fd error\n");
	    exit(-1);
	}
	close(fd);
	buf3[ret]='\0';
	printf("buf3 = %s\n",buf3);
    }
    return 0;
}
