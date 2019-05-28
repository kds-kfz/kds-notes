/*
	IP:192.168.13.255(广播)------>最大值 255.255.255.255
　　IP:192.168.13.1(网关)
	设置网络的时候不能用这两个地址　		

	默认情况下sockfd是不允许进行广播的
	setsockopt(int sockfd, int level, int optname, const void*optval, socklen_t optlen)
	给sockfd开发广播权限
*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>
#include <string.h>

#define SERV_PORT 8888
#define MAXLINE 1500

#define CLIE_PORT 9000
#define BROADCAST_IP "162.168.117.255"

int main()
{
	int sfd;
	struct sockaddr_in saddr, caddr;
	char buf[MAXLINE];
	
	sfd = socket(AF_INET,SOCK_DGRAM,0);
	if(sfd == -1)
		perror("socket error");

	bzero(&saddr,sizeof(saddr));	
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(SERV_PORT);
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(sfd,(struct sockaddr*)&saddr,sizeof(saddr));

	int flag = 1;
	setsockopt(sfd,SOL_SOCKET,SO_BROADCAST,&flag,sizeof(flag));
	
	/*构造client 地址　IP+端口*/
	bzero(&caddr,sizeof(caddr));
	caddr.sin_family = AF_INET;
	caddr.sin_port = htons(CLIE_PORT);
	inet_pton(AF_INET,BROADCAST_IP,&caddr.sin_addr.s_addr);
	
	int i = 0;
	while(1){
		sprintf(buf,"I'am broadcast[%d]\n",i++);
		sendto(sfd,buf,strlen(buf),0,(struct sockaddr*)&caddr,sizeof(caddr));
		sleep(1);
	}	
	close(sfd);
	return 0;
}




