#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<strings.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
//2.clinet.c

int main(int argc,char *argv[]){
    if(argc<3){
	printf("./client less\n");
	exit(-1);
    }

    int cfd;
    int ret;
    char buf[80];
    struct sockaddr_in saddr;

    cfd=socket(AF_INET,SOCK_STREAM,0);

    bzero(&saddr,sizeof(saddr));
    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(atoi(argv[2]));
    inet_pton(AF_INET,argv[1],&saddr.sin_addr.s_addr);
    //建立链接
    connect(cfd,(struct sockaddr *)&saddr,sizeof(saddr));
    printf("客户端连接服务端成功\n");
    while(1){
	//从终端读入
	ret=read(STDIN_FILENO,buf,sizeof(buf));
	if(ret<0){
	    printf("read error\n");
	    exit(-1);
	}
	//将从终端读到的数据，写到套接字文件
	//此时服务器从套接字文件读取客户端写入的数据
	ret=write(cfd,buf,ret);
	if(ret<0){
	    printf("write error\n");
	    exit(-1);
	}
	//服务器接收到数据后，向套接字文件写入"I am client\n"
	//随后客户端读取套接字文件里的内容
	ret=read(cfd,buf,sizeof(buf));
	//打印读到的内容
	printf("%s",buf);
//	==write(STDOUT_FILENO,buf,ret);
    }
    return 0;
}

