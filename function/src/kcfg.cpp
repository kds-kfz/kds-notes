#include"kcfg.hpp"
#include"klog.hpp"
//TODO 目前最多只支持三层key查找，多了就不行了，后续优化考虑采取不定参数作为形参，或是采用字段分隔
bool cJsonInfo::InsertKeyValue(cJSON *item, Values &mapkv, string preName, string tailName, string Name, int flag){
    cJSON* obj = cJSON_GetObjectItem(item, preName.c_str());
    if(obj && obj->type == cJSON_String){
        if(tailName == ""){
            mapkv.insert(make_pair(preName, obj->valuestring));
        }else{
            string newKey;
            if(Name == ""){
                newKey = tailName + "/" + preName;
            }else{
                newKey = Name + "/" + tailName + "/" + preName;
            }
            mapkv.insert(make_pair(newKey, obj->valuestring));
        }
        return true;
    }else if(obj && obj->type == cJSON_Object){
        if(!flag && tailName != ""){
            InsertKeyValue(obj, mapkv, tailName, preName, Name, 1);
        }
        if(flag == 1 && Name != ""){
            InsertKeyValue(obj, mapkv, Name, preName, tailName);
        }
    }
    return false;
}

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
        delete b;
        return false;
    }

    Values readkv;
    readkv.clear();
    InsertKeyValue(item, readkv, "log_path", "", "");
    InsertKeyValue(item, readkv, "log_level", "", "");
    InsertKeyValue(item, readkv, "gearman", "server_info", "server_1");
    InsertKeyValue(item, readkv, "gearman", "client_timeout", "");
    InsertKeyValue(item, readkv, "gearman", "connectMode", "");
    InsertKeyValue(item, readkv, "enable_redis", "", "");
    InsertKeyValue(item, readkv, "redis", "ip", "");
    InsertKeyValue(item, readkv, "redis", "port", "");
    InsertKeyValue(item, readkv, "redis", "expire_time", "");
    InsertKeyValue(item, readkv, "redis", "zqxx_expire_time", "");
    InsertKeyValue(item, readkv, "redis", "login_expire_time", "");
    InsertKeyValue(item, readkv, "redis", "enable_hq_redis", "");
    InsertKeyValue(item, readkv, "redis", "hq_redis_cfg_path", "");
    InsertKeyValue(item, readkv, "runningMod", "", "");
    g_cfg.insert(make_pair(ctype, readkv));
    
    delete b;
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

static Json::Value gCfg;

bool JsonInfo::LoadConfig(string fileName, CfgType ctype){
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
    cout<<b->file.Data()<<endl;

    Json::Reader reader;
    if(!reader.parse(string(b->file.Data()), gCfg)){
        ERROR_TLOG("配置文件 %s, json 数据格式错误，请耐心检查!\n", fileName.c_str());
        delete b;
        return false;
    }

    delete b;
}

bool JsonInfo::GetCfgValue(string key, string &value, CfgType ctype){

}

void JsonInfo::ShowCfgValue(CfgType ctype){

}


