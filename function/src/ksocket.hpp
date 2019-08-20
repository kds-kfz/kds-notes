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

#define BUFF_SIZE (1024 * 8)
#define REQ_MAX_LEN (1024 * 7)

//设计缘由，单独的缓存区，不管任何情况，每次先接顺序为：长度、类型、消息内容
//根据每次接收的不同消息长度，选择循环读取还是一次性读取
/*
typedef struct{
//    char type[2];
    char info[BUFF_SIZE];//数据长度(8字节) + 数据类型(2字节) + '#' + 消息体(10字节之后到\0)
}Msg_buff;
*/

//这是socket通信结构体，服务端于客户端公用
typedef struct{
    int fd;
    struct sockaddr_in addr;
    char ip_buf[64];
//    Msg_buff buff;
    char buff[BUFF_SIZE];
}Total_msg;


//套接字通信类
class Socket{
private:
    string errmsg;              //错误信息
    char type[4];               //消息类型
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
    
    bool SocketServeSend(const char *type, void *data){
        if(!strcmp(type, "01#")){
            //测试请求类型
            char *pbuf = cfd->buff;
            memset(pbuf, 0, sizeof(cfd->buff));
            char ResHead[11] = {0};
            sprintf(ResHead, "%8d01#", strlen((char *)data));
            memcpy(pbuf, ResHead, sizeof(ResHead));
            memcpy(pbuf + 11, (char *)data, strlen((char *)data));
            
            cout<<"serve send data:"<<cfd->buff<<endl;
            int ret = write(cfd->fd, cfd->buff, strlen(cfd->buff));
            //int ret = send(cfd->fd, &(cfd->buff), sizeof(Msg_buff), 0);
            if(ret < 0){
                errmsg = "Server Write To Client Fail!";
                return false;
            }
            return true;
        }
        errmsg = "[" + string(type) + "]，该服务尚未实现!";
        return false;
    }
/*
    bool SocketServeSetup(){
        //8秒,有些系统未能精确到微秒
        struct timeval tv = {8, 0};
        //在send(), recv()过程有时由于网络等原因，收发不能如期进行，可设置收发时限
        //接受时限
        if(setsockopt(socket, SOL_S0CKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) == -1){
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
        
        memset(cfd->buff, 0, sizeof(cfd->buff));
        //读取的字节数大于 240*384 = 92160 = 90 * 1024, 90k
        //此时需要循环读取
        //而使用 recv 读取
#if 0
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
#endif
        int count = 0;
        //有可能接收的请求字节数超长，故先检验包体长度是否合法
        if((count = read(cfd->fd, cfd->buff, 8)) != 8){
            cout<<"Recv Header Error, break!"<<endl;
            return -1;
        }
        //这里要校验前8字节是否都为纯数字
        unsigned long pkg_len = atol(cfd->buff);
        cout<<"收到请求包体长度:"<<pkg_len<<endl;
        if(pkg_len <= 0 || pkg_len > REQ_MAX_LEN){
            cout<<"The Length oF Reqmsg Is Illigle Max["<<REQ_MAX_LEN<<"]!"<<endl;
            return -2;
        }
        memset(this->type, 0, sizeof(this->type));
        memset(cfd->buff, 0, sizeof(cfd->buff));
        int nbyte = 3;
        long tpkg_len = pkg_len;
        //每次最大读取 1024 字节
        char *pbuf = cfd->buff;
        while(nbyte){
            char TempBuff[1024] = {0};
            count = read(cfd->fd, TempBuff, nbyte);
            if(count < 0){
                if(errno == EINTR){
                    count = 0;
                    errmsg = "SocketServeRead Read EINTR!";
                    return -3;
                }else{
                    char temp[12] = {0};
                    sprintf(temp, "%d", count);
                    errmsg = "Recv Type is None Or Error:[" + string(strerror(errno)) + "][" + string(temp) + "]!";
                    return -4;
                }
            }else if(count == 0){
                errmsg = SocketShowAccept(false);                
                return 1;
                //break;
            }/*
            else{
                errmsg = "SocketServeRead Read Error!";
                return -5;
            }*/
            if(nbyte == 3 && TempBuff[2] == '#'){
                strcpy(this->type, TempBuff);
                nbyte = tpkg_len > sizeof(TempBuff) ? sizeof(TempBuff) : tpkg_len;
            }else{
                tpkg_len -= count;
                nbyte = tpkg_len > sizeof(TempBuff) ? sizeof(TempBuff) : tpkg_len;
                memcpy(pbuf, TempBuff, count);
                pbuf += count;
            }
        }
    }
    
    char *SocketServeBuff(){
        return cfd->buff;
    }
    
    //服务端 客户端 公共函数
    string SocketErrmsg(){
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

    bool SocketClientSend(const char *type, void *data){
        /*
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
        */
        if(!strcmp(type, "01#")){
            //测试请求类型
            char *pbuf = cfd->buff;
            memset(pbuf, 0, sizeof(cfd->buff));
            char ResHead[11] = {0};
            sprintf(ResHead, "%8d01#", strlen((char *)data));
            memcpy(pbuf, ResHead, sizeof(ResHead));
            memcpy(pbuf + 11, (char *)data, strlen((char *)data));
            
            cout<<"clint send data:"<<cfd->buff<<endl;
            int ret = write(cfd->fd, cfd->buff, strlen(cfd->buff));
            //int ret = send(cfd->fd, &(cfd->buff), sizeof(Msg_buff), 0);
            if(ret < 0){
                errmsg = "Server Write To Client Fail!";
                return false;
            }
            return true;
        }
        errmsg = "[" + string(type) + "], 该请求尚未实现!";
        return false;
    }

    //之所以客户端接收服务器应答也要校验，应答长度，应答类型，是因为有可能某个客户有多个请求类型
    int SocketClientRead(){
        /*
        memset(&(cfd->buff), 0, sizeof(Msg_buff));
        int ret = read(cfd->fd, &(cfd->buff), sizeof(Msg_buff));
        //ret == -1
        if(ret == -1){
            errmsg = "SocketClientRead Error:[" + string(strerror(errno)) + "]!";
            return false;
        }else if(ret == 0){
            errmsg = "SocketClientRead the EOF or Read 0 Byte!";
            return false;
        }
        return true;
        */
        int count = 0;
        memset(cfd->buff, 0, sizeof(cfd->buff));
        
        //有可能接收的请求字节数超长，故先检验包体长度是否合法
        if((count = read(cfd->fd, cfd->buff, 8)) != 8){
            cout<<"Recv Header Error, break!"<<endl;
            return -1;
        }
        //这里要校验前8字节是否都为纯数字
        unsigned long pkg_len = atol(cfd->buff);
        if(pkg_len <= 0 || pkg_len > REQ_MAX_LEN){
            cout<<"The Length oF Reqmsg Is Illigle Max["<<REQ_MAX_LEN<<"]!"<<endl;
            return -2;
        }
        
        memset(this->type, 0, sizeof(this->type));
        memset(cfd->buff, 0, sizeof(cfd->buff));
        int nbyte = 3;
        //每次最大读取 1024 字节
        char *pbuf = cfd->buff;
        long tpkg_len = pkg_len;
        cout<<"收到应答包体长度:"<<pkg_len<<endl;
        while(nbyte){
            char TempBuff[1024] = {0};
            count = read(cfd->fd, TempBuff, nbyte);
            if(count < 0){
                if(errno == EINTR){
                    count = 0;
                    errmsg = "SocketServeRead Read EINTR!";
                    return -3;
                }else{
                    char temp[12] = {0};
                    sprintf(temp, "%d", count);
                    errmsg = "Recv Type is None Or Error:[" + string(strerror(errno)) + "][" + string(temp) + "]!";
                    return -4;
                }
            }else if(count == 0){
                errmsg = "SocketClientRead the EOF or Read 0 Byte!";
                //return 1;
                break;
            }/*
            else if(count == -1){
                errmsg = "SocketClientRead Error:[" + string(strerror(errno)) + "]!";
            }*/
            
            if(nbyte == 3 && TempBuff[2] == '#'){
                strcpy(this->type, TempBuff);
                //第一次读取消息内容，默认读取整个包长
                //如果包长大于缓存大小，则每次最多接收缓存大小这么长的字节
                //及时实际包长很长，没此最大获取 sizeof(TempBuff) 个字节，直到获取完毕
                nbyte = tpkg_len > sizeof(TempBuff) ? sizeof(TempBuff) : tpkg_len;
            }else{
                tpkg_len -= count;
                nbyte = tpkg_len > sizeof(TempBuff) ? sizeof(TempBuff) : tpkg_len;
                memcpy(pbuf, TempBuff, count);
                pbuf += count;
            }
        }
        return 1;
    }
    
    char *SocketClientBuff(){
        return cfd->buff;
    }


protected:
};

}

#endif
