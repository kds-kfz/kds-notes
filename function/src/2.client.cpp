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
    
    if(!client.SocketClientInit("127.0.0.1", argv[1])){
        cout<<"SocketClientInit Fail"<<endl;
        exit(-1);
    }
    cout<<"SocketClientInit Success!"<<endl;
    /*
    //默认请求服务器数据
    Msg_buff req;
    memset(&req, 0, sizeof(Msg_buff));
    memcpy(req.type, "01", 2);
    memcpy(req.length, "00000011", 8);
    memcpy(req.info, "hello world", 11);
*/
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
            /*
            Msg_buff *pcbuff = (Msg_buff *)client.SocketClientBuff();
            cout<<"收到应答内容: "<<pcbuff->info<<endl;
            */
            cout<<"收到应答类型: "<<client.SocketType()<<endl;
            cout<<"收到应答内容: "<<client.SocketClientBuff()<<endl;

        }
    }

    return  0;
}
