#include"ksocket.hpp"
#include<sys/wait.h>

using namespace std;
using namespace kfz;

void fun(int n){
    int wai=waitpid(-1,NULL,WNOHANG);
    printf("waitpid = %d\n",wai);
}

int main(int argc, char *argv[]){
#if 0
    //文件测试
    Files fd;
    fd.Open("1.txt", O_RDONLY);
    cout<<"size:"<<fd.Size()<<endl;
    cout<<"date:"<<fd.Date();
    fd.Write("./","2.txt","hello");
    
    //字符串测试
    String str;
    char str_1[128] = {0};
    char str_2[128] = "hello world pp";
    cout<<"转换结果:"<<str.StringMateMore(str_2, " ")<<endl;
#endif
    //套接字测试
    if(argc<2){
        printf("./a.out less port\n");
        exit(-1);
    }

    pid_t pid;
    Socket serve;
    /*
    if(serve.SocketServeBuild()){
        cout<<"SocketServeBuild ok"<<endl;
        if(serve.SocketServeBind("127.0.0.1", argv[1])){
            cout<<"SocketServeBind ok"<<endl;
            if(serve.SocketServeListen(64)){
                cout<<"SocketServeListen ok"<<endl;
            }
        }
    }
    */
    if(!serve.SocketServeInit("127.0.0.1", argv[1], 128)){
        cout<<"SocketServeInit Fail!"<<endl;
        exit(-1);
    }
    cout<<"SocketServeInit Success!"<<endl;

    //默认应答客户端数据
    Msg_buff res;
    memset(&res, 0, sizeof(Msg_buff));
    memcpy(res.type, "02", 2);
    memcpy(res.length, "00000012", 8);
    memcpy(res.info, "receive data", 12);
    
    while(1){
        printf("Serve is waiting ...\n");
        if(serve.SocketServeAccept()){
            cout<<serve.SocketShowAccept()<<endl;
            if((pid = fork()) == -1){
                cout<<"fork fail"<<endl;
                exit(-1);
            }else if(pid == 0){
                if(!serve.SocketServeClose()){
                    cout<<"close sfd fail!"<<endl;
                    exit(-1);
                }
                
                while(1){
                    int ret = serve.SocketServeRead();
                    if(ret == -1){
                        cout<<serve.SocketErrmsg()<<endl;
                        continue;
                    }else if(ret == 0 || ret == -2){
                        cout<<serve.SocketErrmsg()<<endl;
                        exit(-1);
                    }
                     
                    //已收到请求
                    Msg_buff *psbuff = (Msg_buff *)serve.SocketServeBuff();
                    cout<<"收到请求内容: "<<psbuff->info<<endl;
                    if(!serve.SocketServeSend(&res)){
                        cout<<serve.SocketErrmsg()<<endl;
                    }
                }
            }else{
                serve.SocketClientClose();
                signal(SIGCHLD,fun);
            }
        }else{
            cout<<serve.SocketErrmsg()<<endl;
            exit(-1);
        }
    }
    return 0;
}
