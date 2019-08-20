#include"ksocket.hpp"
#include <unistd.h>

using namespace std;
using namespace kfz;

int main(int argc,char *argv[]){
    if(argc<2){
	    printf("./a.out less port\n");
	    exit(-1);
    }
    
    Socket client;
    /*
    if(client.SocketClientBuild()){
        cout<<"SocketClientBuild ok"<<endl;
        if(client.SocketClientConnect("127.0.0.1", argv[1])){
            cout<<"SocketClientConnect ok"<<endl;
        }
    }
    */
    if(!client.SocketClientInit("127.0.0.1", argv[1])){
        cout<<"SocketClientInit Fail"<<endl;
        exit(-1);
    }
    cout<<"SocketClientInit Success!"<<endl;
    
    //默认请求服务器数据
    Msg_buff req;
    memset(&req, 0, sizeof(Msg_buff));
    memcpy(req.type, "01", 2);
    memcpy(req.length, "00000011", 8);
    memcpy(req.info, "hello world", 11);

    while(1){
        sleep(3);
        if(!client.SocketClientSend(&req)){
            cout<<client.SocketErrmsg()<<endl;
        }
        if(!client.SocketClientRead()){
            cout<<client.SocketErrmsg()<<endl;
        }else{
            //已收到应答
            Msg_buff *pcbuff = (Msg_buff *)client.SocketClientBuff();
            cout<<"收到应答内容: "<<pcbuff->info<<endl;
        }
    }

    return  0;
}
