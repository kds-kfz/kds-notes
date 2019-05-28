/*
	nc 127.0.0.1 8000用于测试服务端
*/
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define PORT 8888
#define IP "127.0.0.1"

int main()
{
	char buf[BUFSIZ];
	char client_ip[40];
	int listenfd,clientfd,ret;
	struct sockaddr_in saddr, caddr;
	
	//创建套接字
	listenfd = socket(AF_INET,SOCK_STREAM,0);
	if(listenfd < 0)
		fprintf(stderr,"socket error\n");
	
	//ip地址和端口号赋值
	socklen_t len = sizeof(caddr), ip_len;	
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(PORT);
	//addr.sin_addr.s_addr = htonl(INADDR_ANY);//绑定本地有效的任意ip
	inet_pton(AF_INET,"127.0.0.1",&saddr.sin_addr.s_addr);	
	
	//绑定本地地址和端口号
	ret = bind(listenfd,(struct sockaddr*)&saddr,sizeof(saddr));	
	if(ret < 0)
		fprintf(stderr,"bind error...\n");
	
	//监听
	ret = listen(listenfd,20);//默认值是128,最大上限
	if(ret < 0)
		fprintf(stderr,"lsiten error\n");
	printf("before accept...\n");
	
	//建立连接
	clientfd = accept(listenfd,(struct sockaddr*)&caddr,&len);
	printf("after accept...\n");	
	memset(buf,sizeof(buf),0);//bzero(buf,sizeof(buf))
	
	while(1){
		ret = read(clientfd,buf,sizeof(buf)-1);
		if(ret < 0){
			fprintf(stderr,"read error...\n");
			exit(-1);
		}else if(ret == 0){
			printf("与客户端断开连接\n");
			exit(-1);
		}
		buf[ret] = '\0';
		printf("recv a msg[client ip:%s:client port:%d]:%s",inet_ntop(AF_INET,&caddr.sin_addr.s_addr,client_ip,sizeof(client_ip)),ntohs(caddr.sin_port),buf);

		memset(buf,sizeof(buf),0);
		sprintf(buf,"i am server");
		ret = write(clientfd,buf,strlen(buf));
		if(ret < 0){
			printf("write error\n");
			exit(-1);
		}
	}

	close(clientfd);
	close(listenfd);

	return 0;
}












