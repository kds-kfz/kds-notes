#ifndef __KLOG_H__
#define __KLOG_H__

#include <stdarg.h>
#include <sys/types.h>
#include <pwd.h>
#include "kfile.hpp"
#include <map>
#include <string>

using namespace std;

# ifndef LOG_MODULE
# define LOG_MODULE "LOG "
# endif

typedef enum{
    ERROR = 1, WARN, INFO, DEBUG, TEST,
}LOG_TYPE;

typedef enum{
    KDS = 1, TC,
}COMPANYS;

//日志切割类型.按日志大小切割，建立天日志，建立小时日志
//使用情景：
//当某时间段内日志量过大时，可设置为小时日志
//默认设置为天日志
//天日志与小时日志都可以设置日志切割模式，日志名都为：mount-service-年月日-时分秒.log
typedef enum{
    
}SLICES;

//日志来源，作者：获取本地用户 公司：指定公司
typedef struct{
    string _author;
    COMPANYS _company;
}LOG_SOURCE;

//此句位置声明很重要
extern LOG_TYPE _gLogLevel;

class Log{
public:
    //由于其他文件共享同一日志文件，故为公共变量
    Files *_plog;
private:
    string _logpatch;
    LOG_TYPE _logtype;
    LOG_SOURCE _logsource;
    bool _slice;                           //切割标志
    unsigned int _targetsize;              //文件大小
    //static LOG_TYPE _gLogLevel;             //类内静态成日志级别
public:
    //无参构造
    /*
    Log(){
        this->_logpatch = "";
        _plog = new Files();
        _pdate = new Date();
        _logtype = TEST;
        _logsource._author = "";
        _logsource._company = OTHER;
    }
    */
    //有参构造
    Log(string path, LOG_TYPE type = TEST, COMPANYS company = TC);

    //校验当前文件是否满足切割条件，真：条件满足，需要切割
    bool IsAccord(){
        Files fd;
        fd.Open(this->_logpatch.c_str(), O_RDONLY);
        return !(_slice > 0 && _targetsize > 0) ? false : fd.Size() >= _targetsize * 1024 * 1024 ? true : false;
    }

    //日志写入
    //设计的来源是 KDS 日志的实现，原版只是单一的cpp与头文件
    //重新设计成日志类，可更好的条调用日志方法，方法内不同地方设计成宏替换
    //该函数值输出不定参数的内容
    void Log_Msg(COMPANYS company, LOG_TYPE type, const char *fmt, ... );
    
    ~Log(){
        delete _plog;
    }
};

extern Log *glog;

#define DETAIL(company, level, fmt, args...) glog->Log_Msg(company, level, \
        " " "%d %d" " " LOG_MODULE "%s:%d %s: " fmt, \
            getppid(), getpid(), __FILE__, __LINE__, __FUNCTION__, ##args)
//天创日志
#define ERROR_TLOG(fmt, args...) DETAIL(TC, ERROR, fmt, ##args)
#define WARN_TLOG(fmt, args...)  DETAIL(TC, WARN,  fmt, ##args)
#define INFO_TLOG(fmt, args...)  DETAIL(TC, INFO,  fmt, ##args)
#define DEBUG_TLOG(fmt, args...) DETAIL(TC, DEBUG, fmt, ##args)
#define TEST_TLOG(fmt, args...)  DETAIL(TC, TEST,  fmt, ##args)
//盈通日志
#define ERROR_KLOG(fmt, args...) DETAIL(KDS, ERROR, fmt, ##args)
#define WARN_KLOG(fmt, args...)  DETAIL(KDS, WARN,  fmt, ##args)
#define INFO_KLOG(fmt, args...)  DETAIL(KDS, INFO,  fmt, ##args)
#define DEBUG_KLOG(fmt, args...) DETAIL(KDS, DEBUG, fmt, ##args)
#define TEST_KLOG(fmt, args...)  DETAIL(KDS, TEST,  fmt, ##args)

#endif
