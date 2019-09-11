#include"ksocket.hpp"
#include<sys/wait.h>
#include"klog.hpp"

//服务器，才用预编译，选择不同的服务器
# ifndef LOG_MODULE
# define LOG_MODULE "SERVER "
# endif

LOG_TYPE _gLogLevel = TEST;
Log *glog;

void handle(int n){
    int wai = waitpid(-1, NULL, WNOHANG);
    WARN_TLOG("waitpid = %d\n", wai);
}

int main(int argc, char *argv[]){
    glog = new Log("../log/mount-service");
    
    //套接字测试
    if(argc<2){
        ERROR_TLOG("./a.out less port\n");
        exit(-1);
    }

    pid_t pid;
    Socket serve;
    
    if(!serve.SocketServeInit("127.0.0.1", argv[1], 128) || !serve.SocketServeSetup()){
        ERROR_TLOG("SocketServeInit Fail Error:[%s]\n", serve.SocketErrmsg().c_str());
        exit(-1);
    }
    INFO_TLOG("SocketServeInit Success!\n");

    //默认应答客户端数据
    while(1){
        INFO_TLOG("Serve is waiting ...\n");
        if(serve.SocketServeAccept()){
            INFO_TLOG("%s\n", serve.SocketShowAccept().c_str());
            if((pid = fork()) == -1){
                INFO_TLOG("fork fail\n");
                exit(-1);
            }else if(pid == 0){
                if(!serve.SocketServeClose()){
                    INFO_TLOG("close sfd fail!\n");
                    exit(-1);
                }
                
                while(1){
                    int ret = serve.SocketServeRead();
                    if(ret == -3){
                        ERROR_TLOG("%s\n", serve.SocketErrmsg().c_str());
                        continue;
                    }
                    INFO_TLOG("收到请求类型:[%s]\n", serve.SocketType());
                    INFO_TLOG("收到请求内容:[%s]\n", serve.SocketBuff());
                    char res[64] = "已收到测试请求包，正在处理...";
                    if(!serve.SocketServeSend("01#", res)){
                        INFO_TLOG("%s\n", serve.SocketErrmsg().c_str());
                    }
                }
            }else{
                serve.SocketClientClose();
                signal(SIGCHLD, handle);
            }
        }else{
            ERROR_TLOG("%s\n", serve.SocketErrmsg().c_str());
            exit(-1);
        }
    }
    return 0;
}
