#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<strings.h>
#include<stdlib.h>
#include<errno.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/wait.h>
#if 0
    
#endif
//3.多进程_serve.c
void fun(int n){
    int wai=waitpid(-1,NULL,WNOHANG);
    printf("waitpid = %d\n",wai);
}

int main(int argc,char *argv[]){
    if(argc<2){
	perror("./a.out less");
	exit(-1);
    }
    int sfd,cfd;
    pid_t pid;
    char buf[128];
    char ip_buf[40];
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
    socklen_t c_len=sizeof(caddr);
    while(1){
    printf("serve is waiting ...\n");
    cfd=accept(sfd,(struct sockaddr *)&caddr,&c_len);
    printf("client ip[%s],port[%d]\n",
	    inet_ntop(AF_INET,&caddr.sin_addr.s_addr,ip_buf,sizeof(ip_buf)),
	    ntohs(caddr.sin_port));
    if(cfd==-1){
	perror("accept error\n");
	exit(-1);
    }
    printf("----------accept ok----------\n");
    pid=fork();
    if(pid==-1){
	printf("fork fail\n");
	exit(-1);
    }
    else if(pid==0){
	close(sfd);
	while(1){
	    ret=read(cfd,buf,sizeof(buf));
	    if(ret==0){
		printf("客户端断开\n");
		exit(-1);
	    }
	    else if(ret<0){
		if(errno==EINTR)
		    continue;
		else{
		    perror("read error\n");
		    exit(-1);
		}
	    }
	ret=write(STDOUT_FILENO,buf,ret);
	write(cfd,"I am client\n",sizeof("I am client\n"));
	}
    
    }
    else{
	close(cfd);
	signal(SIGCHLD,fun);
	}
    }
    return 0;
}
