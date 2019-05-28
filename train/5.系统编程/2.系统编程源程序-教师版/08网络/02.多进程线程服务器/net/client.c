#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char* argv[])
{
	if(argc < 2){
		printf("./client less...\n");
		exit(-1);
	}
	
	int ret;
	int cfd;
	int n;
	char buf[1024];
	struct sockaddr_in saddr;
	
	cfd = socket(AF_INET,SOCK_STREAM,0);
	if(cfd == -1){
		perror("socket error");
		exit(-1);
	}
	memset(&saddr,0,sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(atoi(argv[1]));
	inet_pton(AF_INET,"127.0.0.1",&saddr.sin_addr.s_addr);

	ret = connect(cfd,(struct sockaddr*)&saddr,sizeof(saddr));
	if(ret == -1){
		perror("connect error");
		exit(-1);
	}
	while(fgets(buf,sizeof(buf),stdin)!= NULL){
		n = write(cfd,buf,strlen(buf));	
		if(n == -1){
		
		}
		n = read(cfd,buf,sizeof(buf));

		write(STDOUT_FILENO,buf,n);
	}
	close(cfd);
	
	return 0;
}
