#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

void fun(){
    printf("fifo signal error\n");
}
void (*p)()=&fun;
void fun2(int n){
    printf("fifo signal error\n");
}
int main(){
//    signal(SIGPIPE,p);
    signal(SIGPIPE,fun2);//管道断裂

    int ret=mkfifo("fifo.txt",0640);
    if(ret==-1){
	printf("fifo fail\n");
    }
    int fd=open("fifo.txt",O_WRONLY);
    if(fd==-1){
	printf("open fail\n");
    }
    char buf1[]="Hello";
    char buf2[]="Red";
    char buf3[]="Yellow";
    char buf4[]="World";
    char buf5[]="Hello1Red123YellowWorld1";
    printf("write before\n");
    printf("sleep 5s ...\n");
    sleep(5);
    int count=0;
    int t=0;
    while(1){
	count++;
//	ret=write(fd,buf1,strlen(buf1));
	ret=write(fd,&buf5[t],6);
	if(ret==-1){
	    printf("write fail : %dth\n",count);
	    exit(-1);
	}
	printf("write ok : %dth\n",count);
	if(count>10)break;
	sleep(5);
	t+=6;
    }
    close(fd);
    return 0;
}
