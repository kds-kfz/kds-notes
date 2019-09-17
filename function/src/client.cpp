#include"ksocket.hpp"
#include <unistd.h>
#include"kipc.hpp"

//单进程客户端
//多线程客户端，模拟高迸发请求
# ifndef LOG_MODULE
# define LOG_MODULE "CLIENT "
# endif

#define TEST_MODE 1

LOG_TYPE _gLogLevel = TEST;
Log *glog;

int main(int argc,char *argv[]){
    glog = new Log("../log/mount-service");
    
#if TEST_MODE == 1
    //套接字测试
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
    int count = 0;
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
            INFO_TLOG("客户端接收到数据[%d]\n", count++);
            INFO_TLOG("收到应答类型:[%s]\n", client.SocketType());
            INFO_TLOG("收到应答内容:[%s]\n", client.SocketClientBuff());
        }
    }
#elif TEST_MODE == 2
    //共享内存测试
    //共享内存初始化(客户端)
    if(!KIPC::initShmc()){
        cout<<"共享内存客户端初始化失败!"<<endl;
        exit(-1);
    }
    
    /*
    cout<<"g_msqid = "<<g_msqid<<endl;
    cout<<"g_semid = "<<g_semid<<endl;
    cout<<"g_shmid = "<<g_shmid<<endl;
    cout<<"g_key = "<<g_key<<endl;
    */

    while(1){
        KIPC::sendshm(1001, "#9001", "2019年9月13日,中秋快乐!");
        sleep(2);
        KIPC::sendshm(1001, "#9002", "");
        break;
    }
#elif TEST_MODE == 3
    //测试消息队列
    //消息队列初始化(客户端)
    if(!KIPC::initMsg()){
        cout<<"消息队列服务器初始化失败!"<<endl;
        exit(-1);
    }
    
    bool flag = true;
    while(flag){
        //向队列写入数据
        KIPC::sendMsg(1001, "#9001");
        sleep(2);
        KIPC::sendMsg(1001, "#9002");
        break;
    }
#elif TEST_MODE == 5
    //有名管道测试
    if(KIPC::readfifo(KIPC::buff) < 0){
        cout<<"有名管道读取数据失败!"<<endl;
    }
    cout<<"已读取有名管道内容:"<<KIPC::buff<<endl;
    
    string cmd;
    cmd.clear();
    cmd.append("rm -f ");
    cmd.append(FIFO_FILE_NAME);
    system(cmd.c_str());
    cout<<"已清除有名管道文件"<<endl;
#endif
    return  0;
}
