#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main(){
    int fd=open("fifo.txt",O_RDONLY);
    if(fd==-1){
	printf("open1 fail\n");
    }
    char buf[20]={};
    printf("sleep 5s ...\n");
    sleep(5);
    int ret=read(fd,buf,sizeof(buf)-1);
    if(ret==-1){
	printf("read2 fail\n");
    }
    printf("read2 ok\n");
    buf[ret]='\0';
    close(fd);
    printf("buf2 = %s\n",buf);
    return 0;
}
