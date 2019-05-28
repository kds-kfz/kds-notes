#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/mman.h>
//2.mmap-r.c
int num1=100;
typedef struct{
    int id;
    char name[10];
}Stu;
int main(){
    int fd=open("mmap.txt",O_RDONLY);
    if(fd== -1){
	perror("open error\n");
	exit(-1);
    }
    #if 0
    //循环读取1个整型
    ftruncate(fd,4);
    int *mm=mmap(NULL,4,PROT_READ,MAP_SHARED,fd,0);
    if(mm==MAP_FAILED){
	perror("mmap fail\n");
	exit(-1);
    }
    close(fd);
    while(1){
	printf("read *mm = %d\n",*mm);
	sleep(3);
    }
    int ret=munmap(mm,4);
    if(ret==-1){
	perror("munmap fail\n");
    }
    #endif
    #if 0
    //循环读取一个字符串
    ftruncate(fd,10);
    char *mm=mmap(NULL,10,PROT_READ,MAP_SHARED,fd,0);
    if(mm==MAP_FAILED){
	perror("mmap fail\n");
	exit(-1);
    }
    close(fd);
    while(1){
	printf("read *mm = %s\n",mm);
	sleep(2);
    }
    int ret=munmap(mm,10);
    if(ret==-1){
	perror("munmap fail\n");
    }
    #endif
    #if 1
    //循环读取一个结构体
    ftruncate(fd,sizeof(Stu));
    Stu *mm=mmap(NULL,sizeof(Stu),PROT_READ,MAP_SHARED,fd,0);
    if(mm==MAP_FAILED){
	perror("mmap fail\n");
	exit(-1);
    }
    close(fd);
    while(1){
	printf("read id = %d,name = %s\n",mm->id,mm->name);
	sleep(2);
    }
    int ret=munmap(mm,sizeof(Stu));
    if(ret==-1){
	perror("munmap fail\n");
    }
    #endif
    return 0;
}

