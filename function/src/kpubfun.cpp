#include"kpubfun.hpp"

#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <cstring>
#include <stdarg.h>

#include "klog.hpp"

/* 2019年09月24日，对该文件重新设计，由于这是个功能函数文件，会有各种辅助功能函数，为方便管理故采用类中类设计 */

//日期 静态成员变量，类外初始化
time_t KPUBFUN::Date::tmt = {0};
char KPUBFUN::Date::timeBuf[128] = {0};
struct timeval KPUBFUN::Date::tv = {0};
struct tm *KPUBFUN::Date::pTime = NULL;
string KPUBFUN::Date::stime = "";

//目录 静态成员变量，类外初始化
DIR *KPUBFUN::DirFile::dir = NULL;
struct dirent *KPUBFUN::DirFile::ptr = NULL;
char KPUBFUN::DirFile::base[1024] = {0};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 字 符 处 理 函 数 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

//demo："hello world read" --> "read"
string KPUBFUN::CopyStringWithoutEmppty(char *dst, char *src, const char *flag){
    #if 0
    int len = strlen(src);
    int pos = 0, tmppos = 0;
    strncpy(dst, src, len);
    string tmpstr(dst);
    //不断更新 tmpstr 字符串，直到不在出现空格
    //TODO 要是字符串太长，得不断更新 tmpstr
    while(tmppos = tmpstr.find(' ') != string::npos && pos <len){
        pos++;
        tmpstr = string(dst + pos);
    }
    memset(dst, 0, len);
    strncpy(dst, tmpstr.c_str(), tmpstr.size());
    #endif
    
    string tmpstr(src);
    int len = strlen(src);
    string::size_type pos = 0;
    memset(dst, 0, len);
    if((pos = tmpstr.find_last_of(flag)) == string::npos){
        strncpy(dst, tmpstr.c_str(), tmpstr.size());
    }else{
        tmpstr = tmpstr.substr(pos + 1);
        strncpy(dst, tmpstr.c_str(), tmpstr.size());
    }
    return tmpstr;
}

//demo: 
string KPUBFUN::StringMateOne(char *src, const char *mark, bool flag){
    if(!strlen(src) && !strlen(mark))
        return string("");
    string tmpstr(src);
    string::size_type pos = 0;
    //从字符串后面查找，src 中 只要是出现 mark 中任何一个字符都算查找成功
    pos = flag ? tmpstr.find_last_of(mark) : tmpstr.find_first_of(mark);
    return pos == string::npos ? tmpstr : tmpstr.substr(pos + 1);
}

//demo: 
string KPUBFUN::StringMateMoreLast(char *src, const char *mark, bool flag){
    if(!strlen(src) && !strlen(mark))
        return string("");
    string tmpstr(src);
    string::size_type pos = 0;
    //从字符串后面查找，src 中 完全匹配 mark 字符串
    pos = flag ? tmpstr.rfind(mark) : tmpstr.find(mark);
    return pos == string::npos ? tmpstr : tmpstr.substr(pos + strlen(mark));
}

//demo: 
string KPUBFUN::StringMateMoreFront(const char *src, const char *mark, bool flag){
    if(!strlen(src) && !strlen(mark))
        return string("");
    string tmpstr(src);
    string::size_type pos = 0;
    //从字符串后面查找，src 中 完全匹配 mark 字符串
    pos = flag ? tmpstr.rfind(mark) : tmpstr.find(mark);
    return pos == string::npos ? tmpstr : tmpstr.substr(0, pos + strlen(mark));
}

//demo：StringSplit((char *)":12:::2:3:4:a::33::", ':', files)
int KPUBFUN::StringSplit(char *src, const char split, vector<string> &files){
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
            if(i == 0 || (i > 0 && (split == src[i - 1]))){
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

//demo：
int KPUBFUN::stringSplit(const string str, const string sep, vector<string> &vec){
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

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 日 期 处 理 函 数 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

KPUBFUN::Date::Date(){
    tmt = time(NULL);
    pTime = NULL;
    memset(timeBuf, 0, sizeof(timeBuf));
    stime = "";
    //this->ptv = NULL;
    //this->ptz = NULL;
}

KPUBFUN::Date::~Date(){
    pTime = NULL;
}
    
//demo：%Y->年，%m->月，%d->日，%H->时，%M->分，%S->秒
string KPUBFUN::Date::GetTime(const string& fmt){
    tmt = time(NULL);
    pTime = NULL;
    memset(timeBuf, 0, sizeof(timeBuf));
    stime = "";
    
    pTime = localtime(&tmt);
    strftime(timeBuf, sizeof(timeBuf), fmt.c_str(), pTime);
    stime = timeBuf;
    return stime;
}
    
//demo：
string KPUBFUN::Date::GetTime(){
    tmt = time(NULL);
    pTime = NULL;
    memset(timeBuf, 0, sizeof(timeBuf));
    stime = "";
    
    pTime = localtime(&tmt);

    gettimeofday(&tv, NULL);
    //gettimeofday(&tv, ptz);
    //localtime_r(&ptv->tv_sec, pTime);//线程安全
    
    //XXX 这里指针访问不到 tv_usec，不知为什么
    //sprintf(timeBuf, "%04d-%02d-%02d %02d:%02d:%02d.%06ld",
    //    pTime->tm_year + 1900, pTime->tm_mon + 1, pTime->tm_mday,
    //    pTime->tm_hour, pTime->tm_min,pTime->tm_sec, ptv->tv_usec);
        
    sprintf(timeBuf, "%04d-%02d-%02d %02d:%02d:%02d.%06ld",
        pTime->tm_year + 1900, pTime->tm_mon + 1, pTime->tm_mday,
        pTime->tm_hour, pTime->tm_min, pTime->tm_sec, tv.tv_usec);
    stime = timeBuf;
    return stime;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 其 他 处 理 函 数 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

//demo：debugout("当前日期：%s", GetTime("%Y%m%d").c_str());
void KPUBFUN::debugout(const char *fmt, ...){
    char buf[512] = {0};
    try{
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
        va_end(ap);
        TEST_TLOG("%s\n", buf);
    }catch(...){
        ERROR_TLOG("exception in construct debug message\n");
    }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 目 录 处 理 函 数 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

KPUBFUN::DirFile::DirFile(){
    dir = NULL;
    ptr = new dirent{0};
    memset(base, 0, sizeof(base));
}

KPUBFUN::DirFile::~DirFile(){
    closedir(dir);
    delete []ptr;
}

//demo：readFileList(".", ".md", files);
int KPUBFUN::DirFile::readFileList(const char *basePath, const char *mark, vector<string> &files, const char *head){
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
            TEST_TLOG("link file==>d_name:[%s] [%s]\n", basePath, ptr->d_name);
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

/*
typedef struct task_node_t{
    char AppSysCode[128];
    char BranchCode[128];
}task_node_t;

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
*/
