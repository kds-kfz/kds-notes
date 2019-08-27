#include <iostream>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <vector>
#include <stdarg.h>
#include <map>

using namespace std;

typedef struct task_node_t{
    char AppSysCode[128];
    char BranchCode[128];
}task_node_t;


//函数功能：删除字符串中出现空格前的字符，前提是字符串中末尾不能出现空格
//demo："hello world read" --> "read"
int CopyStringWithoutEmppty(char *dst, char *src, int len){
    int pos = 0, tmppos = 0;
    strncpy(dst, src, len);
    string tmpstr = string(dst);
    while(tmppos = tmpstr.find(' ') != -1 && pos <len){
        pos++;
        tmpstr = string(dst + pos);
    }
    memset(dst, 0, len);
    strncpy(dst, tmpstr.c_str(), tmpstr.size());
}

//函数功能：读取目录下指定格式的所有文件路径到vector容器
//demo：readFileList(".", ".md", files);
//新增，文件开头匹配，初略的比较
int readFileList(const char *basePath, const char *mark, vector<string> &files, const char *head = NULL){
    DIR *dir;
    struct dirent *ptr;
    char base[1024] = {0};
    if((dir = opendir(basePath)) == NULL){
        perror("Open dir error ...");
        exit(1);
    }
    while((ptr = readdir(dir)) != NULL){
        //跳过 . .. 以及隐藏文件或目录
        if(strncmp(ptr->d_name, ".", 1) == 0 || strcmp(ptr->d_name, "..") == 0){
            continue;
        }else if(ptr->d_type == 8){//file
            if(strstr(ptr->d_name, mark) != 0){
                memset(base, '\0', sizeof(base));
                strcpy(base, basePath);
                strcat(base, "/");
                if(head){
                    if(strstr(ptr->d_name, head) != 0){
                        strcat(base, ptr->d_name);
                        files.push_back(base);
                    }
                    continue;
                }
                strcat(base, ptr->d_name);
                files.push_back(base);
            }
        }else if(ptr->d_type == 10){//link file
            cout<<"link file==>d_name:"<<basePath<<ptr->d_name<<endl;
        }
        else if(ptr->d_type == 4){//dir 是目录则递归调用
            memset(base, '\0', sizeof(base));
            strcpy(base, basePath);
            strcat(base, "/");
            strcat(base, ptr->d_name);
            readFileList(base, mark, files);
        }
    }
    closedir(dir);
    return 0;
}

//函数功能：以任何格式打印出数据
//demo：debugout("当前日期：%s", GetTime("%Y%m%d").c_str());
void debugout(const char *fmt, ...){
    char buf[512];
    try{
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
        va_end(ap);
        cout<<buf<<endl;
    }catch(...){
        cout<<"exception in construct debug message"<<endl;
    }
}

//函数功能：获取本地时间
//demo：%Y->年，%m->月，%d->日，%H->时，%M->分，%S->秒
static string GetTime(const string& fmt)
{
    time_t tmt = time(NULL);
    struct tm *pTime = localtime(&tmt);
    char   timeBuf[64]={0};
    strftime(timeBuf,sizeof(timeBuf),fmt.c_str(),pTime);
    return string(timeBuf);
}

//函数功能：从字符串中截取字段到vector容器，分隔符是个字符，暂不支持字符字符有效校验
//demo：StringSplit((char *)":12:::2:3:4:a::33::", ':', files)
int StringSplit(char *src, const char split, vector<string> &files){
    int start = 0;
    int end = 0;
    int nSpiltCount = 0;
    char szTmp[256] = {0};
    string szCurrent = "";

    int nLength = strlen(src);
    if(nLength == 0){
        return 0;
    }

    for(int i = 0; i < nLength; i++){
        if(split == src[i]){
            //判断第一个分隔符出现前的字符和中间分隔符是否合法
            if(i == 0 || i > 0 && split == src[i - 1]){
                start += 1;
                continue;
            }
            nSpiltCount++;
            end = i;
            memcpy(szTmp, src + start, end - start);
            files.push_back(string(szTmp));
            start = end + 1;
        }
        memset(szTmp, 0, sizeof(szTmp));
    }
    //判断分隔符到结尾是否是有效字符
    if(start != nLength){
        memcpy(szTmp, src + start, nLength - start);
        files.push_back(string(szTmp));
    }
    //TODO
    //字符获取完成，字符有效校验待处理
    //写一个函数可获取数字，字符，特殊符号的函数对其校验
    return nSpiltCount;
}

//函数功能：从string中截取字段到vector容器
//demo：
int stringSplit( const string str, const string sep, vector <string> &vec )
{
    string::size_type begin, end;

    if ( str.length() == 0 ){
        return 0;
    }

    begin = 0;
    while(1){
        //end = str.find_first_of( sep, begin );
        end = str.find( sep, begin );
        if( end == string::npos ){
            break;
        }
        vec.push_back( str.substr( begin, end-begin ) );
        begin = end + sep.length();
    }
    vec.push_back( str.substr(begin) );

    return vec.size();
}

//守护进程初始化
void init_dameon(){
#ifdef __OS_LINUX_VER
    int pid;
    int i;
    if(pid = fork()){//是父进程，结束父进程
        exit(0);
    }else if(pid < 0){//fork 失败，退出
        exit(1);
    }

    //是第一子进程，后台继续执行
    
    //第一子进程成为新的会话组长和进程组长
    setsid();
    
    //关闭打开的文件描述符
    //for(i = 0; i < NOFILE; ++i){
    //    close(i);
    //}
    //改变工作目录到 /tmp
    //chdir("/tmp");

    //重设文件创建掩模
    umask(0);
    close(0);
    open("/dev/null", O_RDWR);
    dup2(0, 1);
    dup2(0, 2);

    //signal(SIGCHLD, SIG_IGN);

    return;
#endif
}

//信号回调函数
void SigHandlerProc(int signo){
    if(signo == SIGCHLD){
        cout<<"SigHandlerProc: signo = SIGCHLD"<<endl;
        exit(0);
    }else if(signo == SIGINT){
        cout<<"SigHandlerProc: signo = SIGINT"<<endl;
        delSem();
        exit(0);
    }else if(signo == SIGFPE || signo == SIGSEGV || signo == SIGBUS ||signo == SIGABRT){
        string temp;
        temp.clear();
        if(signo == SIGFPE){
            temp.appned("SIGFPE");
        }else if(signo == SIGSEGV){
            temp.append("SIGSEGV");
        }else if(signo == SIGBUS){
            temp.append("SIGBUS");
        }else if(signo == SIGABRT){
            temp.append("SIGABRT");
        }

        ZSocket so;
        int ret = so.Connect("127.0.0.1", 5669);
        if(ret){
            cout<<"SigHandlerProc: signo = "<<temp<<" task process failed!"<<endl;
            exit(0);
        }else{
            char msgbuff[1024] = {0}, *pmsg = NULL;
            pmsg = msgbuff;
            memcpy(msgbuff, "00000002", 8);
            pmsg += 8;
            memcpy(pmsg, "99", 2);
            pmsg += 2;
            ret = so.Send((void *)msgbuff, 10);
            if(ret <= 0){
                cout<<"SigHandlerProc: signo = "<<temp<<" task process msg failed!"<<endl;   
                so.Close();
                exit(0);
            }
        }

        close(so.Handle());
        cout<<"SigHandlerProc signo = "<<temp<<endl;
        exit(0);
    }
}


//信号处理函数
int initSigProc(){
    //SIGINT 程序终止（interrupt）信号，在用户键入INTR(通常是ctrl - c)时发出，用于通知前台进程组终止程序
    if(SIG_ERR == signal(SIGINT, SigHandlerProc)){
        return -1;
    }
    if(SIG_ERR == signal(SIGPIPE, SigHandlerProc)){
        return -1;
    }
    if(SIG_ERR == signal(SIGFPE, SigHandlerProc)){
        return -1;
    }

    //设置信号处理
    struct sigaction sa;
    sa.sa_handler = SigHandlerProc;

    //一般情况下，当信号处理函数运行时，内核将阻塞给定信号
    //但是如果设置了 SA_NODEFER 标记，那么在该信号处理函数运行时，内核不会阻塞该信号
    sa.sa_flags = SA_NODEFER;

    //清空信号集
    sigemptyset(&sa.sa_mask);

    //查询或设置信号处理方式
    //SIGSEGV 试图访问为分配给自己的内存，或者试图往没有权限的内存地址写数据
    /*
    int ret = sigaction(SIGSEGV, &sz, NULL);
    if(ret < 0){
        cout<<"Error: 设置信号 SIGSEGV 处理方式失败"<<endl;
        return ret;
    }
    ret = sigaction(SIGBUS, &sz, NULL);
    if(ret < 0){
        cout<<"Error: 设置信号 SIGBUS 处理方式失败"<<endl;
        return ret;
    }
    ret = sigaction(SIGABRT, &sz, NULL);
    if(ret < 0){
        cout<<"Error: 设置信号 SIGABRT 处理方式失败"<<endl;
        return ret;
    }
    */

    //SIG_IGN 这个符号表示忽略该信号，执行了相当于 signal() 调用后，进程会忽略类型为 sig 的信号
    sa.sa_handler = SIG_IGN;
    sa.sa_flags |= SA_NOCLDWAIT;
    int ret = sigaction(SIGCHLD, &sa, NULL);
    if(ret < 0){
        cout<<"Error: 设置信号 SIGCHLD 处理方式失败"<<endl;
        return ret;
    }

    return ret;
}

int main(int argc, char *argv[]){
    task_node_t node;
    vector<string> files;
    files.clear();
    memset(&node, 0, sizeof(node));
    char tempstr[64] = " hello world read";
    CopyStringWithoutEmppty(node.AppSysCode, tempstr, sizeof(tempstr));
    cout<<"["<<node.AppSysCode<<"]"<<endl;

    readFileList("../log", ".log", files, "asdad-123");
    cout<<"有效字符个数："<<StringSplit((char *)":12:::& $2:3:4:a::33::", ':', files)<<endl;
    for(vector<string>::iterator it = files.begin(); it != files.end(); it++){
        cout<<*it<<endl;
    }
    
    string path = "../log/mount-service";
    string yy = path.substr(0, path.find_last_of("/"));
    cout<<yy<<endl;
    yy = path.substr(path.find_last_of("/"));
    cout<<yy<<endl;
    
    debugout("当前日期：%s", GetTime("%Y%m%d%H%M%S").c_str());
    return 0;
}
