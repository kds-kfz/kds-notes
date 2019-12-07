#include<stdio.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<strings.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define SERVER_PORT 8888
//多线程_SERVER.C

void fun(int n){
    int wai=waitpid(-1,NULL,WNOHANG);
    printf("waitpid : %d\n",wai);
}

int main(){
    int sfd,cfd;
    int ret;
    struct sockaddr_in saddr;
    struct sockaddr_in caddr;
    socklen_t c_len;
    char ip_buf[INET_ADDRSTRLEN];
    char buf[128];

    pid_t pid;

    sfd=socket(AF_INET,SOCK_STREAM,0);
    if(sfd==-1){
        perror("socket error\n");
        exit(-1);
    }
    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(SERVER_PORT);
    //saddr.sin_addr.s_addr=htonl(INADDR_ANY);
    inet_pton(AF_INET,"127.0.0.1",&saddr.sin_addr.s_addr);

    ret=bind(sfd,(struct sockaddr *)&saddr,sizeof(saddr));
    if(ret==-1){
        perror("bind fail\n");
        exit(-1);
    }
    ret=listen(sfd,128);
    if(ret==-1){
        perror("listen fail\n");
        exit(-1);
    }
    while(1){
        c_len=sizeof(caddr);
        cfd=accept(sfd,(struct sockaddr *)&caddr,&c_len);
        if(cfd==-1){
            perror("accept error\n");
            exit(-1);
        }
        printf("client ip[%s],port[%d]\n",
                inet_ntop(AF_INET,&caddr.sin_addr.s_addr,ip_buf,INET_ADDRSTRLEN),
                caddr.sin_port);	
        pid=fork();
        if(pid==-1){
            perror("fork error\n");
            exit(-1);
        }
        else if(pid==0){
            close(sfd);
            break;
        }
        else{
            close(cfd);
            signal(SIGCHLD,fun);//等待进程结束
        }
    }
    if(pid==0){
        while(1){
            ret=read(cfd,buf,sizeof(buf));
            if(ret==0){
                perror("客户端断开\n");
                exit(-1);
            }
            else if(ret<0){
                if(errno==EINTR)
                    continue;
                else{
                    perror("read error\n");
                    exit(-1);
                }
            }
            write(STDOUT_FILENO,buf,ret);
            write(cfd,"server recieve data ok\n",
                    sizeof("server recieve data ok\n"));
        }
    }
    return 0;
}
