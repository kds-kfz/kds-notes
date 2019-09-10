#include"kcfg.hpp"
#include"klog.hpp"
#include "../common/cJSON.h"

# ifndef LOG_MODULE
# define LOG_MODULE "KCFG "
# endif

using namespace std;
//using namespace kfz;


bool JsonInfo::LoadConfig(string fileName){
    if(fileName.empty() || fileName == ""){
        ERROR_TLOG("配置文件路径不存在!\n");
        return false;
    }
    JsBase *b = NULL;
    b = new JsonInfo();
    if(!b){
        ERROR_TLOG("配置文件基类空间申请失败，请稍后尝试!\n");
        return false;
    }
    //读文件
    b->file.Open(fileName.c_str(), O_RDONLY);
    INFO_TLOG("已读取配置文件信息:\n%s\n", b->file.Data());
    cJSON* item = cJSON_Parse(b->file.Data());
    if(!item){
        ERROR_TLOG("配置文件 %s, json 数据格式错误，请耐心检查!\n", fileName.c_str());
        return false;
    }
    cJSON* obj = cJSON_GetObjectItem(item, "log_path");
    cJSON_Delete(item);
    return true;
}
