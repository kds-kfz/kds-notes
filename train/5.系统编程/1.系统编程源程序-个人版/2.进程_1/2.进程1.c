#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>
//2.进程1.c
#if 0
1.程序是指编译好的二进制文件，在磁盘上，不占用系统资源（cpu 内存 打开的文件 设备）
2.进程是一个抽象的概念，与操作系统联系紧密，可以将进程理解为活跃的程序，
占用系统资源，在内存中执行（程序运行起来，产生一个进程）
gcc a.c
预处理，编译，汇编，链接 ---> 可执行的二进制文件
--->a.out
#endif

//3G-4G里，PCB:进程控制块
#if 0
    pid_t fork(void);
    返回值：子进程返回0,父进程返回子进程id
    错误，返回-1

    pid_t wait(int *status);
    wait：等待终止进程，获取终止进程状态
    waitpid：详细看1124文件夹waitpid
    execve：在进程中执行其它代码
    getpid：获取自身进程id
    getppid：获取父进程id
#endif
/*
//练习：打印
    parent id
    child id
    parent id
    child id
    parent id
    child id
    parent id
    child id
    parent id
    child id
*/
int main(){
#if 0
    int count=0;
    printf("before fork ...\n");
    pid_t pid=fork();
    if(pid==0){
	sleep(1);
	count++;
	printf("child id=%d\n",pid);
    }
    else if(pid>0){
	printf("parent id=%d\n",pid);
    }
    if(count==1){
    pid_t pid2=fork();
    if(pid2==0){
	count++;
	printf("child id2=%d\n",pid2);
    }
    else if(pid2>0)
	printf("parent id2=%d\n",pid2);
    }
    if(count==2){
    pid_t pid3=fork();
    if(pid3==0){
	count++;
	printf("child id3=%d\n",pid3);
    }
    else if(pid3>0)
	printf("parent id3=%d\n",pid3);
    }
#endif
    printf("before fork ...\n");
    pid_t pid=fork();
    if(pid==-1){
	    printf("fork error\n");
	    exit(-1);
    }
    if(pid==0){
	    for(int i=0;i<5;i++){
	        sleep(2);
	        printf("child id=%d\n",pid);
	    }
    }else if(pid>0){
	    for(int i=0;i<5;i++){
	        sleep(2);
	        printf("parent id=%d\n",pid);
	    }
    }
    return 0;
}
