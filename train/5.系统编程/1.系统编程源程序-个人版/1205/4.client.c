#include<stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
 #include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>

#if 0
    int socket(int domain, int type, int protocol);
    int connect(int sockfd, const struct sockaddr *addr,socklen_t addrlen);
    int inet_pton(int af, const char *src, void *dst);
#endif
#define CLIENT_PORT 7777
int main(){
    int cfd;
    int ret;
    char buf[50];
    struct sockaddr_in client_init;
    client_init.sin_family=AF_INET;
    client_init.sin_port=htons(CLIENT_PORT);
    //client_init.sin_addr.s_addr=htonl(INADDR_ANY);
    inet_pton(AF_INET,"127.0.0.1",&client_init.sin_addr.s_addr);

    cfd=socket(AF_INET,SOCK_STREAM,0);
    if(cfd==-1){
	printf("socket error\n");
	exit(-1);
    }
    connect(cfd,(struct sockaddr *)&client_init,sizeof(client_init));
    printf("客户端连接服务端成功\n");
    while(1){
	ret=read(STDIN_FILENO,buf,sizeof(buf));
	if(ret==-1){
	    printf("read error\n");
	    exit(-1);
	}
	ret=write(cfd,buf,ret);
	if(ret==-1){
	    printf("read error\n");
	    exit(-1);
	}
	ret=read(cfd,buf,sizeof(buf));
	if(ret==-1){
	    printf("read error\n");
	    exit(-1);
	}
	weite(STDOUT,buf,ret);
//	printf("%s",buf);
    }

    return 0;
}
