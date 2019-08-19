#include"pubfun.hpp"

using namespace std;
using namespace kfz;

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
    pid_t pid;
    Socket serve;
    int fd;
    if(serve.SocketServeBuild()){
        cout<<"SocketServeBuild ok"<<endl;
        if(serve.SocketServeBind("127.0.0.1", "8080")){
            cout<<"SocketServeBind ok"<<endl;
            if(serve.SocketServeListen(64)){
                cout<<"SocketServeListen ok"<<endl;
            }
        }
    }
    Msg_buff res;
    memset(&res, 0, sizeof(Msg_buff));
    memcpy(res.type, "02", 2);
    memcpy(res.length, "00000012", 8);
    memcpy(res.info, "receive data", 12);
    while(1){
        cout<<"tttt"<<endl;
        fd = serve.SocketServeFd();
        cout<<"sfd:"<<fd<<endl;
        printf("serve is waiting ...\n");
        if(serve.SocketServeAccept()){
            printf("----------accept ok----------\n");
            if((pid = fork()) == -1){
                cout<<"fork fail"<<endl;
                exit(-1);
            }else if(pid == 0){
                close(fd);
                
                while(1){
                     cout<<"正在接受客户端消息:"<<endl;
                     int ret = serve.SocketServeRead();
                     if(ret == -1){
                         cout<<serve.SocketServeErrmsg()<<endl;
                         continue;
                     }else if(ret == 0 || ret == -2){
                         cout<<serve.SocketServeErrmsg()<<endl;
                         exit(-1);
                     }
                     
                     //已收到请求
                     Msg_buff *psbuff = (Msg_buff *)serve.SocketServeBuff();
                     cout<<"收到请求内容: "<<psbuff->info<<endl;
                     if(!serve.SocketServeSend(&res)){
                         cout<<serve.SocketServeErrmsg()<<endl;
                     }
                }
            }else{
                serve.Close();
            }
        }else{
            cout<<serve.SocketServeErrmsg()<<endl;
            exit(-1);
        }
    }
    return 0;
}
