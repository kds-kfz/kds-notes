#include<sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include "ksocket.hpp"
#include "klog.hpp"
#include "kipc.hpp"
#include "ksignal.hpp"

//服务器，才用预编译，选择不同的服务器
# ifndef LOG_MODULE
# define LOG_MODULE "SERVER "
# endif

#define TEST_MODE 1

LOG_TYPE _gLogLevel = TEST;
Log *glog;

//extern char pipebuff[PIPE_MAX_SIZE];

void handle(int n){
    int wai = waitpid(-1, NULL, WNOHANG);
    WARN_TLOG("waitpid = %d\n", wai);
}

int main(int argc, char *argv[]){
    glog = new Log("../log/mount-service");
    INFO_TLOG("挂载系统初始化成功...\n");
    
    //创建信号量集
    if(!KIPC::creatSem(IPC_MODE)){
        exit(1);
    }
    //初始化信号量
    if(!KIPC::initSem(1)){
        exit(1);
    }
    //信号注册
    if(!initSigProc()){
        exit(1);
    }
#if TEST_MODE == 1
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
    int count = 0;
    //默认应答客户端数据
    while(1){
        INFO_TLOG("Serve is waiting ...\n");
        if(serve.SocketServeAccept()){
            INFO_TLOG("%s\n", serve.SocketShowAccept().c_str());
            if((pid = fork()) == -1){
                INFO_TLOG("fork fail\n");
                exit(1);
            }else if(pid == 0){
                if(!serve.SocketServeClose()){
                    INFO_TLOG("close sfd fail!\n");
                    exit(1);
                }
                
                while(1){
                    int ret = serve.SocketServeRead();
                    if(ret == -3){
                        ERROR_TLOG("%s\n", serve.SocketErrmsg().c_str());
                        continue;
                    }
                    INFO_TLOG("接收到第[%d]个请求\n", count++);
                    INFO_TLOG("收到请求类型:[%s]\n", serve.SocketType());
                    INFO_TLOG("收到请求内容:[%s]\n", serve.SocketBuff());
                    char res[64] = "已收到测试请求包，正在处理...";
                    if(!serve.SocketServeSend("01#", res)){
                        WARN_TLOG("%s\n", serve.SocketErrmsg().c_str());
                        break;
                    }
                }
            }else{
                serve.SocketClientClose();
                //signal(SIGCHLD, handle);
            }
        }else{
            WARN_TLOG("%s\n", serve.SocketErrmsg().c_str());
            //exit(1);
        }
    }
    //删除信号量
    KIPC::delsem();
#elif TEST_MODE == 2
    //共享内存测试
    //共享内存初始化(服务器)
    if(!KIPC::initShms()){
        cout<<"共享内存服务器初始化失败!"<<endl;
        exit(-1);
    }
    
    /*
    cout<<"g_msqid = "<<g_msqid<<endl;
    cout<<"g_semid = "<<g_semid<<endl;
    cout<<"g_shmid = "<<g_shmid<<endl;
    cout<<"g_key = "<<g_key<<endl;
    */

    bool flag = true;
    while(flag){
        //读消息队列数据
        KIPC::recvshm(1001, flag);
    }

    //删除共享内存
    if(KIPC::delShm()){
        cout<<"已经删除共享内存，谢谢!"<<endl;
    }
#elif TEST_MODE == 3
    //测试消息队列
    //消息队列初始化(服务器)
    cout<<"测试模式:"<<TEST_MODE<<endl;
    if(!KIPC::initMsg()){
        cout<<"消息队列服务器初始化失败!"<<endl;
        exit(-1);
    }
    cout<<"消息队列初始化成功"<<endl;
    bool flag = true;
    while(flag){
        //读消息队列数据
        KIPC::recvMsg(1001, &KIPC::msg, flag);
        sleep(1);
    }
    //删除消息队列
    if(KIPC::delMsg()){
        cout<<"已经删除消息队列，谢谢!"<<endl;
    }
#elif TEST_MODE == 4
    pid_t pid;
    //无名管道测试
    if(!KIPC::initPipe()){
        cout<<"无名管道服务器初始化失败!"<<endl;
        exit(-1);
    }
    if((pid = fork()) < 0){
        ERROR_TLOG("fork fail!\n");
        exit(-1);
    }else if(pid > 0){//父进程
        KIPC::writechild("迎接国庆!");
    }else{
        KIPC::readchild(KIPC::buff);
    }
#elif TEST_MODE == 5
    //有名管道测试
    if(!KIPC::initfifo()){
        cout<<"有名管道服务器初始化失败!"<<endl;
    }
    if(KIPC::writefifo("国庆倒计时中!") < 0){
        cout<<"有名管道写入数据失败!"<<endl;
    }
#endif
    INFO_TLOG("挂载系统正在退出，欢迎使用...\n");
    return 0;
}
