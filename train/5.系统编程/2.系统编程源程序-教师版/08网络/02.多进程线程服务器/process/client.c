#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <strings.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
	if(argc < 3){
		printf("./client filename less...\n");
		return 0;
	}

	int ret;
	int cfd;
	char buf[80];
	struct sockaddr_in saddr;
	//创建套接字
	cfd = socket(AF_INET,SOCK_STREAM,0);
	
	//服务器端的ip和端口
	//memset(&saddr,0,sizeof(saddr));
	bzero(&saddr,sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(atoi(argv[2])); 
	inet_pton(AF_INET,argv[1],&saddr.sin_addr.s_addr);
	//建立连接
	connect(cfd,(struct sockaddr*)&saddr,sizeof(saddr));
	printf("----------connect ok--------------\n");

	while(1){
		ret = read(STDIN_FILENO,buf,sizeof(buf));
		if(ret < 0){
			perror("read error");
			return -1;
		}
		ret = write(cfd,buf,ret);
		if(ret < 0){
		
		}
		ret = read(cfd,buf,sizeof(buf));
		write(STDOUT_FILENO,buf,ret);
	}

}
