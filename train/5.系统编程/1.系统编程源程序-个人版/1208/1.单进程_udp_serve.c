#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<strings.h>
#include<stdlib.h>
#include<errno.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<ctype.h>
//1.单进程_udp_serve.c
#if 0
    ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
	    struct sockaddr *src_addr, socklen_t *addrlen);
    sockfd：
    buf：
    len：
    flags：
    src_addr：
    addrlen：
    返回值：
    ***************************************************************
    ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
	    const struct sockaddr *dest_addr, socklen_t addrlen);
    sockfd：
    buf：
    len：
    flags：
    dest_addr：
    addrlen：
    返回值：成功返回发送字节数，错误返回-1
#endif

int main(int argc,char *argv[]){
    if(argc<2){
	perror("./a.out less");
	exit(-1);
    }
    int sfd;
    char buf[128];
    char ip_buf[40];
    struct sockaddr_in saddr;
    struct sockaddr_in caddr;

    sfd=socket(AF_INET,SOCK_DGRAM,0);
    if(sfd==-1){
	perror("socket error\n");
	exit(-1);
    }
    bzero(&saddr,sizeof(saddr));
    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(atoi(argv[1]));
    inet_pton(AF_INET,"127.0.0.1",&saddr.sin_addr.s_addr);
    int ret=bind(sfd,(struct sockaddr *)&saddr,sizeof(saddr));
    if(ret==-1){
	printf("bind error\n");
	exit(-1);
    }

    while(1){
    socklen_t c_len=sizeof(caddr);
    ret=recvfrom(sfd,buf,sizeof(buf),0,(struct sockaddr *)&caddr,&c_len);
    if(ret==-1)
	perror("recvfrom\n");
    printf("client ip[%s],port[%d]\n",
	    inet_ntop(AF_INET,&caddr.sin_addr.s_addr,ip_buf,sizeof(ip_buf)),
	    ntohs(caddr.sin_port));
    for(int i=0;i<ret;i++)
	buf[i]=toupper(buf[i]);
    ret=sendto(sfd,buf,ret,0,(struct sockaddr *)&caddr,sizeof(caddr));
    if(ret==-1){
	perror("sendto error\n");
	exit(-1);
	}
    }
    close(sfd);
    return 0;
}
