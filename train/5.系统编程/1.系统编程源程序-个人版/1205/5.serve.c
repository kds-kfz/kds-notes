#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<sys/wait.h>
#if 0
    int socket(int domain, int type, int protocol);
    int bind(int sockfd, const struct sockaddr *addr,socklen_t addrlen);
    int inet_pton(int af, const char *src, void *dst);
    int listen(int sockfd, int backlog);
    int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

#endif

#define SERVER_PORT 7777

void fun(int a){
    int wai=waitpid(-1,NULL,WNOHANG);
    printf("waitpid %d\n",wai);
}

int main(){
    int sfd;
    int cfd;
    int ret;
    pid_t pid;
    char buf[512];
    char buf_ip[INET_ADDRSTRLEN];
    struct sockaddr_in server_init;
    struct sockaddr_in client_rcv;

    server_init.sin_family=AF_INET;
    server_init.sin_port=htons(SERVER_PORT);
    //server_init.sin_addr.s_addr=htonl(INADDR_ANY);
    inet_pton(AF_INET,"127.0.0.1",&server_init.sin_addr.s_addr);

    //创建
    sfd=socket(AF_INET,SOCK_STREAM,0);
    if(sfd==-1){
	printf("socket error\n");
	exit(-1);
    }
    //绑定
    ret=bind(sfd,(struct sockaddr *)&server_init,sizeof(server_init));
    if(ret==-1){
	printf("bind error\n");
	exit(-1);
    }
    ret=listen(sfd,20);
    if(ret==-1){
	printf("listen error\n");
	exit(-1);
    }
    while(1){
    printf("服务端等待客户端连接 ...\n");
    int c_len=sizeof(client_rcv);
    cfd=accept(sfd,(struct sockaddr *)&client_rcv,&c_len);
    if(cfd==-1){
	printf("accept error\n");
	exit(-1);
    }
    printf("服务端接入客户端成功\n");
    printf("client ip[%s],port[%d]\n",
	inet_ntop(AF_INET,
	    &client_rcv.sin_addr.s_addr,buf_ip,INET_ADDRSTRLEN),
	ntohs(client_rcv.sin_port));
    pid=fork();
    if(pid==-1){
	printf("fork error\n");
	exit(-1);
    }
    else if(pid==0){
	close(sfd);//关闭服务端套接字描述符
	while(1){
	    ret=read(cfd,buf,sizeof(buf));
	    if(ret==0){
		printf("服务器断开\n");
		exit(-1);
	    }
	    write(STDOUT_FILENO,buf,ret);
	    ret=write(cfd,"I am client\n",12);
	}
//	break;
    }
    else{
	close(cfd);//关闭客户端套接字描述符
	signal(SIGCHLD,fun);
	}
    }
    return 0;
}
