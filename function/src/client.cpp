#include"ksocket.hpp"
#include <unistd.h>

//单进程客户端
//多线程客户端，模拟高迸发请求
# ifndef LOG_MODULE
# define LOG_MODULE "CLIENT "
# endif

LOG_TYPE _gLogLevel = TEST;
Log *glog;

int main(int argc,char *argv[]){
    sleep(2);
    glog = new Log("../log/mount-service");
    
    if(argc<2){
        ERROR_TLOG("./a.out less port\n");
	    exit(-1);
    }
    
    Socket client;
    
    if(!client.SocketClientInit("127.0.0.1", argv[1])){
        ERROR_TLOG("SocketClientInit Fail!\n");
        exit(-1);
    }
    INFO_TLOG("SocketClientInit Success!\n");
    while(1){
        sleep(3);
        char buff[64] = "hello world";
        if(!client.SocketClientSend("01#", buff)){
            ERROR_TLOG("%s\n", client.SocketErrmsg().c_str());
        }
        if(client.SocketClientRead() != 1){
            ERROR_TLOG("%s\n", client.SocketErrmsg().c_str());
        }else{
            //已收到应答
            INFO_TLOG("收到应答类型:[%s]\n", client.SocketType());
            INFO_TLOG("收到应答内容:[%s]\n", client.SocketClientBuff());
        }
    }

    return  0;
}
