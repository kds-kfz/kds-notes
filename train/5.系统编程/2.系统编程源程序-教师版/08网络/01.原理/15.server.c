#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERV_PORT 8888

int main()
{
	int ret;
	char ip_buf[40];
	char buf[256];
	int lfd/*用于监听*/,cfd/*与客户端进行数据交互*/;
	struct sockaddr_in saddr, caddr;
	socklen_t c_len;

	//创建socket套接字
	lfd = socket(AF_INET,SOCK_STREAM,0);
	
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(SERV_PORT);
	//saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	inet_pton(AF_INET,"127.0.0.1",&saddr.sin_addr.s_addr);

	//绑定本地地址和端口
	bind(lfd,(struct sockaddr*)&saddr,sizeof(saddr));

	//监听
	listen(lfd,128);

	c_len = sizeof(caddr);
	printf("before accept ...\n");
	cfd = accept(lfd,(struct sockaddr*)&caddr,&c_len);
	printf("------------accept ok-----------------\n");
	printf("client:ip[%s]:port[%d]\n",inet_ntop(AF_INET,&caddr.sin_addr.s_addr,ip_buf,sizeof(ip_buf)),ntohs(caddr.sin_port));


	while(1){
		ret = read(cfd,buf,sizeof(buf));
		if(ret == 0)
			printf("服务端断开连接\n");
		ret = write(STDOUT_FILENO,buf,ret);
		write(cfd,"I'am client\n",12);		
	}
	
	return 0;
}
