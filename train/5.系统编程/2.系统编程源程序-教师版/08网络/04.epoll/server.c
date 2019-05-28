#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <errno.h>
#include <ctype.h>

#include "wrap.h"

#define MAXLINE 8192
#define SERV_PORT 8000
#define OPEN_MAX 5000

/*将文件描述符设置为非阻塞方式,例如:read读数据时,如果读不到是阻塞,
  进行非阻塞设置,读数据不会阻塞
 */
void setNonBlocking(int fd)
{
	int flag;
	flag = fcntl(fd,F_GETFL);
	if(flag == -1){
		fprintf(stderr,"fcntl error\n");
		exit(1);
	}
	flag |= O_NONBLOCK;
	if(fcntl(fd,F_SETFL) == -1){
		fprintf(stderr,"fcntl error\n");
	}
}

int main(int argc, char *argv[])
{
    int i, lfd, cfd, sockfd;
    int  n, num = 0;
    ssize_t nready, efd, res;
    char buf[MAXLINE], str[INET_ADDRSTRLEN];
    socklen_t clilen;

    struct sockaddr_in cliaddr, servaddr;
    struct epoll_event tep, ep[OPEN_MAX];       //tep: epoll_ctl参数  ep[] : epoll_wait参数

    lfd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));      //端口复用

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT);

    bind(lfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

    listen(lfd, 20);

    efd = epoll_create(OPEN_MAX);               //创建epoll模型, efd指向红黑树根节点
    if (efd == -1)
        perr_exit("epoll_create error");

    tep.events = EPOLLIN; tep.data.fd = lfd;           //指定lfd的监听事件为"读"
    res = epoll_ctl(efd, EPOLL_CTL_ADD, lfd, &tep);    //将lfd及对应的结构体设置到树上,efd可找到该树
    if (res == -1)
        perr_exit("epoll_ctl error");

    for ( ; ; ) {
        /*epoll为server阻塞监听事件, ep为struct epoll_event类型数组, OPEN_MAX为数组容量, -1表永久阻塞*/
        nready = epoll_wait(efd, ep, OPEN_MAX, -1); 
        if (nready == -1)
            perr_exit("epoll_wait error");

        for (i = 0; i < nready; i++) {
			/*我们这里只是处理的读事件集合*/
            if (!(ep[i].events & EPOLLIN))      //如果不是"读"事件, 继续循环
                continue;

            if (ep[i].data.fd == lfd) {    //判断满足事件的fd是不是lfd            
                clilen = sizeof(cliaddr);
                cfd = accept(lfd, (struct sockaddr *)&cliaddr, &clilen);    //接受链接

                printf("received from %s at PORT %d\n", 
                        inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)), 
                        ntohs(cliaddr.sin_port));
                printf("cfd %d---client %d\n", connfd, ++num);

                tep.events = EPOLLIN; tep.data.fd = connfd;
                res = epoll_ctl(efd, EPOLL_CTL_ADD, connfd, &tep);
                if (res == -1)
                    perr_exit("epoll_ctl error");

            } else {                                //不是lfd, 
                sockfd = ep[i].data.fd;
                n = read(sockfd, buf, MAXLINE);

                if (n == 0) {                       //读到0,说明客户端关闭链接
                    res = epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, NULL);  //将该文件描述符从红黑树摘除
                    if (res == -1)
                        perr_exit("epoll_ctl error");
                    close(sockfd);                  //关闭与该客户端的链接
                    printf("client[%d] closed connection\n", sockfd);

                } else if (n < 0) {                 //出错
                    perror("read n < 0 error: ");
                    res = epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, NULL);
                    close(sockfd);

                } else {                            //实际读到了字节数

                    write(STDOUT_FILENO, buf, n);
                    writen(sockfd, buf, n);
                }
            }
        }
    }
    close(listenfd);
    close(efd);

    return 0;
}

