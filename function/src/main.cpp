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
    Socket serve;
    pid_t pid;
    if(!serve.SocketServeInit("127.0.0.1", "8089", 128)){
        cout<<"服务器初始化失败!错误信息为:"<<serve.SocketServeErrmsg()<<endl;
    }
    
    Msg_buff res;
    memset(&res, 0, sizeof(Total_msg));
    memcpy(res.type, "02", 2);
    memcpy(res.length, "00000012", 8);
    memcpy(res.info, "receive data", 12);
    
    while(1){
        cout<<"tttt"<<endl;
        serve.SocketServeFd();
        //if(serve.SocketServeAccept()){
        if(serve.SocketServeAccept(serve.SocketServeFd())){
            cout<<"SocketServeAccept ok"<<endl;
        }
            cout<<"服务启动成功"<<endl;
            cout<<"有客户接入"<<endl;
            //serve.SocketShowAccept();
            if((pid = fork()) == -1){
                cout<<"fork fail"<<endl;
                exit(-1);
            }else if(pid == 0){
                close(serve.SocketServeFd());
                
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
            }/*
        }else{
            cout<<serve.SocketServeErrmsg()<<endl;
            exit(-1);
        }*/
        cout<<"asdasdad"<<endl;
    }
    return 0;
}
