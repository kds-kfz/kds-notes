#ifndef __KSOCKET__H__
#define __KSOCKET__H__

#include <iostream>
#include <string>

#include <cstdio>
#include <cstring>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstdlib>

/*******************************
说明(explain):
这是个套接字处理类，属于公共类部分
此类集成了套接字所有用法，可以支持一下服务器的建立
(1) 多进程套接字通信服务器 
(2) 多线程套接字通信服务器 
* 考虑到网络原因或是有中断事件打断socket通信，使用 select 或 epoll 搭建服务器
* 发送 send  与 接收 recv
* 发送 write 与 接收 read
* 考虑 每次通信传输的数据长度可能接收不完整，其原因可能是网络或是字节太大
* 故采用设置收发延时，阻塞等待

*******************************/

using namespace std;

namespace kfz{

typedef struct{
    char length[8];
    char type[2];
    char info[2048];//数据长度(8字节) + 数据类型(2字节) + 消息体(10字节之后到\0)
}Msg_buff;

//这是socket通信结构体，服务端于客户端公用
typedef struct{
    int fd;
    struct sockaddr_in addr;
    char ip_buf[64];
    Msg_buff buff;
}Total_msg;


//套接字通信类
class Socket{
private:
    string errmsg;              //错误信息
    Total_msg *sfd;             //服务端结构体
    Total_msg *cfd;             //客户端结构体
public:
    Socket(){
        //TODO 这里考虑可以用集成关系搭建，目的是减少内存分配，设计合理，毕竟服务端与客户端各自通信时需要的内存不全相同
        cfd = new Total_msg;
        sfd = new Total_msg;
        memset(sfd, 0, sizeof(Total_msg));
        memset(cfd, 0, sizeof(Total_msg));
        sfd->fd = 0;
        cfd->fd = 0;
        errmsg = "none";
    }
    
    ~Socket(){
        Close();
    }

    void Close(){
        if(!SocketServeClose()){
            cout<<errmsg<<endl;
        }
        if(!SocketClientClose()){
            cout<<errmsg<<endl;
        }
        delete []sfd;
        delete []cfd;
    }

    /**********     SocketServe 服务端     **********/
    //TODO SocketBuild SocketClose SocketRead SocketSend 这4个函数，可以写成纯虚函数，在各自派生类重写
    bool SocketServeBuild(){
        sfd->fd = socket(AF_INET,SOCK_STREAM,0);
        if(sfd->fd == -1){
            //errmsg = "SocketServeBuild Fail!";
            perror("SocketServeBuild Fail!");
            return false;
        }
        return true;
    }
    
    bool SocketServeBind(const char *addr, const char *port){
        sfd->addr.sin_family = AF_INET;
        sfd->addr.sin_port = htons(atoi(port));
        inet_pton(AF_INET, addr, &sfd->addr.sin_addr.s_addr);
        int ret = bind(sfd->fd, (struct sockaddr *)&sfd->addr, sizeof(sfd->addr));
        if(ret == -1){
            //errmsg = "SocketServeBind Fail!";
            perror("SocketServeBind Fail!");
            return false;
        }
        return true;
    }

    bool SocketServeListen(int backlog){
        int ret = listen(sfd->fd, backlog);
        if(ret == -1){
            //errmsg = "SocketServeListen Fail!";
            perror("SocketServeListen Fail!");
            return false;
        }
        return true;
    }

    bool SocketServeAccept(){
        socklen_t c_len = sizeof(sockaddr_in);
        cfd->fd = accept(sfd->fd, (struct sockaddr *)&cfd->addr, &c_len);
        if(cfd->fd == -1){
            errmsg = "SocketServeAccept Fail!";
            return false;
        }

        return true;
    }
    
    bool SocketServeClose(){
        if(sfd->fd < 0){
            errmsg = "The Serve Sfd was Shuted!";
            return false;
        }
        
        int ret = close(sfd->fd);
        if(ret == 0){
            return true;
        }else if(ret == -1){
            errmsg = "SocketServeClose Error:[" + string(strerror(ret)) + "]!";
            return false;
        }
    }
    
    string SocketShowAccept(bool flag = true){
        char msg[512] = {};
        if(flag){
            sprintf(msg, "client ip[%s], port[%d] Has been Accessed!",
                 inet_ntop(AF_INET, &cfd->addr.sin_addr.s_addr, cfd->ip_buf, sizeof(cfd->ip_buf)),
                 ntohs(cfd->addr.sin_port));
        }else{
            sprintf(msg, "client ip[%s], port[%d] Has been Disconnecting!",
                 inet_ntop(AF_INET, &cfd->addr.sin_addr.s_addr, cfd->ip_buf, sizeof(cfd->ip_buf)),
                 ntohs(cfd->addr.sin_port));
        }
        return string(msg);
    }
    
    bool SocketServeInit(const char *addr = "127.0.0.0", const char *port = "8080", int backlog = 128){
        return SocketServeBuild() == false ? false
            : SocketServeBind(addr, port) == false ? false
            : SocketServeListen(backlog) == false ? false
            : true;
    }
    
    bool SocketServeSend(void *data){
        Msg_buff *cp = (Msg_buff *)data;
        memset(&(cfd->buff), 0, sizeof(Msg_buff));
        memcpy(&(cfd->buff), cp, sizeof(Msg_buff));
        int ret = write(cfd->fd, &(cfd->buff), sizeof(cfd->buff));
        //int ret = send(cfd->fd, &(cfd->buff), sizeof(Msg_buff), 0);
        if(ret < 0){
            errmsg = "Server Write To Client Fail!";
            return false;
        }
        return true;
    }
/*
    bool SocketServeSetup(){
        //8秒,有些系统未能精确到微秒
        struct timeval tv = {8, 0};
        //在send(), recv()过程有时由于网络等原因，收发不能如期进行，可设置收发时限
        //接受时限
        if(setsockopt(socket，SOL_S0CKET,SO_RCVTIMEO，(char *)&tv,sizeof(tv) == -1){
            errmsg = "Error: Setup Socket Timeout Failed!";
            return false;
        }
        //发送时限
        //setsockopt (socket，SOL_S0CKET,SO_SNDTIMEO，(char *)&nNetTimeout,sizeof(int));
        return true;
    }
*/
    int SocketServeRead(){
        //设置通信时间
        //timeval tv;
        //tv.tv_sec = timeout / 1000;
        //tv.tv_usec = (timeout $ 1000) * 1000;
        //select(sfd->fd +1, sfd->buff, 0, 0, &tv);
        
        memset(&(cfd->buff), 0, sizeof(Msg_buff));
        //读取的字节数大于 240*384 = 92160 = 90 * 1024, 90k
        //此时需要循环读取
        //而使用 recv 读取
        int ret = read(cfd->fd, &(cfd->buff), sizeof(Msg_buff));
        if(ret == 0){
            errmsg = SocketShowAccept(false);
            //errmsg = "SocketServeRead Fail, client is breaked!";
            return 0;
        }else if(ret < 0){
            if(errno == EINTR){
                errmsg = "SocketServeRead Read EINTR!";
                return -1;
            }else{
                errmsg = "SocketServeRead Read Error!";
                return -2;
            }
        }
        return 1;
    }
    
    Msg_buff *SocketServeBuff(){
        return &(cfd->buff);
    }
    
    //服务端 客户端 公共函数
    string SocketErrmsg(){
        return errmsg;
    }

    /**********     SocketClient 客户端     **********/
    bool SocketClientBuild(){
        cfd->fd = socket(AF_INET, SOCK_STREAM, 0);
        if(cfd->fd == -1){
            errmsg = "SocketClientBuild Fail!";
            return false;
        }
        return true;
    }

    bool SocketClientConnect(const char *addr, const char *port){
        //memset(&cfd->addr, 0, sizeof(cfd->addr));
        cfd->addr.sin_family = AF_INET;
        cfd->addr.sin_port = htons(atoi(port));
        inet_pton(AF_INET, addr, &cfd->addr.sin_addr.s_addr);
        int ret = connect(cfd->fd,(struct sockaddr *)(&(cfd->addr)),sizeof(cfd->addr));
        if(ret == -1){
            errmsg = "SocketClientConnect Fail!";
            return false;
        }else if(ret != 0){
            errmsg = "SocketClientConnect Error:[" + string(strerror(ret)) + "]!";
            return false;
        }
        return true;
    }
    
    bool SocketClientClose(){
        if(cfd->fd < 0){
            errmsg = "The Serve Sfd was Shuted!";
            return false;
        }
        
        int ret = close(cfd->fd);
        if(ret == 0){
            return true;
        }else if(ret == -1){
            errmsg = "SocketClientClose Error:[" + string(strerror(ret)) + "]!";
            return false;
        }
    }

    bool SocketClientInit(const char *addr = "127.0.0.1", const char *port = "8080"){
        return SocketClientBuild() == false ? false
            : SocketClientConnect(addr, port) == false ? false
            : true;
    }

    bool SocketClientSend(void *data){
        Msg_buff *cp = (Msg_buff *)data;
        memset(&(cfd->buff), 0, sizeof(cfd->buff));
        memcpy(&(cfd->buff), cp, sizeof(Msg_buff));
        int ret = write(cfd->fd, &(cfd->buff), sizeof(cfd->buff));
        //int ret = send(cfd->fd, &(cfd->buff), sizeof(Msg_buff), 0);
        if(ret < 0){
            errmsg = "Client Write To Server Fail!";
            return false;
        }
        return true;
    }

    bool SocketClientRead(){
        memset(&(cfd->buff), 0, sizeof(Msg_buff));
        int ret = read(cfd->fd, &(cfd->buff), sizeof(Msg_buff));
        //ret == -1
        if(ret == -1){
            errmsg = "SocketClientRead Error:[" + string(strerror(ret)) + "]!";
            return false;
        }else if(ret == 0){
            errmsg = "SocketClientRead the EOF or Read 0 Byte!";
            return false;
        }
        return true;
    }
    
    Msg_buff *SocketClientBuff(){
        return &(cfd->buff);
    }


protected:
};

}

#endif
