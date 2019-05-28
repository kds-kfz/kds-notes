#include<stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//1.serve.c
#if 0
    int socket(int domain, int type, int protocol);
    domain：确定通信特性，可取以下宏定义
	AF_INET：IPv4因特网域
	AF_INET6：IPv6因特网域
	AF_UNIX：UNIX域
	AF_UPSPEC：未指定
    -------------------------------------------------------
    type：确定套接字的类型，可取以下宏定义
	SOCK_DGRAM：UDP
	SOCK_RAW：
	SOCK_SEQPACKET：
	SOCK_STREAM：TCP
    -------------------------------------------------------
    protocol：通常是0,表示给定的域和套接字类型选择默认协议
    -------------------------------------------------------
    返回值：成功返回套接字文件描述符，错误返回-1
    *******************************************************
    uint32_t htonl(uint32_t hostlong);
    将主机ip地址(32位)字节序转换成网络ip地址字节序
    返回值：以网络字节序表示的32位整数
    -------------------------------------------------------
    uint16_t htons(uint16_t hostshort);
    将主机端口号(16位)字节序转换成网络端口字节需
    返回值：以网络字节序表示的16位整型
    -------------------------------------------------------
    uint32_t ntohl(uint32_t netlong);
    将网络ip地址字节序转换成主机ip地址(32位)字节序
    返回值：以主机字节序表示的32位整型
    -------------------------------------------------------
    uint16_t ntohs(uint16_t netshort);
    将网络端口字节序转换成主机端口号(16位)字节序
    返回值：以主机字节序表示的16为整型
    *******************************************************
    int bind(int sockfd, const struct sockaddr *addr,socklen_t addrlen);
    将参数sockfd和addr绑定在一起，使sockfd这个用于网络通讯的文件描述符
    监听addr所描述的地址和端口号。
    sockfd：套接字文件描述符
    addr：
	struct sockaddr_in {
	    sa_family_t    sin_family;//确定通信特性
	    in_port_t      sin_port;
	    //本地端口号，需要将主机端口号字节序转换成网络字节序
	    //调用htons函数
	    struct in_addr sin_addr;
	};
	struct in_addr {
	    uint32_t       s_addr;
	    //主机ip地址，需要将主机ip地址字节序转换成网络字节序
	    //调用htonl函数
	};
    -----------------------------------------------------------------------
	struct sockaddr {
	    sa_family_t sa_family;
	    char        sa_data[14];
	};
    最后将sockaddr_in结构体强转成(sockaddr *)结构体地址
    -----------------------------------------------------------------------
    addrlen：addrlen指定结构体的长度,即sockaddr_in的长度
    返回值：成功返回0,错误返回-1
    ***********************************************************************
    int listen(int sockfd, int backlog);
    sockfd：套接字文件描述符
    backlog：提供一个提示
    返回值：成功返回0,错误返回-1
    ***********************************************************************
    int inet_pton(int af, const char *src, void *dst);
    将文本字符串格式转换成网络字节序的二进制地址
    af：通信特性，只支持AF_INET(32位),AF_INET6(128位)这两种
    -----------------------------------------------------------------------
    src：主机ip地址
    -----------------------------------------------------------------------
    dst：网络ip地址
    -----------------------------------------------------------------------
    返回值：成功返回1,格式无效返回0,错误返回-1
    ***********************************************************************
    const char *inet_ntop(int af, const void *src,char *dst, socklen_t size);
    af：通信特性，只支持AF_INET(32位),AF_INET6(128位)这两种
    -----------------------------------------------------------------------
    src：
    -----------------------------------------------------------------------
    size：指定保存文本字符串的缓存区(src)的大小
	INET_ADDRSTRLEN：定义了足够大的空间存放一个表示IPv4地址的文本字符串
	INET6_ADDRSTRLEN：定义了足够大的空间存放一个表示IPv6地址的文本字符串
    -----------------------------------------------------------------------
    返回值：成功返回1,格式无效返回0,错误返回-1
    ***********************************************************************
    int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
	返回的套接字描述符连接到调用connect的客户端
    这个新的套接字描述符和原始套接字(sockfd)具有相同的套接字类型和地址族
    传给accept的原始套接字没有关联到这个连接，而是继续保持可用状态并接收其它连接请求
    -----------------------------------------------------------------------
    sockfd：原始套接字描述符
    -----------------------------------------------------------------------
    addr：
	若不关心客户标识，可将addr和addrlen置NULL
	调用accept之前，将addr参数设为足够大的缓存区填充客户端的地址，
	并且更新指向addrlen的整数来反映该地址的大小
    -----------------------------------------------------------------------
    addrlen：
	返回值：成功返回新的套接字描述符，错误返回-1
    -----------------------------------------------------------------------
    备注：如果没有连接请求在等待，accept会阻塞直到有一个请求到来。
    如果原始套接字(sockfd)处于非初赛状态，
    accept会返回-1,将error设为 EAGAIN 或 EWOULDBLOCK

#endif

#define SERVE_PORT 8888
int main(){
    int lfd;//原始套接字描述符，用于创建套接字文件，服务器监听
    int cfd;//新套接字描述符，用于服务器跟客户端数据交互，读或写
    char buf[256];//使用新套接从套接字字文件读出数据，将数据缓存在buf里
//    char ip_buf[40];
    char ip_buf[INET_ADDRSTRLEN];//将网络ip地址字节序转换成主机ip地址字节序的文本字符串
    struct sockaddr_in saddr;//服务端结构体，保存通信特性，和指定的主机地址和端口
    struct sockaddr_in caddr;//缓存网络字节序中客户端的ip地址和端口
    socklen_t c_len;//定义服务端相应客户端后，保存客户端信息的sockaddr_in结构体大小
    //socklen_t是数据类型

    saddr.sin_family = AF_INET;
    saddr.sin_port=htons(SERVE_PORT);
    //saddr.sin_addr.s_addr=htonl(INADDR_ANY);
    inet_pton(AF_INET,"127.0.0.1",&saddr.sin_addr.s_addr);
    //创建套接字
    lfd=socket(AF_INET,SOCK_STREAM,0);
    //服务端绑定本地ip和端口，与套接字联系在一起
    int ret=bind(lfd,(struct sockaddr *)&saddr,sizeof(saddr));
    //监听
    listen(lfd,20);
    c_len=sizeof(caddr);//将大小设置为足够大的缓存区填充客户端的地址
    printf("等待客户端链接服务端 ...\n");
    //阻塞等待客户端连接
    cfd=accept(lfd,(struct sockaddr *)&caddr,&c_len);

    printf("服务端响应\n");

    printf("licent : ip[%s],port[%d]\n",
	inet_ntop(AF_INET,
	    &caddr.sin_addr.s_addr,ip_buf,INET_ADDRSTRLEN),ntohs(caddr.sin_port));
    while(1){
	ret=read(cfd,buf,sizeof(buf));
	if(ret==0){
	    printf("服务端断开链接\n");
//	    break;
	}
	ret=write(STDOUT_FILENO,buf,ret);
	write(cfd,"I am client\n",sizeof("I am client\n"));
    }
    return 0;
}
