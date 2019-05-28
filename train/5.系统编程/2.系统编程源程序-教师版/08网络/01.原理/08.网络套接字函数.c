socket模型创建流程图:
									
									TCP服务器端
                                      socket() 
                                         | 
									   bind()
                                         |  
 TCP客户端                             listen()
                                         | 
 socket()                              accept()
    |                                    |      
    |                                    |阻塞直到有客户端建立连接
 connet()         建立连接               |                                        
    |<---------------------------------- |
    |            请求数据                |
 write()<------------------------------>read()  
    |                                    |
    |                                    |
    |                                    |
 read()<------------------------------->write()
    |                                    |
	|                                    |
    |              结束连                |
  close()------------------------------>read()	
                                         |
                                        close()
--------------------------------------------------------------------------------------------------------------------
socket 函数:
#include <sys/types.h>
#include <sys/socket.h>

int socket(int domain, int type, int protocol)

domain:
	AF_INET:这是大多数用来产生socket的协议,使用tcp或udp来传输,用ipv4的地址
	AF_INET6:与上面类似,不过是用IPV6的地址
	AF_UNIX:本地协议,使用在UNIX和linux系统上,一般都是当客户端和服务器端在同一台机器上的时候使用
type:
	SOCK_STREAM:这个协议是按照顺序的,可靠的,数据完整的基于字节流的连接,这是一个使用最多的socket类型,这个socket是使用TCP来进行传输的
	SOCK_DGRAM:这个协议是无连接的,固定长度的传输调用,该协议是不可靠的,数据报文协议,使用udp来进行它的连接
	SOCK_SEQPACKET:该协议是双线路的,可靠的连接,发送固定长度的数据包进行传输,必须把这个包完整的接受才能进行读取
	SOCK_RAM:socket类型提供单一的网络访问,这个socket类型使用ICMP公共协议(ping,traceroute使用该协议)
	SOCK_RDM:这个类型很少使用,在大部分的操作系统上没有实现,它是提供给数据链路层使用,不保证数据包的顺序

protocol:
	传0表示使用默认协议

返回值：成功返回新创建的socket的文件描述符,失败,返回-1,设置errno
----------------------------------------------------------------------------------
struct sockaddr_in {
  sa_family_t    sin_family; /* address family: AF_INET */
  in_port_t      sin_port;   /* port in network byte order */
  struct in_addr sin_addr;   /* internet address */
};

/* Internet address. */
struct in_addr {
  uint32_t       s_addr;     /* address in network byte order */
};

-----------------------------------------------------------
#include <sys/types.h>
#include <sys/socket.h>
int bind(int sockfd, const struct sockaddr *addr/*绑定的是自己的地址和端口*/,socklen_t addrlen/*传入的大小*/)
sockfd:
	socket文件描述符
addr:
	结构体的端口号和IP地址赋值后,传到这里
addrlen:
	sizeof(addr),传长度
返回值:
	成功返回0,失败返回-1,设置errno
说明:
	服务器所监听的网络地址和端口号通长是固定不变的,客户端得知服务器程序的地址和端口号后就可以向服务器发起连接,
	因此服务器需要调用bind绑定一个固定的网络地址和端口号(这就像我们的电话机有一个固定的电话号码,其他人就可以
	通过这个号码,与我进行通话,当我设置分机后,我这个号码就可以实现一对多的通话了)

    bind的作用是将sockfd和addr绑定在一起,使sockfd这个用于网络通讯的文件描述符监听addr所描述的地址和端口号(也就是指定
    了我当前服务的,某个进程)
	
	因为,struct sockaddr* 是一个通用指针类型,addr参数可以接受多种类型的sockaddr结构体,它们的长度各不相同,因而,在这里
	我们需要在第三个参数中指定长度
--------------------------------------------------------------------------------------------------------------------
#include <sys/types.h>
#include <sys/socket.h>

并不是指定连接的上限数
int listen(int sockfd, int backlog) 仅仅是用来指定同时允许多少个客户端与我建立连接(处于３次握手的数量是多少)
sockfd:
	socket文件描述符
backlog:
	排队建立3次握手队列和刚刚建立3次握手队列的链接数和

典型的服务器程序可以同时服务于多个客户端,当有客户端发起连接时,服务器调用的accept()返回并接受这个连接,
如果有大量的客户端发起连接而服务器来不及处理,尚未accept的客户端就处于连接等待状态,listen()声明sockfd
处于监听状态,并且最多允许有backlog个客户端处于连接等待状态,如果接收到更多的连接请求就忽略
返回值:
成功返回0,失败返回-1
--------------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>

int cfd = int accept(int sockfd, struct sockaddr* addr, socklen_t *addrlen);
sockfd:
	socket文件描述符
addr:
	传出参数,返回链接客户端地址信息,这个信息中包含有ip地址和端口号
addrlen:
	传入参数(值－结果),传如sizeof(addr)大小,函数返回时返回真正收到的地址结构体大小
返回值:
	成功返回一个新的sockfd(这个描述符是用于和客户端通信的描述符号,注意和listen监听的描述符相区别)
---------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>

int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen)
sockfd:
	socket文件描述符
addr:
	传入参数,指定服务器端地址信息(即服务端的ip和端口)
addrlen:
	传入参数,传入的sizeof(addr)大小
返回值:
　　成功返回0,失败返回-1,设置errno变量
注意:
	connect函数和bind的参数形式一致,但是,bind是绑定的自身的ip和端口,而connect是要连接方的ip和端口

