struct sockaddr数据结构:
[16位地址类型][14字节地址数据]
struct sockaddr_in 数据结构:
[16位地址类型AF_INET][2字节的16位端口号][4字节的32位ip地址][8字节填充位]

struct sockaddr_in6 数据结构:
[16位地址类型AF_INET6][2字节的16位端口号][4字节的32位ip地址][8字节填充位]

struct sockaddr很多网络编程函数诞生早于ipv4,那时候都使用的是sockaddr结构体,为了向前兼容,
现在sockaddr退化成了(void*)的作用,传递一个地址函数,至于这个函数是sockaddr_in还是sockaddr_in6,
由地址族确定,然后函数内部再强制类型转化为所需要的地址类型


这三个函数涉及sockaddr_in 被强转sockaddr(它已经不能被使用)类型
bind()
accept()
connect()


struct sockaddr_in {
	sa_family_t    sin_family; /* address family: AF_INET */
	in_port_t      sin_port;   /* port in network byte order */
    struct in_addr sin_addr;   /* internet address */
};

/* Internet address. */
struct in_addr {
	uint32_t       s_addr;     /* address in network byte order */
};

struct sockaddr_in addr;
addr.sa_family_t = AF_INET;
addr.in_port_t = htons(8000);
addr.in_addr.s_addr = inet_pton()/htonl();
