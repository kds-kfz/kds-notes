#include "kpubfun.hpp"
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
        //String path;
        validpath = KPUBFUN::StringMateMoreFront(validpath.c_str(), "mount-service", false);
        //Date date;
        string nowtime = KPUBFUN::Date::GetTime("-%Y%m%d-%H%M%S.log");
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
    //Date date;
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
        KPUBFUN::Date::GetTime().c_str()
        );
    #endif

    pbuf += strlen(buf);
    try{
        // 等待资源
        KIPC::sem_p(); 
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(pbuf, sizeof(buf) - strlen(buf), fmt, ap);
        va_end(ap);
        _plog->Write(buf);
        // 释放资源
        KIPC::sem_v();
    }catch(...){
        ERROR_TLOG("exception in construct Log_Msg message\n");
    }
}

Log::Log(string path, LOG_TYPE type, COMPANYS company){
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
    //DirFile dirfile;
    vector<string> files; //符合条件的文件
    KPUBFUN::DirFile::readFileList(basepath.c_str(), ".log", files, headpath.c_str());
    if(!files.empty()){
        vector< map< string, map< long, long> > > dirpath;
        for(vector<string>::size_type i = 0; i < files.size(); i++){
            //文件全路径 files[i]
            //考虑到有可能文件很多且大，不宜使用文件操作中，文件大小的方式来判断
            //目前判断日期和时间
            //String path;
            vector <string> destpath;
            KPUBFUN::stringSplit(files[i], "-", destpath);
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
        for(vector< map< string, map< long, long> > >::size_type i = 1; i < dirpath.size(); i++){
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
            //Date date;
            string nowtime = KPUBFUN::Date::GetTime("-%Y%m%d-%H%M%S.log");
            path.append(nowtime);
            validpath = path;
            this->_plog->Open(validpath.c_str(), O_RDWR); // 新建
        }
    }else{
        //Date date;
        string nowtime = KPUBFUN::Date::GetTime("-%Y%m%d-%H%M%S.log");
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
