#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
//7.creat.c
int main(int argc,char *argv[]){
    char buf[20]="hello world";
    int fd=creat(argv[1],0640);
//    open(argv[1],O_WRONLY|O_CREAT|O_TRUNC,0640);
    if(fd==-1){
	printf("creat error\n");
	exit(-1);
    }
    int ret=write(fd,buf,strlen(buf));
    if(ret==-1){
	printf("wtite error\n");
	exit(-1);
    }
    /*
    ret=read(fd,buf,sizeof(buf)-1);
    if(ret==-1){
	printf("read error\n");
	exit(-1);
    }
    buf[ret]='\0';
    */
    printf("fd = %d\n",fd);
    return 0;
}

