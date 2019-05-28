#include<stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include <arpa/inet.h>


//client.c
#define CLIENT_PORT 8888

int main(){
    int cfd;
    int ret;
    struct sockaddr_in caddr;
    char buf[512];
    char read_buf[512];
    cfd=socket(AF_INET,SOCK_STREAM,0);
    if(cfd==-1){
	perror("client socket error\n");
	exit(-1);
    }
    caddr.sin_family=AF_INET;
    caddr.sin_port=htons(CLIENT_PORT);
    //caddr.sin_addr.s_addr=htonl(INADDR_ANY);
    inet_pton(AF_INET,"127.0.0.1",&caddr.sin_addr.s_addr);
    ret=connect(cfd,(struct sockaddr *)&caddr,sizeof(caddr));
    if(ret==-1){
	perror("connect error\n");
	exit(-1);
    }
    while(fgets(read_buf,sizeof(read_buf),stdin)!=NULL){
	ret=write(cfd,read_buf,strlen(read_buf));
	ret=read(cfd,buf,sizeof(buf));
	if(ret==-1){
	    perror("read error\n");
	    exit(-1);
	}
	write(STDOUT_FILENO,buf,ret);
    }
    
    return 0;
}
