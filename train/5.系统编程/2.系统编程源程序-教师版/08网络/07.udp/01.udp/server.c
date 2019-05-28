/*
	传输层主要应用的协议类型有两种,一种是TCP协议,一种是UDP协议,TCP协议在网络通信中占主导地位,
	绝大多数的网络通信协议借助TCP协议来完成数据传输,但UDP也是网络通信中不可或缺的重要通信手段
	相较于TCP而言,UDP通信的形式更像是发短信,不需要在数据传输之前建立,维护连接.只需要专心获取数据
	就好.省去了三次握手的过程.通信速度可以大大提高,但与之伴随的稳定性和正确率便得不到保证,因此
	我们称UDP为"无连接的不可靠数据报协议"

	试用场景:视频会议


	比较:
	TCP:面向连接的可靠数据字节流传递
	　　优点:稳定　　　数据稳定,传输速率稳定,流量稳定、
		缺点: 效率低,传输速度慢
	使用场景:大文件,重要文件
		
	UDP:无连接的不可靠报文传递
		优点:效率高,速度快
		缺点:不稳定    数据,速率,流量
	使用场景:视频会议,视频通话,对实时性要求较高,广播	
		
*/
/*
	UDP可能出现缓冲区被填满后,再接收数据时丢包的现象:
	可以用下面的方法解决:
	1)服务器应用层设计流量控制,控制发送数据速度
	2)借助setsockopt函数改变缓冲区大小
	#include <sys/socket.h>
	int setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen);
	int n = 220*1024
	setsockopt(sockfd,SOL_SOCKET,SO_RCVBUF,&n,sizeof(n));
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <strings.h>

int main()
{
	int sfd;
	int n,i;
	char ip_buf[40];
	char buf[1024];
	socklen_t c_len;
	struct sockaddr_in saddr,caddr;

	sfd = socket(AF_INET,SOCK_DGRAM,0);		
	if(sfd == -1){
		perror("socket error");
		exit(1);
	}
		
	bzero(&saddr,sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(8888);
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	int ret = bind(sfd,(struct sockaddr*)&saddr,sizeof(saddr));
	if(ret == -1){
		perror("bind error");
		exit(1);
	}
	printf("Accepting connections...\n");
	while(1){
			c_len = sizeof(caddr);
			n = recvfrom(sfd,buf,sizeof(buf),0,(struct sockaddr*)&caddr,&c_len);
			if(n == -1)
				perror("recvfrom");
	htonl
			printf("client:IP[%s]:[%d]\n",inet_ntop(AF_INET,&caddr.sin_addr.s_addr,ip_buf,sizeof(ip_buf)),ntohs(caddr.sin_port));
			for(i=0; i<n; i++)
				buf[i] = toupper(buf[i]);
			n = sendto(sfd,buf,n,0,(struct sockaddr*)&caddr,sizeof(caddr));	
			if(n == -1){
				perror("sendto error");
				exit(1);
			}
	}
	close(sfd);
	return 0;
}










