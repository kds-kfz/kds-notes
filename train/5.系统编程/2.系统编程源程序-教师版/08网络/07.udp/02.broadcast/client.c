#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <strings.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define SERV_PORT 8888
#define MAXLINE 4096
#define CLIENT_PORT 9000

int main()
{
	struct sockaddr_in localaddr;
	int cfd, n;
	char buf[MAXLINE];

	cfd = socket(AF_INET,SOCK_DGRAM,0);
	if(cfd == -1){
		perror("socket error");
		exit(1);
	}
	
	bzero(&localaddr,sizeof(localaddr));
	localaddr.sin_family = AF_INET;
	localaddr.sin_port = htons(CLIENT_PORT);
	inet_pton(AF_INET,"0.0.0.0",&localaddr.sin_addr.s_addr);

	/*显示绑定,不能省略*/
	int ret = bind(cfd,(struct sockaddr*)&localaddr,sizeof(localaddr));

	while(1){
		n = recvfrom(cfd,buf,sizeof(buf),0,NULL,0);	
		if(n == -1)
			perror("recv from");
		write(STDOUT_FILENO,buf,n);	
	}
	
	return 0;
}











