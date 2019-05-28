#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<strings.h>
#include<string.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<error.h>
//2.udp_clinent.c
int main(int argc,char *argv[]){
    if(argc<2){
	printf("./a.out less\n");
	exit(-1);
    }
    char buf[1024];
    int cfd,ret;
    struct sockaddr_in caddr;
    cfd=socket(AF_INET,SOCK_DGRAM,0);
    if(cfd==-1){
	perror("socket fail\n");
	exit(-1);
    }
    memset(&caddr,0,sizeof(caddr));
    caddr.sin_family=AF_INET;
    caddr.sin_port=htons(atoi(argv[1]));
    //caddr.sin_addr.s_addr=htonl(INADDR_ANY);
    inet_pton(AF_INET,"127.0.0.1",&caddr.sin_addr.s_addr);
    while(fgets(buf,sizeof(buf),stdin)!=NULL){
	ret=sendto(cfd,buf,strlen(buf),0,(struct sockaddr *)&caddr,sizeof(caddr));
	if(ret==-1){
	    printf("sendto error\n");
	}
	ret=recvfrom(cfd,buf,sizeof(buf),0,NULL,0);
	if(ret==-1)
	    perror("recv from\n");
	write(STDOUT_FILENO,buf,ret);
    }
    close(cfd);
    return  0;
}
