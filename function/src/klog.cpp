#include"klog.hpp"
#include"kipc.hpp"

void Log::Log_Msg(COMPANYS company, LOG_TYPE type, const char *fmt, ... ){
    //这句话很关键，如果锁调用的日志函数的日志别大于设置的级别则不打印
    //要想把日志全部输出则将日志级别设置最高，即 TEST
    if(type > _gLogLevel){
        return;   
    }

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
        ( type == ERROR ? "ERR" : type == WARN ? "WARN" : type == INFO ? "RUN" : type == DEBUG ? "DEBUG" : type == TEST ? "TEST" : "---"),
        _logsource._author.c_str(),
        date.GetTime("%Y-%m-%d %H:%M:%S").c_str()
        );
    #else
    sprintf(pbuf, "%s %s %s %s",
        ( company == KDS ? "KDS" : company == TC ? "TC" : "OTHER" ),
        ( type == ERROR ? "ERR" : type == WARN ? "WARN" : type == INFO ? "RUN" : type == DEBUG ? "DEBUG" : type == TEST ? "TEST" : "---"),
        _logsource._author.c_str(),
        date.GetTime().c_str()
        );
    #endif

    pbuf += strlen(buf);
    try{
        // 等待资源
        sem_p(); 
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(pbuf, sizeof(buf) - strlen(buf), fmt, ap);
        va_end(ap);
        _plog->Write(buf);
        // 释放资源
        sem_v();
    }catch(...){
        cout<<"exception in construct Log_Msg message"<<endl;
    }
}
