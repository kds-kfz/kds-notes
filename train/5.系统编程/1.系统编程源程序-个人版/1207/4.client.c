#include<stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include<strings.h>
#include<string.h>
#include<stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
 #include <error.h>
int main(int argc,char *argv[]){
    if(argc<2){
	printf("./a.out less\n");
	exit(-1);
    }
    char buf[1024];
    int cfd,ret;
    struct sockaddr_in caddr;
    cfd=socket(AF_INET,SOCK_STREAM,0);
    if(cfd==-1){
	perror("socket fail\n");
	exit(-1);
    }
    memset(&caddr,0,sizeof(caddr));
    caddr.sin_family=AF_INET;
    caddr.sin_port=htons(atoi(argv[1]));
    inet_pton(AF_INET,"127.0.0.1",&caddr.sin_addr.s_addr);
    ret=connect(cfd,(struct sockaddr *)&caddr,sizeof(caddr));
    if(ret==-1){
	printf("connect error\n");
	exit(-1);
    }
    else
	printf("----------connect ok---------\n");
    while(fgets(buf,sizeof(buf),stdin)!=NULL){
	ret=write(cfd,buf,strlen(buf));
	if(ret==-1){
	
	}
	ret=read(cfd,buf,sizeof(buf));
	write(STDOUT_FILENO,buf,ret);
    }
    close(cfd);
    return  0;
}
