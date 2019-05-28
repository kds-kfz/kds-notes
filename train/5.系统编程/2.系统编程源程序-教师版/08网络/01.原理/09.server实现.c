#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

#define MAXLINE 80
#define SERPORT 6666

int main()
{
	struct sockaddr_in servAddr, clientAddr;
	int cliAddrLen = sizeof(cliAddrLen);
	int listenfd, connfd;
	char buf[MAXLINE];
	char str[INET_ADDRSTRLEN];
	/*
	  socket()打开一个网络通讯端口,如果成功的话,就像open()一样
	  返回一个文件描述符,应用程序可以像读写文件一样用read/write在
	  网络上收发数据,如果socket()调用出错则返回-1.对于IPv4,domain参数
	  指定为AF_INET.对于TCP协议,type参数指定为SOCK_STREAM,表示面向流的
	  传输协议.如果是UDP协议,则type参数指定为SOCK_DGRAM,表示面向数据报的传输协议
	  protocol,指定为0即可.
	*/
	listenfd = socket(AF_INET,SOCK_STREAM,0);
	memset(&servAddr,0,sizeof(servAddr));
	//bzero(&serAddr,sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(SERPORT);
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	/*
	  服务器程序所监听的网络地址和端口号通常是固定不变的，客户端程序得知服务器程序
	  的地址和端口号后就可以像服务器发起连接，因此服务器需要调用bind()绑定一个固定
	  的网络地址和端口号
	  bind()的作用是将参数sockfd和addr绑定在一起，使sockfd这个用于网络通讯的文件描述符
	  监听addr所描述的地址和端口号。
	  前面讲过,struct sockaddr* 是一个通用指针类型，addr参数实际上可以接受多种协议的sockaddr结构体
	  而它们的长度各不相同，所以需要第三个参数addrlen指定结构体的长度
	  例如:
	  struct sockaddr_in servaddr;
	  bzero(&servaddr,sizeof(servaddr));
	  servaddr.sin_family = AF_INET;
	  servaddr.sin_port = htons(6666);
	  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	  首先将整个结构体清零,然后设置地址类型为AF_INET,网络地址为INADDR_ANY,这个宏表示
	  本地的任意IP地址，因为服务器可能有多个网卡,每个网卡也可能绑定多个IP地址，这样的
	  设置可以在所有的IP地址上监听,直到与某个客户端建立了连接时才确定下来到底用哪个IP地址，端口号为6666
	*/
	bind(listenfd,(struct sockaddr*)&servAddr,sizeof(servAddr));
	/*
	  典型的服务器程序可以同时服务于多个客户端,当有客户端发起连接时,服务器调用的accept()
	  返回并接受这个连接，如果有大量的客户端发起连接而服务器来不及处理，尚未accept()的客户端
	  就处于连接等待状态,listen()声明sockfd处于监听状态,并且最多允许有backlog个客户端处于连接等待状态
	  如果接收到更多的连接请求就忽略,listen()成功返回0,失败返回-1
	*/
	listen(listenfd,20);
	
	printf("Accepting connections...\n");

	while(1){
		cliAddrLen = sizeof(clientAddr);
		/*
			sockfd:socket文件描述符
			addr:传出参数,返回链接客户端地址信息,含ip地址和端口号
			addrlen:传入参数(值-结果),传入sizeof(addr)大小,函数返回时返回真正接收到地址结构体的大小
			返回值:成功返回一个新的socket文件描述符,用于和客户端的通信,失败返回-1,设置errno

			三次握手完成后,服务器调用accept()接受连接,如果服务器调用accept()时还没有客户端的连接请求,
			就阻塞等待直到有客户端连接上来。addr是一个传出参数,accept()返回时传出客户端的地址和端口号,
			addrlen参数是一个传入传出参数,传入的是调用者提供的缓冲区addr的长度以避免缓冲区溢出问题，
			传出的是客户端地址结构体的实际长度(有可能没有占满调用者提供的缓冲区).如果给addr参数传NULL,
			表示不关心客户端的地址

		*/
		connfd = accept(listenfd,(struct sockaddr*)&clientAddr,&cliAddrLen);
		/*
		  整个是一个while死循环,每次循环处理一个客户端连接。由于cleAddrLen是传入传出参数,
		  每次调用accept()之前应该重新赋初值。accept()的参数listenfd是先前的监听文件描述符,
		  而accept()的返回值是另外一个文件描述符connfd,之后与客户端之间就通过这个connfd通讯,
		  最后关闭connfd断开连接,而不关闭listenfd，再次回到循环开头listenfd仍然用作accept()
		  accept()成功返回一个文件描述符，失败返回-1
		*/
		int n = read(connfd,buf,sizeof(buf));
		printf("received from %s at PORT %d\n",
			inet_ntop(AF_INET,&clientAddr.sin_addr,str,sizeof(str)),
			ntohs(clientAddr.sin_port));
		for(int i = 0; i < n; i++)
			buf[i] = toupper(buf[i]);
		write(connfd,buf,n);
		close(connfd);
	}
	return 0;
}
