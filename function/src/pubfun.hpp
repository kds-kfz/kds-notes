#ifndef __PUBFUN__H__
#define __PUBFUN__H__

#include <iostream>
#include <string>

#include <cstdio>
#include <cstring>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


using namespace std;

namespace kfz{

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
     * 函数功能：完全匹配字符串，此函数兼容 StringMateOne 函数
     * demo: 
     * *********************************************************************/
    string StringMateMore(char *src, const char *mark, bool flag = true){
        if(!strlen(src) && !strlen(mark))
            return string("");
        string tmpstr(src);
        string::size_type pos = 0;
        //从字符串后面查找，src 中 完全匹配 mark 字符串
        pos = flag ? tmpstr.rfind(mark) : tmpstr.find(mark);
        return pos == string::npos ? tmpstr : tmpstr.substr(pos + strlen(mark));
    }

//只有在类内可以直接访问
private:

//类内、派生类可以访问；类外不能访问
protected:

};

//日期处理类
class Date{

};

//日志处理类
class Log{

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
class Json{

};

//xml数据格式处理类
class Xml{

};

//线程池

//对象池

//监控处理

//负载均衡

}

#endif
