#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include <sys/mman.h>
//3.mmap-fork.c
typedef struct{
    int id;
    char name[6];
}Stu;
//在子进程中修改磁盘文件映射在内存的地址，以达到修改磁盘文件，
//当子进程中修改了磁盘文件的内容，父进程访问磁盘文件时，也是修改后最新的内容 
int main(){
    pid_t pid;
    Stu s1={1001,"lisi"};
    int fd=open("mmap.txt",O_RDWR|O_CREAT,0640);
    if(fd==-1){
	perror("mmap error\n");
	exit(-1);
    }
    ftruncate(fd,sizeof(Stu));
    Stu *mm=mmap(NULL,sizeof(Stu),PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
    if(mm==MAP_FAILED){
	perror("mmap fail\n");
	exit(-1);
    }
    close(fd);
    //创建进程
    if((pid=fork())==-1){
	perror("fork fail\n");
	exit(-1);
    }
    #if 0
    else if(pid==0){
	memcpy(mm,&s1,sizeof(Stu));
	mm->id=1002;
	printf("in child pid = %d,parent pid = %d\n",getpid(),getppid());
	printf("name = %s,id = %d\n",mm->name,mm->id);
    }
    else{
	sleep(2);
	printf("in parent pid = %d\n",getpid());
	int ret=munmap(mm,sizeof(Stu));
	if(ret==-1){
	    perror("mnumap fail\n");
	}
    }
    #endif
    #if 1
    else if(pid==0){
	strcpy(mm->name,"huang");
	mm->id=1002;
	printf("in child pid = %d,parent pid = %d\n",getpid(),getppid());
	printf("id = %d,name = %s\n",mm->id,mm->name);
    }
    else{
	printf("sleep 5s ...\n");
	sleep(5);
	//延时5s，子进程获取时间片，执行子进程代码，时间结束后，执行父进程代码
	printf("in parent pid = %d\n",getpid());
	memcpy(mm,&s1,sizeof(Stu));
	printf("id = %d,name = %s\n",mm->id,mm->name);
	int ret=munmap(mm,sizeof(Stu));
	if(ret==-1){
	    perror("mnumap fail\n");
	}
	printf("munmap ok\n");
    }
    #endif
    return 0;
}
