#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <assert.h>

#define PORT 8000
#define IP "127.0.0.1"

int main()
{	
	int clientfd;
	int ret;
	char buf[BUFSIZ];
	memset(buf,0,sizeof(buf));

	clientfd = socket(AF_INET,SOCK_STREAM,0);
	assert(clientfd);
	//在这里linux操作系统帮我们隐式bind
	struct sockaddr_in saddr, caddr;
	
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(PORT);
	inet_pton(AF_INET,IP,&saddr.sin_addr.s_addr);//saddr.sin_addr.s_addr = htonl(INADDR_ANY);
				
	connect(clientfd,(struct sockaddr*)&saddr,sizeof(saddr));
	
	while(1){
		fgets(buf,sizeof(buf),stdin);
		ret = write(clientfd,buf,strlen(buf));
		if(ret < 0){
			printf("write error\n");
			exit(-1);
		}
		ret = read(clientfd,buf,sizeof(buf)-1);
		if(ret < 0){
			printf("read error\n");
			exit(-1);
		}
		printf("from server:%s\n",buf);
	}

	return 0;
}
