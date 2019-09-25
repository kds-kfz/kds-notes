#include"klog.hpp"
#include"kipc.hpp"
#include"ksignal.hpp"
#include"kcfg.hpp"

# ifndef LOG_MODULE
# define LOG_MODULE "MAIN "
# endif

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
#if 0
    //文件测试
    Files fd;
    fd.Open("3.txt", O_RDONLY);
    cout<<"size:"<<fd.Size()<<endl;
    cout<<"date:"<<fd.Date();
    fd.Write("./","4.txt");
    
    fd.Open("5.txt", O_APPEND);
    fd.Write("world!");


#endif
#if 0
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
#else
    #if 1
    //cjson 测试配置文件函数
    cJsonInfo k;
    k.LoadConfig("../etc/config.json");
    k.ShowCfgValue();
    string sztmp = "";
    if(k.GetCfgValue("login_expire_time", sztmp)){
        INFO_TLOG("已读取到配置为:[%s]\n", sztmp.c_str());
    }
    #else
    //cjson 测试配置文件函数
    JsonInfo k;
    k.LoadConfig("../etc/config.json");
    k.ShowCfgValue();
    #endif
    
    INFO_TLOG("挂载系统正在退出，欢迎使用...\n");
    //删除信号量
    KIPC::delsem();
    return 0;
#endif
}
