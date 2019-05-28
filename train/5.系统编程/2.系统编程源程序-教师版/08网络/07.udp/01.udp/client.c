#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <strings.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main()
{
	struct sockaddr_in saddr;
	int sfd, n;
	char buf[BUFSIZ];

	sfd = socket(AF_INET,SOCK_DGRAM,0);
	if(sfd == -1){
		perror("socket error");
		exit(1);
	}
	
	bzero(&saddr,sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(8888);
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);

	while(fgets(buf,sizeof(buf),stdin) != NULL){
		n = sendto(sfd,buf,strlen(buf),0,(struct sockaddr*)&saddr,sizeof(saddr));
		if(n == -1)	
			perror("sendto error");
		n = recvfrom(sfd,buf,BUFSIZ,0,NULL,0);	
		if(n == -1)
			perror("recv from");
		write(STDOUT_FILENO,buf,n);	
	}
	
	return 0;
}











