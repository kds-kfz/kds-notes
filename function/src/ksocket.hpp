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

#include "kmemfile.hpp"
#include "klog.hpp"

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

# ifndef LOG_MODULE
# define LOG_MODULE "SOCK "
# endif

#define BUFF_SIZE (1024 * 8)
#define REQ_MAX_LEN (1024 * 7)

//设计缘由，单独的缓存区，不管任何情况，每次先接顺序为：长度、类型、消息内容
//根据每次接收的不同消息长度，选择循环读取还是一次性读取

//这是socket通信结构体，服务端于客户端公用
typedef struct{
    int fd;
    struct sockaddr_in addr;
    char ip_buf[64];
}Total_msg;

/*
serve:
sfd
cfd
ip_buff
cbuff
Msg_buff

client:
cfd
cbuff
Msg_buff
*/

//套接字通信类
class Socket{
private:
    std::string errmsg;              //错误信息
    char type[4];               //消息类型
    char buff[BUFF_SIZE];       //缓存内容
    Total_msg *sfd;             //服务端结构体
    Total_msg *cfd;             //客户端结构体
public:
    Socket(){
        //TODO 这里考虑可以用集成关系搭建，目的是减少内存分配，设计合理，毕竟服务端与客户端各自通信时需要的内存不全相同
        cfd = new Total_msg{0};
        sfd = new Total_msg{0};
        //memset(this->sfd,  0, sizeof(Total_msg));
        //memset(this->cfd,  0, sizeof(Total_msg));
        memset(this->type, 0, sizeof(this->type));
        memset(this->buff, 0, sizeof(buff));
        sfd->fd = 0;
        cfd->fd = 0;
        errmsg = "none";
    }
    
    ~Socket(){
        Close();
    }

    void Close(){
        if(!SocketServeClose()){
            ERROR_TLOG("%s\n", errmsg.c_str());
        }
        if(!SocketClientClose()){
            ERROR_TLOG("%s\n", errmsg.c_str());
        }
        delete []sfd;
        delete []cfd;
    }

    /**********     SocketServe 服务端     **********/
    //TODO SocketBuild SocketClose SocketRead SocketSend 这4个函数，可以写成纯虚函数，在各自派生类重写
    bool SocketServeBuild(){
        sfd->fd = socket(AF_INET,SOCK_STREAM,0);
        if(sfd->fd == -1){
            ERROR_TLOG("SocketServeBuild Fail!\n");
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
            ERROR_TLOG("SocketServeBind Fail!\n");
            return false;
        }
        return true;
    }

    bool SocketServeListen(int backlog){
        int ret = listen(sfd->fd, backlog);
        if(ret == -1){
            ERROR_TLOG("SocketServeListen Fail!\n");
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
            errmsg = "SocketServeClose Error:[" + std::string(strerror(ret)) + "]!";
            return false;
        }
    }
    
    std::string SocketShowAccept(bool flag = true){
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
        return std::string(msg);
    }
    
    bool SocketServeInit(const char *addr = "127.0.0.0", const char *port = "8080", int backlog = 128){
        return SocketServeBuild() == false ? false
            : SocketServeBind(addr, port) == false ? false
            : SocketServeListen(backlog) == false ? false
            : true;
    }
    
    bool SocketServeSend(const char *type, void *data){
        if(!strcmp(type, "01#")){
            //测试请求类型
            char *pbuf = buff;
            memset(pbuf, 0, sizeof(buff));
            char ResHead[11] = {0};
            sprintf(ResHead, "%8d01#", strlen((char *)data));
            memcpy(pbuf, ResHead, sizeof(ResHead));
            memcpy(pbuf + 11, (char *)data, strlen((char *)data));
            
            INFO_TLOG("serve send data:%s\n", buff);
            int ret = write(cfd->fd, buff, strlen(buff));
            //int ret = send(cfd->fd, &(buff), sizeof(Msg_buff), 0);
            if(ret < 0){
                errmsg = "Server Write To Client Fail!";
                return false;
            }
            return true;
        }
        errmsg = "[" + std::string(type) + "]，该服务尚未实现!";
        return false;
    }

    bool SocketServeSetup(){
        //8.5秒,有些系统未能精确到微秒
        //struct timeval tv = {3, 0};
        struct timeval tv;
        tv.tv_sec = 8;
        tv.tv_usec = 500000;
        #if 0
        //在send(), recv()过程有时由于网络等原因，收发不能如期进行，可设置收发时限
        //接受时限, 服务器接收请求，阻塞等待接收，当接收到半网络断开，超时后则需要重新接收
        //一般不用设置，否则客户端超时，则 accept 处于非阻塞状态
        if(setsockopt(sfd->fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(tv)) == -1){
            errmsg = "Setup Socket SO_RCVTIMEO Timeout Failed! Error:[" + std::string(strerror(errno)) + "]!";
            return false;
        }
        #endif
        //发送时限，服务器发送应答，数据过长，或者网络问题，应答超时,则重新发
        if(setsockopt(sfd->fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv)) == -1){
            errmsg = "Setup Socket SO_SNDTIMEO Timeout Failed! Error:[" + std::string(strerror(errno)) + "]!";
            return false;
        }
        return true;
    }

    //循环读取
    int SocketServeRead();
    
    char *SocketBuff(){
        return buff;
    }
    
    //服务端 客户端 公共函数
    std::string SocketErrmsg(){
        return errmsg;
    }

    char *SocketType(){
        return this->type;
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
            errmsg = "SocketClientConnect Error:[" + std::string(strerror(ret)) + "]!";
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
            errmsg = "SocketClientClose Error:[" + std::string(strerror(ret)) + "]!";
            return false;
        }
    }

    bool SocketClientInit(const char *addr = "127.0.0.1", const char *port = "8080"){
        return SocketClientBuild() == false ? false
            : SocketClientConnect(addr, port) == false ? false
            : true;
    }

    bool SocketClientSend(const char *type, void *data){
        if(!strcmp(type, "01#")){
            //测试请求类型
            char *pbuf = buff;
            memset(pbuf, 0, sizeof(buff));
            char ResHead[11] = {0};
            sprintf(ResHead, "%8d01#", strlen((char *)data));
            memcpy(pbuf, ResHead, sizeof(ResHead));
            memcpy(pbuf + 11, (char *)data, strlen((char *)data));
            
            INFO_TLOG("clint send data:[%s]\n", buff);
            int ret = write(cfd->fd, buff, strlen(buff));
            //int ret = send(cfd->fd, &(buff), sizeof(Msg_buff), 0);
            if(ret < 0){
                errmsg = "Server Write To Client Fail!";
                return false;
            }
            return true;
        }
        errmsg = "[" + std::string(type) + "], 该请求尚未实现!";
        return false;
    }

    //之所以客户端接收服务器应答也要校验，应答长度，应答类型，是因为有可能某个客户有多个请求类型
    int SocketClientRead();
    
    char *SocketClientBuff(){
        return buff;
    }

protected:
};

#endif
