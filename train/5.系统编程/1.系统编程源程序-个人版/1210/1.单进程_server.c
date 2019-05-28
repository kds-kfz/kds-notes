#include<stdio.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<strings.h>
#include <arpa/inet.h>

#define SERVER_PORT 8888
//单线程_SERVER.C

int main(){
    int sfd,cfd;
    int ret;
    struct sockaddr_in saddr;
    struct sockaddr_in caddr;
    socklen_t c_len;
    char ip_buf[INET_ADDRSTRLEN];
    char buf[128];

    sfd=socket(AF_INET,SOCK_STREAM,0);
    if(sfd==-1){
	perror("socket error\n");
	exit(-1);
    }
    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(SERVER_PORT);
    //saddr.sin_addr.s_addr=htonl(INADDR_ANY);
    inet_pton(AF_INET,"127.0.0.1",&saddr.sin_addr.s_addr);

    ret=bind(sfd,(struct sockaddr *)&saddr,sizeof(saddr));
    if(ret==-1){
	perror("bind fail\n");
	exit(-1);
    }
    ret=listen(sfd,128);
    if(ret==-1){
	perror("listen fail\n");
	exit(-1);
    }
    c_len=sizeof(caddr);
    cfd=accept(sfd,(struct sockaddr *)&caddr,&c_len);
    printf("client ip[%s],port[%d]\n",
	inet_ntop(AF_INET,&caddr.sin_addr.s_addr,ip_buf,INET_ADDRSTRLEN),
	caddr.sin_port);
    while(1){
	ret=read(cfd,buf,sizeof(buf));
	if(ret==0)
	    perror("client 断开\n");
//	    exit(-1);
	write(STDOUT_FILENO,buf,ret);
	write(cfd,"server recieve data ok\n",
		sizeof("server recieve data ok\n"));
    }

    return 0;
}
