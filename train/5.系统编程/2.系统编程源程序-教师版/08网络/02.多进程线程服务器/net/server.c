#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <errno.h>


int main(int argc, char* argv[])
{
	if(argc < 2){
		printf("./a.out less...\n");
		exit(1);
	}

	int sfd, cfd;
	int ret;
	int n;
	char buf[128];
	char ip_buf[40];
	socklen_t c_len;
	struct sockaddr_in saddr, caddr;
	//创建套接字
	sfd = socket(AF_INET,SOCK_STREAM,0);
	if(sfd == -1){
		perror("socket error...\n");
		exit(-1);
	}
	
	bzero(&saddr,sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(atoi(argv[1]));
	inet_pton(AF_INET,"127.0.0.1",&saddr.sin_addr.s_addr);
	
	//绑定本地地址和端口
	ret = bind(sfd,(struct sockaddr*)&saddr,sizeof(saddr));   
	if(ret == -1){
		perror("bind error");
		exit(-1);
	}	
	//监听	
  	ret = listen(sfd,128);  
	if(ret == -1){
		perror("listen error");
		exit(-1);
	}

	c_len = sizeof(caddr);	
	printf("before accept...\n");
	cfd = accept(sfd,(struct sockaddr*)&caddr,&c_len);      
	if(cfd == -1){
		perror("accept error");
		exit(-1);
	}
	printf("after accept...\n");	
	printf("client:IP[%s]:PORT[%d]\n",inet_ntop(AF_INET,&caddr.sin_addr.s_addr,ip_buf,sizeof(ip_buf)),ntohs(caddr.sin_port));
	
	while(1){
		n = read(cfd,buf,sizeof(buf));	
		if(n == 0){
			printf("客户端断开连接...\n");
		}else if(n < 0){
			if(errno == EINTR)
				continue;				
			else{
				perror("read eror...\n");
				return -1;
			}
		}

		n = write(STDOUT_FILENO,buf,n);

		n = write(cfd,"I'am client\n",12);
	}

	
	close(cfd);
	close(sfd);

	return 0;
}
