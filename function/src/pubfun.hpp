#ifndef __PUBFUN__H__
#define __PUBFUN__H__

#include <iostream>
#include <string>
#include <vector>
#include <map>

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

//文件处理类
class Files{
private:
    int _fd;
    off_t _size;
    char *_date;
public:
#if 0
    //打开文件
    bool open(const char *filename, const char *mode){
        _fp = fopen(filename, mode);
	    if(_fp == NULL){
            std::cout<<"open error"<<std::endl;
	        return false;
	    }
        //计算文件大小
        _size = fseek(_fp, 0, SEEK_END) == 0 ? ftell(_fp) : -1;
        //重置文件光标位置
        fseek(_fp, 0, SEEK_SET);
        //获取文件内容
        _date = new char[_size + 1];
        //fscanf 读到\0 或者空格结束，但是空文件，此方法不可行
        char *pdate = _date;
	    while(1){
            char buf[1024] = {0};
            int ret = fscanf(_fp, "%s", buf);
            if(ret < 0){
                std::cout<<"fscanf fail"<<std::endl;
                return false;
            }else if(ret == 1){
                memcpy(pdate, buf, strlen(buf));
                pdate += strlen(buf);
            }else{
                break;
            }
        }
        //读到换行或文件末尾结束，此方法不行
	    //fgets(_date, _size, _fp);
        return true;
    }
#endif
    //无参构造函数
    Files(){
        _fd = 0;
        _size = 0;
        _date = new char[1];
        *_date = '\0';
    }
    //打开文件
    bool Open(const char *filename, int flags){
        if((_fd = open(filename, flags)) == -1){
            std::cout<<"open file fail"<<std::endl;
            return false;
        }else{
            _size = lseek(_fd, 0L, SEEK_END);
            lseek(_fd, 0L, SEEK_SET);
            _date = new char[_size + 1];
            memset(_date, 0, _size + 1);
            //获取文件内容
            int ret;
            char ch;
            char *pbuf = _date;
            int byte = _size > 1023 ? 1024 : _size;
            while(1){
                #if 0
                //每次读取一个字节
                if((ret = read(_fd, &ch, 1)) == -1){
                    fprintf(stderr,"read read file by one byte error:[%s]\n",strerror(ret));
                    return false;
                }else if(ret != 0){
                    memcpy(pbuf, &ch, ret);
                    pbuf += ret;
                }else{
                    break;
                }
                #endif
                //每次读取指定个数字节
                char buf[1025] = {0};
                if((ret = read(_fd, buf, byte)) == -1){
                    fprintf(stderr,"read read file by 1024 byte error:[%s]\n",strerror(ret));
                    return false;
                }else if(ret != 0){
                    memcpy(pbuf, buf, ret);
                    pbuf += ret;
                }else{
                    break;
                }
            }
        }
        return true;
    }
    //生成一个新文件
    bool Write(const char *basePath, const char *newfile, const char *content = NULL){
        //查找当前目录是否已经有该文件
        cout<<strlen(content)<<endl;
        DIR *dir;
        struct dirent *ptr;
        bool flag = false;
        char olddir[512] = {0};
        char newdir[512] = {0};
        if(strcmp(basePath, "./") == 0){
            strcat(newdir, "./");
            strcat(newdir, newfile);
        }
        
        //拓展功能，支持指定目录生成文件，TODO：待兼容
        //绝对路径
        //getcwd(currdir, sizeof(newdir));
        //strcat(newdir, "/");
        //strcat(newdir, newfile);
        
        if((dir = opendir(basePath)) == NULL){
            perror("Open dir error ...");
            return false;
        }
        while((ptr = readdir(dir)) != NULL){
            //跳过 . .. 以及隐藏文件或目录
            if(strncmp(ptr->d_name, ".", 1) == 0 || strcmp(ptr->d_name, "..") == 0){
                continue;
            }else if(ptr->d_type == 10){//link file
            
            }else if(ptr->d_type == 4){//dir
            
            }else if(ptr->d_type == 8){//file
                memset(olddir, '\0', sizeof(olddir));
                strcpy(olddir, basePath);
                strcat(olddir, ptr->d_name);
                if(strcmp(olddir, newdir) == 0){
                    flag = true;
                    break;
                }
            }
        }
        
        int fw = 0;
        int ret = 0;
        
        if(!flag){//文件还没有
            if((fw = open(newdir ,O_CREAT|O_RDWR,0666)) == -1){
                cout<<"open newdir fail"<<endl;
                return false;
            }else{
                if(content == NULL){
                    if((ret = write(fw, this->_date, this->_size)) == -1){
                        fprintf(stderr,"write to newfile error[%s]", strerror(ret));
                        return false;
                    }
                }else{
                    if((ret = write(fw, content, strlen(content))) == -1){
                        fprintf(stderr,"write to newfile error[%s]", strerror(ret));
                        return false;
                    }
                }
            }
            close(fw);
        }else{//文件已经存在
            Files ft;//局部类，离开作用于自动析构
            if(ft.Open(newdir, O_RDWR) && ft.Size() == 0){
                if(content == NULL){
                    if((ret = write(ft.Handle(), this->_date, this->_size)) == -1){
                        fprintf(stderr,"write to newfile error[%s]", strerror(ret));
                        return false;
                    }
                }else{
                    if((ret = write(ft.Handle(), content, strlen(content))) == -1){
                        fprintf(stderr,"write to newfile error[%s]", strerror(ret));
                        return false;
                    }
                }
            }else{
                fprintf(stderr, "newfile is already exists and newfile the size is not eq 0\n");
                return false;
            }
        }
    }
    int Handle(){
        return _fd;
    }
    long Size(){
        return _size;
    }
    char *Date(){
        return _date;
    }
    //析构函数
    ~Files(){
        if(_fd){
            close(_fd);
        }
        delete []_date;
        _size = 0;
    }
};

//字符串处理类
class String{
//类内，类外都可以访问
public:
//函数功能：删除字符串中出现空格前的字符，前提是字符串中末尾不能出现空格
//demo："hello world read" --> "read"
string CopyStringWithoutEmppty(char *dst, char *src, const char *flag){
    #if 0
    int len = strlen(src);
    int pos = 0, tmppos = 0;
    strncpy(dst, src, len);
    string tmpstr(dst);
    //不断更新 tmpstr 字符串，直到不在出现空格
    //TODO 要是字符串太长，得不断更新 tmpstr
    while(tmppos = tmpstr.find(' ') != string::npos && pos <len){
        pos++;
        tmpstr = string(dst + pos);
    }
    memset(dst, 0, len);
    strncpy(dst, tmpstr.c_str(), tmpstr.size());
    #endif
    
    string tmpstr(src);
    int len = strlen(src);
    string::size_type pos = 0;
    memset(dst, 0, len);
    if((pos = tmpstr.find_last_of(flag)) == string::npos){
        strncpy(dst, tmpstr.c_str(), tmpstr.size());
    }else{
        tmpstr = tmpstr.substr(pos + 1);
        strncpy(dst, tmpstr.c_str(), tmpstr.size());
    }
    return tmpstr;
}

//只匹配一个字符
string StringMateOne(char *src, const char *mark, bool flag = true){
    if(!strlen(src) && !strlen(mark))
        return string("");
    string tmpstr(src);
    string::size_type pos = 0;
    //从字符串后面查找，src 中 只要是出现 mark 中任何一个字符都算查找成功
    pos = flag ? tmpstr.find_last_of(mark) : tmpstr.find_first_of(mark);
    return pos == string::npos ? tmpstr : tmpstr.substr(pos + 1);
}

//完全匹配字符串
//此函数兼容 StringMateOne 函数
string StringMateMore(char *src, const char *mark, bool flag = true){
    if(!strlen(src) && !strlen(mark))
        return string("");
    string tmpstr(src);
    string::size_type pos = 0;
    //从字符串后面查找，src 中 完全匹配 mark 字符串
    pos = flag ? tmpstr.rfind(mark) : tmpstr.find(mark);
    return pos == string::npos ? tmpstr : tmpstr.substr(pos + strlen(mark));
}
//只有在类内可以直接访问
private:

//类内、派生类可以访问；类外不能访问
protected:
};

//日期处理类
class Date{

};

//日志处理类
class Log{

};

//http通信类
class Http{

};

//套接字通信类
class Socket{
private:
    string errmsg;              //错误信息
    Total_msg *sfd;             //服务端结构体
    Total_msg *cfd;             //客户端结构体
    /*
    char sbuff[2048];           //服务端缓存通信内容
    char cbuff[2048];           //客户端缓存通信内容
    
    int sfd;                    //服务端套接字
    struct sockaddr_in saddr;   //服务端套接字初始化信息
    
    int cfd;                    //客户端套接字
    struct sockaddr_in caddr;   //客户端套接字初始化信息
    */
public:
    Socket(){
        errmsg = "";
        /*
        memcpy(sfd->buff, 0, sizeof(sfd->buff));
        memcpy(cfd->buff, 0, sizeof(cfd->buff));
        bzero(&sfd->addr, sizeof(sfd->addr));
        bzero(&cfd->addr, sizeof(cfd->addr));
        */
        //sfd = (Total_msg *)malloc(sizeof(Total_msg));
        //cfd = (Total_msg *)malloc(sizeof(Total_msg));
        //memset(sfd, 0, sizeof(Total_msg));
        //memset(cfd, 0, sizeof(Total_msg));
        //sfd->fd = 0;
        //cfd->fd = 0;
        //errmsg = "none";
    }
    ~Socket(){
        //Close();
    }
    void Close(){
        close(sfd->fd);
        close(cfd->fd);
        delete []sfd;
        delete []cfd;
    }
    //SocketServe 服务端
    bool SocketServeBuild(){
        sfd->fd = socket(AF_INET,SOCK_STREAM,0);
        if(sfd->fd == -1){
            errmsg = "SocketServeBuild Fail!";
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
            errmsg = "SocketServeBind Fail!";
            return false;
        }
        return true;
    }

    bool SocketServeListen(int backlog){
        int ret = listen(sfd->fd, backlog);
        if(ret == -1){
            errmsg = "SocketServeListen Fail!";
            return false;
        }
        /*
        socklen_t c_len = sizeof(sockaddr_in);
        cout<<"+++:"<<c_len<<endl;
        cout<<"+-+:"<<sfd->fd<<endl;
        cfd->fd = accept(sfd->fd, (struct sockaddr *)&cfd->addr, &c_len);
        cout<<"---:"<<cfd->fd<<endl;
        if(cfd->fd == -1){
            errmsg = "SocketServeAccept Fail!";
            return false;
        }
*/
        return true;
    }

    //XXX
    bool SocketServeAccept(int sfd){
        cout<<"->>-:"<<sfd<<endl;
        socklen_t c_len = sizeof(sockaddr_in);
        cfd->fd = accept(sfd, (struct sockaddr *)&cfd->addr, &c_len);
        cout<<"---:"<<cfd->fd<<endl;
        if(cfd->fd == -1){
            errmsg = "SocketServeAccept Fail!";
            return false;
        }

        return true;
    }
    //XXX
    int SocketServeFd(){
        cout<<"errmsg:"<<errmsg<<endl;
    }
    
    void SocketShowAccept(){
        printf("client ip[%s],port[%d]\n",
                 inet_ntop(AF_INET,&cfd->addr.sin_addr.s_addr,cfd->ip_buf,sizeof(cfd->ip_buf)),
                 ntohs(cfd->addr.sin_port));
    }
    
    bool SocketServeInit(const char *addr = "127.0.0.0", const char *port = "8080", int backlog = 128){
        /*
        return SocketServeBuild() == false ? false
            : SocketServeBind(addr, port) == false ? false
            : SocketServeListen(backlog) == false ? false
            : true;
        */
        if(SocketServeBuild()){
            cout<<"SocketServeBuild ok"<<endl;
            if(SocketServeBind(addr, port)){
                cout<<"SocketServeBind ok"<<endl;
                if(SocketServeListen(backlog)){
                    cout<<"SocketServeListen ok"<<endl;
                    return true;
                }
            }
        }
        return false;
    }
    
    bool SocketServeSend(void *data){
        Msg_buff *sp = (Msg_buff *)data;
        memcpy(&(sfd->buff), 0, sizeof(Msg_buff));
        memcpy(&(sfd->buff), sp, sizeof(Msg_buff));
        //int ret = write(sfd->fd, sfd->buff, strlen(sfd->buff));
        int ret = send(sfd->fd, &(sfd->buff), sizeof(Msg_buff), 0);
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
        
        memcpy(&(sfd->buff), 0, sizeof(Msg_buff));
        //读取的字节数大于 240*384 = 92160 = 90 * 1024, 90k
        //此时需要循环读取
        //而使用 recv 读取
        int ret = read(sfd->fd, &(sfd->buff), sizeof(Msg_buff));
        if(ret == 0){
            errmsg = "SocketServeRead Fail, client is breaked!";
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
        return &(sfd->buff);
    }
    
    Msg_buff *SocketClientBuff(){
        return &(cfd->buff);
    }

    string SocketServeErrmsg(){
        return errmsg;
    }

    //SocketClient 客户端
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
            errmsg = "SocketClientConnect error:[" + string(strerror(ret)) + "]!";
            return false;
        }
        return true;
    }

    bool SocketClientInit(const char *addr = "127.0.0.1", const char *port = "8080"){
        return SocketClientBuild() == false ? false
            : SocketClientConnect(addr, port) == false ? false
            : true;
    }

    bool SocketClientSend(void *data){
        Msg_buff *cp = (Msg_buff *)data;
        memcpy(&(cfd->buff), 0, sizeof(cfd->buff));
        memcpy(&(cfd->buff), &(cp), sizeof(Msg_buff));
        //int ret = write(cfd->fd, cfd->buff, strlen(cfd->buff));
        int ret = send(cfd->fd, &(cfd->buff), sizeof(Msg_buff), 0);
        if(ret < 0){
            errmsg = "Client Write To Server Fail!";
            return false;
        }
        return true;
    }

    bool SocketClientRead(){
        memcpy(&(cfd->buff), 0, sizeof(Msg_buff));
        int ret = read(cfd->fd, &(cfd->buff), sizeof(Msg_buff));
        //ret == -1
        if(ret == -1){
            errmsg = "SocketClientRead error:[" + string(strerror(ret)) + "]!";
            return false;
        }else if(ret == 0){
            errmsg = "SocketClientRead the EOF or Read 0 Byte!";
            return false;
        }
        return true;
    }

protected:
};

//信号处理类
class Signal{

};

//守护进程处理
class Daemon{

};

//json数据格式处理类
class Json{

};

//xml数据格式处理类
class Xml{

};

//线程池

//对象池

//监控处理

//负载均衡

}

#endif
