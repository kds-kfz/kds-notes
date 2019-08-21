#include"ksocket.hpp"
#include <unistd.h>

using namespace std;
using namespace kfz;

//单进程客户端
//多线程客户端，模拟高迸发请求

int main(int argc,char *argv[]){
    if(argc<2){
	    printf("./a.out less port\n");
	    exit(-1);
    }
    
    Socket client;
    
    if(!client.SocketClientInit("127.0.0.1", argv[1])){
        cout<<"SocketClientInit Fail"<<endl;
        exit(-1);
    }
    cout<<"SocketClientInit Success!"<<endl;
    while(1){
        sleep(3);
        char buff[64] = "hello world";
        if(!client.SocketClientSend("01#", buff)){
            cout<<client.SocketErrmsg()<<endl;
        }
        if(client.SocketClientRead() != 1){
            cout<<client.SocketErrmsg()<<endl;
        }else{
            //已收到应答
            cout<<"收到应答类型:"<<client.SocketType()<<endl;
            cout<<"收到应答内容:"<<client.SocketClientBuff()<<endl;
        }
    }

    return  0;
}
