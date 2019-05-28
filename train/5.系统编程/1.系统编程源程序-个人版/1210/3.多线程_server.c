#include<stdio.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>


#define SERVER_PORT 8888
//多线程_SERVER.C
typedef struct{
    int cfd;
    struct sockaddr_in caddr;
}Client;

void *thread_handle(void *arg){
    Client *client=(Client *)arg;
    char ip_buf[INET_ADDRSTRLEN];
    char buf[128];
    int ret;

    printf("client ip[%s],port[%d]\n",
	inet_ntop(AF_INET,&(client->caddr.sin_addr.s_addr),ip_buf,INET_ADDRSTRLEN),
	client->caddr.sin_port);	

    while(1){
	ret=read(client->cfd,buf,sizeof(buf));
	if(ret==0){
	    perror("客户端断开\n");
//	    exit(-1);
//	    pthread_exit(NULL);
	    pthread_join(pthread_self(),NULL);
	}
	else if(ret<0){
	    if(errno==EINTR)
		continue;
	    else{
		perror("read error\n");
		exit(-1);
	    }
	}
	write(STDOUT_FILENO,buf,ret);
	write(client->cfd,"server recieve data ok\n",
	    sizeof("server recieve data ok\n"));
    }
}

int main(){
    int sfd,cfd;
    int ret;
    struct sockaddr_in saddr;
    struct sockaddr_in caddr;
    socklen_t c_len;

    int num=0;
    pthread_t pth;
    Client client[256];

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
    while(1){
	c_len=sizeof(caddr);
	cfd=accept(sfd,(struct sockaddr *)&caddr,&c_len);
	if(cfd==-1){
	    perror("accept error\n");
	    exit(-1);
	}
	client[num].cfd=cfd;
	client[num].caddr=caddr;
	ret=pthread_create(&pth,NULL,thread_handle,(void *)&client[num]);
//	pthread_detach(pth);
	if(ret!=0){
	    printf("pthread_create error %s\n",strerror(ret));
	    exit(-1);
	}
	num++;
    }
    return 0;
}
