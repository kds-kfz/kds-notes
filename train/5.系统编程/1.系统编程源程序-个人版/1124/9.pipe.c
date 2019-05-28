#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<string.h>
//9.pipe.c
#if 0
    管道：
    1.其本质是一个伪文件
    2.由两个文件描述符引用，一个表示读端(fd[0]),一个表示写端(fd[1])
    3.规定数据从管道的写端流入，从读端流出
    注意：
    1.数据自己不能自己写
    2.数据一旦被读走，变在管道中不存在，不可反复读取(阻塞形式)
    3.数据只能在一个方向上流动
    4.有血缘关系的进程间通信
#endif
#if 0
    //无名管道
    int pipe(int pipefd[2]);
    返回值：成功返回0,错误返回-1
#endif
int main(){
    pid_t pid;
    int fd[2];//fd[0]读端，fd[1]写端
    int ret;
    char buf1[20]="hello world";
    char buf2[20];
    ret=pipe(fd);//创建管道文件
    if(ret==-1){
	printf("pipe fail\n");
    }
    if((pid=fork())==-1){
	printf("fork fail\n");
    }
    else if(pid==0){//子进程负责读
	sleep(5);
	close(fd[1]);//关闭写端
	ret=read(fd[0],buf2,sizeof(buf2)-1);
	if(ret==-1){
	
	}
	close(fd[0]);//读完后，关闭读端
	buf2[ret]='\0';
	printf("read after buf2[] = %s\n",buf2);
    }
    else{	    //父进程负责写
	close(fd[0]);//关闭读端
	ret=write(fd[1],buf1,strlen(buf1));
	if(ret==-1){
	
	}
	close(fd[1]);//写完后，关闭写端
    }
    return 0;
}
