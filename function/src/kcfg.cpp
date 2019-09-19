#include"kcfg.hpp"
#include"klog.hpp"
#include "../common/cJSON.h"

bool cJsonInfo::LoadConfig(string fileName, CfgType ctype){
    if(fileName.empty() || fileName == ""){
        ERROR_TLOG("配置文件路径不存在!\n");
        return false;
    }
    JsBase *b = NULL;
    b = new cJsonInfo();
    if(!b){
        ERROR_TLOG("配置文件基类空间申请失败，请稍后尝试!\n");
        return false;
    }
    //读文件
    b->file.Open(fileName.c_str(), O_RDONLY);
    cJSON* item = cJSON_Parse(b->file.Data());
    if(!item){
        ERROR_TLOG("配置文件 %s, json 数据格式错误，请耐心检查!\n", fileName.c_str());
        return false;
    }

    Values readkv;
    readkv.clear();
    cJSON* obj = cJSON_GetObjectItem(item, "log_path");
    if(obj && obj->type == cJSON_String){
        readkv.insert(make_pair("log_path", obj->valuestring));
    }
    obj = cJSON_GetObjectItem(item, "log_level");
    if(obj && obj->type == cJSON_String){
        readkv.insert(make_pair("log_level", obj->valuestring));
    }
    obj = cJSON_GetObjectItem(item, "gearman");
    if(obj && obj->type == cJSON_Object){
        cJSON* obj1 = cJSON_GetObjectItem(obj, "client_timeout");
        if(obj1 && obj1->type == cJSON_String){
            readkv.insert(make_pair("gearman/client_timeout", obj1->valuestring));
        }
        obj1 = cJSON_GetObjectItem(obj, "connectMode");
        if(obj1 && obj1->type == cJSON_String){
            readkv.insert(make_pair("gearman/connectMode", obj1->valuestring));
        }
        obj1 = cJSON_GetObjectItem(obj, "server_info");
        if(obj1 && obj1->type == cJSON_Object){
            cJSON* obj2 = cJSON_GetObjectItem(obj1, "server_1");
            if(obj2 && obj2->type == cJSON_String){
                readkv.insert(make_pair("gearman/server_info/server_1", obj2->valuestring));
            }
        }
    }
    obj = cJSON_GetObjectItem(item, "enable_redis");
    if(obj && obj->type == cJSON_String){
        readkv.insert(make_pair("enable_redis", obj->valuestring));
    }
    obj = cJSON_GetObjectItem(item, "redis");
    if(obj && obj->type == cJSON_Object){
        cJSON* obj1 = cJSON_GetObjectItem(obj, "ip");
        if(obj1 && obj1->type == cJSON_String){
            readkv.insert(make_pair("redis/ip", obj1->valuestring));
        }
        obj1 = cJSON_GetObjectItem(obj, "port");
        if(obj1 && obj1->type == cJSON_String){
            readkv.insert(make_pair("redis/port", obj1->valuestring));
        }
        obj1 = cJSON_GetObjectItem(obj, "expire_time");
        if(obj1 && obj1->type == cJSON_String){
            readkv.insert(make_pair("redis/expire_time", obj1->valuestring));
        }
        obj1 = cJSON_GetObjectItem(obj, "zqxx_expire_time");
        if(obj1 && obj1->type == cJSON_String){
            readkv.insert(make_pair("redis/zqxx_expire_time", obj1->valuestring));
        }
        obj1 = cJSON_GetObjectItem(obj, "login_expire_time");
        if(obj1 && obj1->type == cJSON_String){
            readkv.insert(make_pair("redis/login_expire_time", obj1->valuestring));
        }
        obj1 = cJSON_GetObjectItem(obj, "enable_hq_redis");
        if(obj1 && obj1->type == cJSON_String){
            readkv.insert(make_pair("redis/enable_hq_redis", obj1->valuestring));
        }
        obj1 = cJSON_GetObjectItem(obj, "hq_redis_cfg_path");
        if(obj1 && obj1->type == cJSON_String){
            readkv.insert(make_pair("redis/hq_redis_cfg_path", obj1->valuestring));
        }
    }
    obj = cJSON_GetObjectItem(item, "runningMod");
    if(obj && obj->type == cJSON_String){
        readkv.insert(make_pair("runningMod", obj->valuestring));
    }

    g_cfg.insert(make_pair(ctype, readkv));
    
    cJSON_Delete(item);
    return true;
}
bool cJsonInfo::GetCfgValue(string key, string &value, CfgType ctype){
    if (g_cfg.empty()){
        ERROR_TLOG("为读取到有效的配置文件, 请耐心检查!\n");
        return false;
    }
    ValueDesc::iterator it = g_cfg.find(ctype);
    if(it == g_cfg.end()){
        ERROR_TLOG("尚未读取[%d]配置文件配置!\n", ctype);
        return false;
    }
#if 0
    //完全匹配
    Values::iterator its = it->second.find(key);
    if(its == it->second.end()){
        ERROR_TLOG("配置文件[%d]未曾配置[%s]该属性, 请耐心检查!\n", ctype, key.c_str());
        return false;
    }
#else
    //模糊匹配
    Values::iterator its = it->second.begin();
    for(; its != it->second.end(); its++){
        //获取分隔符下最后一个字符串
        string::size_type pos = its->first.rfind("/");
        string tmp = its->first.substr(pos+1);
        //TODO 如果是多层对象下配置属性相同，则不兼容，此时需使用完全匹配
        if(tmp == key){
            value = its->second;
            return true;
        }else{
            continue;
        }
    }
#endif
    return false;
}
void cJsonInfo::ShowCfgValue(CfgType ctype){
    ValueDesc::iterator it = g_cfg.begin();
    for(; it != g_cfg.end(); it++){
        INFO_TLOG("输出配置文件[%d]的配置信息:\n", it->first);
        Values::iterator its = it->second.begin();
        for(; its != it->second.end(); its++){
            INFO_TLOG("[%s] => [%s]\n", its->first.c_str(), its->second.c_str());
        }
    }
}
