#ifndef __KPUBFUN__H__
#define __KPUBFUN__H__

#include <dirent.h>
#include <string>
#include <vector>

using namespace std;

class KPUBFUN{
public:
    /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 字 符 处 理 函 数 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
    
    //功能描述: 删除字符串中出现空格前的字符，前提是字符串中末尾不能出现空格
    static string CopyStringWithoutEmppty(char *dst, char *src, const char *flag);
    //功能描述: 只匹配一个字符
    static string StringMateOne(char *src, const char *mark, bool flag = true);
    //功能描述: 完全匹配字符串，此函数兼容 StringMateOne 函数，知道后并返回 mark 之后的字符
    static string StringMateMoreLast(char *src, const char *mark, bool flag = true);
    //功能描述: 完全匹配字符串，此函数兼容 StringMateOne 函数，知道后并返回 mark 之前的字符
    static string StringMateMoreFront(const char *src, const char *mark, bool flag = true);
    //功能描述: 从字符串中截取字段到vector容器，分隔符是个字符，暂不支持字符字符有效校验
    static int StringSplit(char *src, const char split, vector<string> &files);
    //功能描述: 从string中截取字段到vector容器
    static int stringSplit(const string str, const string sep, vector<string> &vec);
    
    /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 其 他 处 理 函 数 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
    
    //功能描述: 以任何格式打印出数据
    static void debugout(const char *fmt, ...);

    /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 日 期 处 理 函 数 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
    
    class Date{
        private:
            static time_t tmt;
            static struct tm *pTime;
            static char timeBuf[128];
            static string stime;
            static struct timeval tv;
            //struct timezone *ptz;
        public:
            Date();
            ~Date();

            //功能描述: 支持传入格式，比较灵活
            static string GetTime(const string& fmt);
            //功能描述: 获取本地时间
            static string GetTime();
    };

    /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 目 录 处 理 函 数 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
    
    class DirFile{
        private:
            static DIR *dir;               //目录描述符，类似文件描述符：FILE *
            static struct dirent *ptr;     //目录指针
            static char base[1024];        //基目录
        public:
            DirFile();
            ~DirFile();

            //功能描述: 读取目录下指定格式的所有文件路径到vector容器; 文件开头匹配, 初略的比较
            static int readFileList(const char *basePath, const char *mark, vector<string> &files, const char *head = NULL);
    };

};
#if 0
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
#endif

#endif
