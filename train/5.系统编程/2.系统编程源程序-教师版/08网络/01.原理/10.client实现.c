#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAXLINE 80
#define SERPORT 6666

int main(int argc, char* argv[])
{
	struct sockaddr_in servAddr;
	char buf[MAXLINE];

	int sockfd;
	char *str;

	if(argc != 2){
		fputs("usage: ./client message\n",stderr);
	}
	str = argv[1];
	
	sockfd = socket(AF_INET,SOCK_STREAM,0);
	bzero(&servAddr,sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	inet_pton(AF_INET,"127.0.0.1",&servAddr.sin_addr);
	servAddr.sin_port = htons(SERPORT);

	/*
		socket:文件描述符
		addr:传入参数,指定服务器端地址信息，含IP地址和端口号
		addrlen:传入参数,传入sizeof(addr)大小
		返回值:成功返回0，失败返回-1

		客户端需要调用connect()连接服务器,connect()和bind()的参数形式一致
		区别在于bind的参数是自己的地址,而connect的参数是对方的地址
	*/
	connect(sockfd,(struct sockaddr*)&servAddr,sizeof(servAddr));

	write(sockfd,str,sizeof(str));
	int n = read(sockfd,buf,sizeof(buf));
	printf("Response from server:\n");
	write(STDOUT_FILENO,buf,n);
	printf("\n");
	close(sockfd);

	return 0;
}








