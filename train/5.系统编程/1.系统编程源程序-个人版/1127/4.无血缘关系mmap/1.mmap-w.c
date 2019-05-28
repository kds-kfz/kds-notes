#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/mman.h>
//1.mmap-w.c
int num1=100;
typedef struct{
    int id;
    char name[10];
}Stu;
int main(){
    int fd=open("mmap.txt",O_RDWR|O_CREAT,0640);
    if(fd== -1){
	perror("open error\n");
	exit(-1);
    }
    #if 0
    //循环写入一个整型
    ftruncate(fd,4);
    int *mm=mmap(NULL,4,PROT_WRITE,MAP_SHARED,fd,0);
    if(mm==MAP_FAILED){
	perror("mmap fail\n");
	exit(-1);
    }
    close(fd);
    while(1){
	(*mm)++;
	sleep(2);
    }
    num1=200;
    int ret=munmap(mm,4);
    if(ret==-1){
	perror("munmap fail\n");
    }
    #endif
    #if 0
    //循环写入一个字符串
    char *buf[5]={"hello","red","yellow","operator","blue"};
    ftruncate(fd,sizeof(buf));
    char *mm=mmap(NULL,sizeof(buf),PROT_WRITE,MAP_SHARED,fd,0);
    if(mm==MAP_FAILED){
	perror("mmap fail\n");
	exit(-1);
    }
    close(fd);
    while(1){
	for(int i=0;i<5;i++){
	    memcpy(mm,buf[i],sizeof(buf[i]));
	    printf("write : %s\n",mm);
	    sleep(2);
	}
    }
    int ret=munmap(mm,sizeof(buf));
    if(ret==-1){
	perror("munmap fail\n");
    }
    #endif
    #if 1
    //循环写入一个结构体
    Stu s1={1001,"lisi"};
    ftruncate(fd,sizeof(Stu));
    Stu *mm=mmap(NULL,sizeof(Stu),PROT_WRITE,MAP_SHARED,fd,0);
    if(mm==MAP_FAILED){
	perror("mmap fail\n");
	exit(-1);
    }
    close(fd);
    while(1){
	s1.id++;
	memcpy(mm,&s1,sizeof(Stu));
	printf("write id = %d,name = %s\n",mm->id,mm->name);
	sleep(2);
    }
    int ret=munmap(mm,sizeof(Stu));
    if(ret==-1){
	perror("munmap fail\n");
    }
    #endif
    return 0;
}

