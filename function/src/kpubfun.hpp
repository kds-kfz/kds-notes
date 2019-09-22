#ifndef __KPUBFUN__H__
#define __KPUBFUN__H__

#include <iostream>
#include <string>

#include <cstdio>
#include <cstring>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <vector>
#include <sys/time.h>

using namespace std;

//namespace kfz{

//字符串处理类
class String{
//类内，类外都可以访问
public:
    /* **********************************************************************
     * 函数功能：删除字符串中出现空格前的字符，前提是字符串中末尾不能出现空格
     * demo："hello world read" --> "read"
     * *********************************************************************/
    string CopyStringWithoutEmppty(char *dst, char *src, const char *flag){
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

    /* **********************************************************************
     * 函数功能：只匹配一个字符
     * demo: 
     * *********************************************************************/
    string StringMateOne(char *src, const char *mark, bool flag = true){
        if(!strlen(src) && !strlen(mark))
            return string("");
        string tmpstr(src);
        string::size_type pos = 0;
        //从字符串后面查找，src 中 只要是出现 mark 中任何一个字符都算查找成功
        pos = flag ? tmpstr.find_last_of(mark) : tmpstr.find_first_of(mark);
        return pos == string::npos ? tmpstr : tmpstr.substr(pos + 1);
    }

    /* **********************************************************************
     * 函数功能：完全匹配字符串，此函数兼容 StringMateOne 函数，知道后并返回 mark 之后的字符
     * demo: 
     * *********************************************************************/
    string StringMateMoreLast(char *src, const char *mark, bool flag = true){
        if(!strlen(src) && !strlen(mark))
            return string("");
        string tmpstr(src);
        string::size_type pos = 0;
        //从字符串后面查找，src 中 完全匹配 mark 字符串
        pos = flag ? tmpstr.rfind(mark) : tmpstr.find(mark);
        return pos == string::npos ? tmpstr : tmpstr.substr(pos + strlen(mark));
    }

    /* **********************************************************************
     * 函数功能：完全匹配字符串，此函数兼容 StringMateOne 函数，知道后并返回 mark 之前的字符
     * demo: 
     * *********************************************************************/
    string StringMateMoreFront(const char *src, const char *mark, bool flag = true){
        cout<<"pos:++++"<<endl;
        if(!strlen(src) && !strlen(mark))
            return string("");
        cout<<"pos:====="<<endl;
        string tmpstr(src);
        string::size_type pos = 0;
        cout<<"pos:"<<pos<<endl;
        //从字符串后面查找，src 中 完全匹配 mark 字符串
        pos = flag ? tmpstr.rfind(mark) : tmpstr.find(mark);
        return pos == string::npos ? tmpstr : tmpstr.substr(0, pos + strlen(mark));
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
//只有在类内可以直接访问
private:

//类内、派生类可以访问；类外不能访问
protected:

};

//日期处理类
class Date{
private:
    time_t tmt;
    struct tm *pTime;
    char timeBuf[128];
    string stime;
    
    struct timeval tv;
    //struct timezone *ptz;
public:
    Date(){
        this->tmt = time(NULL);
        this->pTime = NULL;
        memset(timeBuf, 0, sizeof(timeBuf));
        stime = "";
        
        //this->ptv = NULL;
        //this->ptz = NULL;
    }
    
    //支持传入格式，比较灵活
    string GetTime(const string& fmt){
        pTime = localtime(&tmt);
        strftime(timeBuf, sizeof(timeBuf), fmt.c_str(), pTime);
        stime = timeBuf;
        return stime;
    }
    
    //获取本地时间
    string GetTime(){
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

    ~Date(){
        pTime = NULL;
    }
};

#if 0
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
#endif

//目录文件处理类
class DirFile{
private:
    DIR *dir;           //目录描述符，类似文件描述符：FILE *
    struct dirent *ptr; //目录指针
    char base[1024];    //基目录
public:
    DirFile(){
        dir = NULL;
        ptr = new dirent{0};
        memset(base, 0, sizeof(base));
    }

    //函数功能：读取目录下指定格式的所有文件路径到vector容器
    //demo：readFileList(".", ".md", files);
    //新增，文件开头匹配，初略的比较
    int readFileList(const char *basePath, const char *mark, vector<string> &files, const char *head = NULL){
        
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
    ~DirFile(){
        closedir(dir);
        delete []ptr;
    }
};
//http通信类
class Http{

};

//信号处理类
class Signal{

};

//守护进程处理
class Daemon{

};

//json数据格式处理类

//xml数据格式处理类
class Xml{

};

//线程池

//对象池

//监控处理

//负载均衡

//}

#endif
