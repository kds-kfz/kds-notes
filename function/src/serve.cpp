#include"ksocket.hpp"
#include<sys/wait.h>

using namespace std;
//using namespace kfz;

//服务器，才用预编译，选择不同的服务器

void handle(int n){
    int wai = waitpid(-1, NULL, WNOHANG);
    printf("waitpid = %d\n", wai);
}

int main(int argc, char *argv[]){
    //套接字测试
    if(argc<2){
        printf("./a.out less port\n");
        exit(-1);
    }

    pid_t pid;
    Socket serve;
    
    if(!serve.SocketServeInit("127.0.0.1", argv[1], 128) || !serve.SocketServeSetup()){
        cout<<serve.SocketErrmsg()<<endl;
        cout<<"SocketServeInit Fail!"<<endl;
        exit(-1);
    }
    cout<<"SocketServeInit Success!"<<endl;

    //默认应答客户端数据
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
                    if(ret == -3){
                        cout<<serve.SocketErrmsg()<<endl;
                        continue;
                    }
                    cout<<"收到请求类型:"<<serve.SocketType()<<endl;
                    cout<<"收到请求内容:"<<serve.SocketBuff()<<endl;
                    char res[64] = "已收到测试请求包，正在处理...";
                    if(!serve.SocketServeSend("01#", res)){
                        cout<<serve.SocketErrmsg()<<endl;
                    }
                }
            }else{
                serve.SocketClientClose();
                signal(SIGCHLD, handle);
            }
        }else{
            cout<<serve.SocketErrmsg()<<endl;
            exit(-1);
        }
    }
    return 0;
}
