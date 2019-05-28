#include<stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include<strings.h>
#include<stdlib.h>
#include<errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#if 0
    
#endif
//4.pthread_serve.c
struct info{
    int cfd;
    struct sockaddr_in caddr;
};
void *thread_handle(void *arg){
    struct info *ts=(struct info *)arg;
    char ip_buf[40];
    char buf[256];
    int ret;
    printf("client ip[%s],port[%d]\n",
	    inet_ntop(AF_INET,&(ts->caddr.sin_addr.s_addr),ip_buf,sizeof(ip_buf)),
		ntohs(ts->caddr.sin_port));
    while(1){
	ret=read(ts->cfd,buf,sizeof(buf));
	if(ret==0){
	    printf("客户端断开\n");
	    return (void *)-1;
	}
	else if(ret==-1){
	    perror("read error\n");
	    exit(-1);
	}
	write(STDOUT_FILENO,buf,ret);
	write(ts->cfd,"I am client\n",sizeof("I am client\n"));
    }
    close(ts->cfd);
}

int main(int argc,char *argv[]){
    if(argc<2){
	perror("./a.out less");
	exit(-1);
    }
    int sfd,cfd;
    pthread_t pth;
    int count=0;
    struct sockaddr_in saddr;
    struct sockaddr_in caddr;

    sfd=socket(AF_INET,SOCK_STREAM,0);
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
    ret=listen(sfd,128);
    if(ret==-1){
	perror("listen fail\n");
	exit(-1);
    }
    struct info ts[256];

    while(1){
	socklen_t c_len=sizeof(caddr);
	printf("before accept----------\n");
	cfd=accept(sfd,(struct sockaddr *)&caddr,&c_len);
	printf("accept ok----------\n");
	ts[count].cfd=cfd;
	ts[count].caddr=caddr;
	pthread_create(&pth,NULL,thread_handle,(void *)&ts[count]);
	pthread_detach(pth);
	count++;
	if(count>=256)
	    break;
    }
    close(sfd);
//    close(cfd);
    return 0;
}
