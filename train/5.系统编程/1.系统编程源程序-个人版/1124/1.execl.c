#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
//1.execlp.c
#if 0
执行新程序：
    当掉哟个一种exce函数时，将丢弃旧哟有的程序，用磁盘上的新程序替换类当前进程的正文段
数据段，堆段和栈段，新函数则从main函数开始执行

    调用exec并不创建新进程，前后的进程id未改变
    execl
    execlp
    execle
    execv
    execve
    execvp
    1.最后2个字母中有l，传给新程序函数第二个参数为：
    argv0,argv1...NULL(默认是从当前文件夹查找，如果找不到，依旧执行之前的进程)
    2.最后2个字母中有p，传"可执行程序"参数时不用写路径，
    只写程序名(默认是在环境变量中寻找)
    3.最后2个字母中有v，传给新程序函数第二个参数为argv[]
    4.最后2个字母中有e，传给新程序环境表

    int execl(const char *path, const char *arg, .../* (char  *) NULL */);
    int execv(const char *path, char *const argv[]);
    int execle(const char *path, const char *arg, ...
	    /*, (char *) NULL, char * const envp[] */);
//    int execve(const char *path, const char argv[],char *const envp[]);
    
    int execlp(const char *file, const char *arg, .../* (char  *) NULL */);
    int execvp(const char *file, char *const argv[]);
    int execvpe(const char *file, char *const argv[],char *const envp[]);
  
    前4个函数取路径名作为参数，后3个函数取文件名作为参数
    如果路径名包含/，视为路径名
    execlp,execvp使用路径前最中的一个找到了可执行文件，如果该文件不是由编译器产生的，
    就认为是shell脚本

    echo $PATH
    /usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin
    execlp("ls","ls","-l",NULL);
#endif

int main(int argc,char *argv[]){
    pid_t pid=fork();
    char *buf[]={"ls","-l",NULL};
    if(pid==-1){
	printf("fork error");
	exit(-1);
    }
    else if(pid==0){
	printf("in child pid=%d,parent pid=%d\n",getpid(),getppid());
//	execl("hello","./hello",NULL);//从当前目录找可执行文件
//	execl("/bin/ls","ls","-l",NULL);
//	execlp("ls","ls","-l",NULL);//从环境变量里找
//	execv("hello",argv);
	execvp("ls",buf);
	printf("2017/11/24\n");
	}
    else{
	printf("in parent pid=%d\n",getpid());
	sleep(1);
	}
    return 0;
}

