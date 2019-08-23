#ifndef __KLOG_H__
#define __KLOG_H__

#include <stdarg.h>
#include <pwd.h>
#include"kfile.hpp"
#include"pubfun.hpp"

namespace kfz{

typedef enum{
    ERROR = 1, INFO, DEBUG, TEST,
}LOG_TYPE;

typedef enum{
    KDS = 1, TC, OTHER,
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

class Log{
private:
    Files *_plog;
    string _logpatch;
    LOG_TYPE _logtype;
    LOG_SOURCE _logsource;
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
    Log(string path, LOG_TYPE type = TEST, COMPANYS company = TC){
        _plog = new Files();
        if(path.empty()){
            path = "../log/mount-service";
        }
        Date date;
        string nowtime = date.GetTime("-%Y%m%d-%H%M%S.log");
        path.append(nowtime);
        this->_logpatch = path;
        this->_logtype = type;
        
        //获取当前用户
        struct passwd *pwd = getpwuid(getuid());
        _logsource._author = string(pwd->pw_name);
        _logsource._company = company;
    }

    //日志写入
    void logMsg(COMPANYS company, LOG_TYPE type, const char *fmt, ... ){
        if(this->_plog->Handle() == 0){
            //追加方式打开
            //这里需要判断，如果有文件，且文件大小没达到切割的条件则不要新创文件
            this->_plog->Open(this->_logpatch.c_str(), O_APPEND);
        }
        /*
        //string nowtime = this->_pdate->GetTime(" %Y-%m-%d %H:%M:%S ");
        fprintf(this->_plog->Handle(), "%s %s %s", 
                ( type == KDS ? "KDS" :  type == TC ? "TC" : "---" ),
                _logsource._author.c_str(),
                _pdate->GetTime(" %Y-%m-%d %H:%M:%S ").c_str()
               );
        
        try{
            va_list args;
            va_start( args, fmt );
            vfprintf( gLogFp, fmt, args );
            va_end( args );

            fflush( this->_plog->Handle() );
        }catch(...){
            cout<<"exception in construct debug message"<<endl;
        }
        */

        char buf[2048] = {0};
        char buf1[2048];
        char *pbuf = buf;
        Date date;
        sprintf(pbuf, "%s %s %s %s",
            ( company == KDS ? "KDS" :  company == TC ? "TC" : "---" ),
            ( type == ERROR ? "ERR" : type == DEBUG ? "DBG" : type == INFO ? "RUN" : "---" ),
            _logsource._author.c_str(),
            date.GetTime(" %Y-%m-%d %H:%M:%S ").c_str()
            );
        pbuf += strlen(buf);
        try{
            va_list ap;
            va_start(ap, fmt);
            vsnprintf(pbuf, sizeof(buf) - strlen(buf), fmt, ap);
            va_end(ap);
            _plog->Write(buf);
        }catch(...){
            cout<<"exception in construct debug message"<<endl;
        }

    }
void logMsg(const char *fmt, ...){
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
    ~Log(){
        delete _plog;
    }
};

/*
# ifndef LOG_MODULE
# define LOG_MODULE "LOG "
# endif

# define ERR_LOG( fmt, args... ) \
    logMsg( ERROR, LOG_MODULE " " "%s:%d:%s: "fmt"\n", __FILE__, __LINE__, __FUNCTION__, ##args )
# define WARN_LOG( fmt, args... ) \
    logMsg( INFO, LOG_MODULE " " "%s:%d:%s: "fmt"\n", __FILE__, __LINE__, __FUNCTION__, ##args )
# define DBG_LOG( fmt, args... ) \
    logMsg( DEBUG, LOG_MODULE " - " fmt "\n", ##args )
# define TEST_LOG( fmt, args... ) \
    logMsg( TEST, LOG_MODULE " - " fmt "\n", ##args )
# define RUN_LOG( fmt, args... ) \
    logMsg( INFO, LOG_MODULE " - " fmt "\n", ##args )
*/

}

#endif
