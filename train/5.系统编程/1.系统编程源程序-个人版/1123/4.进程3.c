#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
//4.进程3.c
//进程间可以共享全局变量和正文段(如文本编译器，c编译器和shell等)，
//但进程间数据，堆，栈区独立
int num=999;
int main(){
    printf("before fork...\n");
    int i;
    pid_t pid=fork();
    if(pid==-1){
	printf("fork error\n");
	exit(-1);
    }
    else if(pid==0){//子进程，修改全局变量
	//sleep(1);
	//不要在子进程加延时，否则会被脚本抢断时间片
	num=2;	    
	printf("child_pid=%d,father_pid=%d\n",getpid(),getppid());//6935,6934
	printf("in child num = %d\n",num);
    }
    else if(pid>0){//父进程，访问不到子进程已修改的值
//	num=11;
	sleep(1);//必须加延时，不然获得的不是同个父进程pid
	printf("father_pid=%d\n",getpid());//6934
	printf("in parent num = %d\n",num);//99
    }
    return 0;
}
