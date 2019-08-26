#ifndef __KLOG_H__
#define __KLOG_H__

#include <stdarg.h>
#include <sys/types.h>
#include <pwd.h>
#include"kfile.hpp"
#include"pubfun.hpp"
#include <map>

namespace kfz{

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

class Log{
private:
    Files *_plog;
    string _logpatch;
    LOG_TYPE _logtype;
    LOG_SOURCE _logsource;
    bool _slice;             //切割标志
    unsigned long _targetsize;                //文件大小
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
        //初始化前先校验该目录是否已存在日志，匹配所有以 mount-service 开头的日志文件
        //校验其大小
        //将../log/ 目录下所有 mount-service 开头的日志文件放到容器中
        //vector<string, map<int, int> > 
        //<"mount-service-20190826-111352.log", <20190826, 111352>>
        
        string validpath = path;
        size_t npos = path.find_last_of("/");
        string basepath = path.substr(0, npos);
        string headpath = path.substr(npos + 1);
        DirFile dirfile;
        vector<string> files; //符合条件的文件
        dirfile.readFileList(basepath.c_str(), ".log", files, headpath.c_str());
        if(!files.empty()){
            vector< map< string, map< long, long> > > dirpath;
            for(int i = 0; i < files.size(); i++){
                //文件全路径 files[i]
                //考虑到有可能文件很多且大，不宜使用文件操作中，文件大小的方式来判断
                //目前判断日期和时间
                String path;
                vector <string> destpath;
                path.stringSplit(files[i], "-", destpath);
                map<long, long> inside;
                map<string , map<long, long> > outside;
                inside.insert(make_pair(atol(destpath[2].c_str()), atol(destpath[3].c_str())));
                outside.insert(make_pair(files[i], inside));
                dirpath.push_back(outside);
            }
            
            map<string, map<long, long> >::iterator outit = dirpath[0].begin();
            map<long, long>::iterator insideit = outit->second.begin();
            validpath = outit->first;
            long nyr = insideit->first;
            long sfm = insideit->second;
            for(int i = 1; i < dirpath.size(); i++){
                outit = dirpath[i].begin();
                insideit = outit->second.begin();
                long tnyr = insideit->first;
                long tsfm = insideit->second;
                bool flag = false;
                if(tnyr == nyr ){
                    if(tsfm > sfm){
                        flag = true;
                    }
                }else if(tnyr > nyr){
                    flag = true;
                }
                if(flag){
                    nyr = tnyr;
                    sfm = tsfm;
                    validpath = outit->first;
                }
            }

            Files fd;
            fd.Open(validpath.c_str(), O_RDONLY);
            bool ok = fd.Size() >= 10 * 1024 * 1024 ? true : false;
            if(ok){
                Date date;
                string nowtime = date.GetTime("-%Y%m%d-%H%M%S.log");
                path.append(nowtime);
                validpath = path;
                this->_plog->Open(validpath.c_str(), O_RDWR); // 新建
            }
        }else{
            Date date;
            string nowtime = date.GetTime("-%Y%m%d-%H%M%S.log");
            path.append(nowtime);
            validpath = path;
            this->_plog->Open(validpath.c_str(), O_APPEND);//追加
        }
        
        this->_logpatch = validpath;
        this->_logtype = type;
        this->_slice = true;
        this->_targetsize = 10;
        
        //获取当前用户
        struct passwd *pwd = getpwuid(getuid());
        _logsource._author = string(pwd->pw_name);
        _logsource._company = company;
    }

    //校验当前文件是否满足切割条件，真：条件满足，需要切割
    bool IsAccord(){
        Files fd;
        fd.Open(this->_logpatch.c_str(), O_RDONLY);
        return !(_slice > 0 && _targetsize > 0) ? false : fd.Size() >= _targetsize * 1024 * 1024? true : false;
    }

    //日志写入
    //设计的来源是 KDS 日志的实现，原版只是单一的cpp与头文件
    //重新设计成日志类，可更好的条调用日志方法，方法内不同地方设计成宏替换
    //该函数值输出不定参数的内容
    void Log_Msg(COMPANYS company, LOG_TYPE type, const char *fmt, ... ){
        /*
        if(this->_plog->Handle() == 0){
            //追加方式打开
            //这里需要判断，如果有文件，且文件大小没达到切割的条件则不要新创文件
            this->_plog->Open(this->_logpatch.c_str(), O_APPEND);
        }*/
        if(IsAccord()){
            //先保存旧的目录
            string validpath = this->_logpatch;
            //保留日志前缀，修改后缀(nyr-sfm)
            String path;
            validpath = path.StringMateMoreFront(validpath.c_str(), "mount-service", false);
            Date date;
            string nowtime = date.GetTime("-%Y%m%d-%H%M%S.log");
            validpath.append(nowtime);
            this->_logpatch = validpath;
            
            //日志类中的文件类，是指向日志文件，但没有读取日志内容，故当心建文件时，主需要关闭文件描述符即可
            //当再打开新的文件时，大小空间跟上一文件类一样，一直是0，和\0
            close(this->_plog->Handle());
            this->_plog->Open(this->_logpatch.c_str(), O_RDWR); // 新建
            
            // TODO 这里只是新建，并不是文件截断
            // 需要增加文件截断的处理逻辑，后续优化成扫描指定下指定一类文件，
            // 将其同一生成新的文件，每个文件要求超过指定字节的，需要截断生成新文件
            // (为了方便处理，新生成的文件名，统一依次取最新日期命名，这会导致文件日期与日志内容的日期时间可能不一致，
            // 这需要后期优化，方案是获取文件内容的日期和时间，重新命名文件)
        }else{
            this->_plog->Open(this->_logpatch.c_str(), O_APPEND); // 追加
        }
        char buf[2048] = {0};
        char *pbuf = buf;
        Date date;
        #if 0
        sprintf(pbuf, "%s %s %s %s",
            ( company == KDS ? "KDS" : company == TC ? "TC" : "OTHER" ),
            ( type == ERROR ? "ERR" : type == DEBUG ? "DBG" : type == INFO ? "RUN" : "---" ),
            _logsource._author.c_str(),
            date.GetTime("%Y-%m-%d %H:%M:%S").c_str()
            );
        #else
        sprintf(pbuf, "%s %s %s %s",
            ( company == KDS ? "KDS" : company == TC ? "TC" : "OTHER" ),
            ( type == ERROR ? "ERR" : type == DEBUG ? "DBG" : type == INFO ? "RUN" : "---" ),
            _logsource._author.c_str(),
            date.GetTime().c_str()
            );
        #endif


        pbuf += strlen(buf);
        try{
            va_list ap;
            va_start(ap, fmt);
            vsnprintf(pbuf, sizeof(buf) - strlen(buf), fmt, ap);
            va_end(ap);
            _plog->Write(buf);
        }catch(...){
            cout<<"exception in construct Log_Msg message"<<endl;
        }
    }
    
    ~Log(){
        delete _plog;
    }
};

#define DETAIL(company, level, fmt, args...) glog.Log_Msg(company, level, \
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

# ifndef LOG_MODULE
# define LOG_MODULE "LOG "
# endif

}
#endif
