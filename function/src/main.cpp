#include"klog.hpp"
#include"kipc.hpp"
#include"ksignal.hpp"
#include"kcfg.hpp"

# ifndef LOG_MODULE
# define LOG_MODULE "MAIN "
# endif

#define TEST_MODULE -1
#define TEST_PARSE_MODULE 3

//开发测试阶段
LOG_TYPE _gLogLevel = TEST;
Log *glog;

int main(int argc, char *argv[]){
    //读配置文件测试
    glog = new Log("../log/mount-service");
    INFO_TLOG("挂载系统初始化成功...\n");
    //创建信号量集
    if(!KIPC::creatSem(IPC_MODE)){
        exit(1);
    }
    //初始化信号量
    if(!KIPC::initSem(1)){
        exit(1);
    }
    //信号注册
    if(!initSigProc()){
        exit(1);
    }
#if TEST_MODULE == 0
    //文件测试
    Files fd;
    fd.Open("3.txt", O_RDONLY);
    cout<<"size:"<<fd.Size()<<endl;
    cout<<"date:"<<fd.Data();
    fd.Write("./","4.txt");
    
    fd.Open("5.txt", O_APPEND);
    fd.Write("world!");

#elif TEST_MODULE == 1
    //字符串测试
    String str;
    char str_1[128] = {0};
    char str_2[128] = "hello world pp";
    cout<<"转换结果:"<<str.StringMateMoreLast(str_2, " ")<<endl;
    
    //Date time1;
    //cout<<time1.GetTime("%Y%m%d-%H%M%S.log")<<endl;

    //初始化，后续得放到对象池中，同一初始化
    //Log glog("../log/mount-service");
    glog = new Log("../log/mount-service");
    long sec = 0;
    while(1){
        sleep(1);
        INFO_KLOG("测试[%ld]\n", sec++);
        INFO_TLOG("测试[%ld]\n", sec++);
        WARN_TLOG("测试[%ld]\n", sec++);
    }
#endif
    
#if TEST_PARSE_MODULE == 0
    //cjson 测试配置文件函数
    cJsonInfo k;
    k.LoadConfig("../etc/config.json");

#elif TEST_PARSE_MODULE == 1
    //cjson 测试配置文件函数
    JsonInfo k;
    k.LoadConfig("../etc/config.json");

#elif TEST_PARSE_MODULE == 2
    //xml 测试配置文件
    DocInfo k;
    k.LoadConfig("../etc/config.xml");
#elif TEST_PARSE_MODULE == 3
    //xml 测试配置文件
    NodeInfo k;
    k.LoadConfig("../etc/config.ini");
    
#endif
    k.ShowCfgValue();
    /*
    string sztmp = "";
    sztmp = k.GetCfgValue("log_path");
    INFO_TLOG("已读取到配置为:[%s]\n", sztmp.c_str());
    sztmp = k.GetCfgValue("gearman", "server_info", "server_1");
    INFO_TLOG("已读取到配置为:[%s]\n", sztmp.c_str());
    sztmp = k.GetCfgValue("redis", "login_expire_time");
    INFO_TLOG("已读取到配置为:[%s]\n", sztmp.c_str());
    */
    INFO_TLOG("挂载系统正在退出，欢迎使用...\n");
    //删除信号量
    KIPC::delsem();
    return 0;
}
