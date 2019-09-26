#include "kcfg.hpp"
#include "klog.hpp"
#include "kpubfun.hpp"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ json 配 置 文 件 基 类 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

const string JsBase::GetCfgValue(const string key, CfgType ctype){
    if (g_cfg.empty()){
        ERROR_TLOG("为读取到有效的配置文件, 请耐心检查!\n");
        return "";
    }
    ValueDesc::iterator it = g_cfg.find(ctype);
    if(it == g_cfg.end()){
        ERROR_TLOG("尚未读取 [%d] 配置文件配置!\n", ctype);
        return "";
    }
#if 1
    //完全匹配
    Values::iterator its = it->second.find(key);
    if(its == it->second.end()){
        ERROR_TLOG("配置文件 [%d] 未曾配置 [%s] 该属性, 请耐心检查!\n", ctype, key.c_str());
        return "";
    }
    return its->second;
#else
    //模糊匹配
    Values::iterator its = it->second.begin();
    for(; its != it->second.end(); its++){
        //获取分隔符下最后一个字符串
        string::size_type pos = its->first.rfind("/");
        string tmp = its->first.substr(pos+1);
        //TODO 如果是多层对象下配置属性相同，则不兼容，此时需使用完全匹配
        if(tmp == key){
            return its->second;
        }else{
            continue;
        }
    }
    return "";
#endif
}
    
const string JsBase::GetCfgValue(const string objName, const string key, CfgType ctype){
    if (g_cfg.empty()){
        ERROR_TLOG("为读取到有效的配置文件, 请耐心检查!\n");
        return "";
    }
    ValueDesc::iterator it = g_cfg.find(ctype);
    if(it == g_cfg.end()){
        ERROR_TLOG("尚未读取 [%d] 配置文件配置!\n", ctype);
        return "";
    }
    //完全匹配
    string newKey = objName + "/" + key;
    Values::iterator its = it->second.find(newKey);
    if(its == it->second.end()){
        ERROR_TLOG("配置文件 [%d] 未曾配置 [%s] [%s]该属性, 请耐心检查!\n",
                ctype, objName.c_str(), key.c_str());
        return "";
    }
    return its->second;
}

const string JsBase::GetCfgValue(const string objFather, const string objChild, const string key, CfgType ctype){
    if (g_cfg.empty()){
        ERROR_TLOG("为读取到有效的配置文件, 请耐心检查!\n");
        return "";
    }
    ValueDesc::iterator it = g_cfg.find(ctype);
    if(it == g_cfg.end()){
        ERROR_TLOG("尚未读取 [%d] 配置文件配置!\n", ctype);
        return "";
    }
    //完全匹配
    string newKey = objFather + "/" + objChild + "/" + key;
    Values::iterator its = it->second.find(newKey);
    if(its == it->second.end()){
        ERROR_TLOG("配置文件 [%d] 未曾配置 [%s] [%s] [%s] 该属性, 请耐心检查!\n",
                ctype, objFather.c_str(), objChild.c_str(), key.c_str());
        return "";
    }
    return its->second;
}

void JsBase::ShowCfgValue(CfgType ctype){
    ValueDesc::iterator it = g_cfg.begin();
    for(; it != g_cfg.end(); it++){
        INFO_TLOG("输出配置文件[%d]的配置信息:\n", it->first);
        Values::iterator its = it->second.begin();
        for(; its != it->second.end(); its++){
            INFO_TLOG("[%s] => [%s]\n", its->first.c_str(), its->second.c_str());
        }
    }
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ cjson 配 置 文 件派 生 类 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#if 0
//TODO 目前最多只支持三层key查找，多了就不行了，后续优化考虑采用字段分隔
//demo: InsertKeyValue(item, readkv, "log_path", "", "");
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
#endif

bool cJsonInfo::CheckDataForm(cJSON *item){
    switch(item->type){
        case cJSON_NULL:
            {
                item->type = cJSON_String;
                char value[5] = {0};
                sprintf(value, "%s", "NULL");
                //item->valuestring 初始化时为 0
                item->valuestring = new char[5]{0};
                memcpy(item->valuestring, value, strlen(value));
                //TEST_TLOG("cJSON_NULL after type = [%d], valuestring = [%s]\n", item->type, item->valuestring);
            }
            break;
        case cJSON_Number:
            {
                //item->valueint=(int)item->valuedouble
                item->type = cJSON_String;
                char value[33] = {0};
                sprintf(value, "%lf", item->valuedouble);
                //item->valuestring 初始化时为 0
                item->valuestring = new char[33]{0};
                memcpy(item->valuestring, value, strlen(value));
                //TEST_TLOG("cJSON_Number after type = [%d], valuestring = [%s]\n", item->type, item->valuestring);
            }
            break;
        case cJSON_String:
            break;
        case cJSON_False:
            {
                item->type = cJSON_String;
                char value[6] = {0};
                sprintf(value, "%s", "false");
                //item->valuestring 初始化时为 0
                item->valuestring = new char[6]{0};
                memcpy(item->valuestring, value, strlen(value));
                //TEST_TLOG("cJSON_False after type = [%d], valuestring = [%s]\n", item->type, item->valuestring);
            }
            break;
        case cJSON_True:
            {
                item->type = cJSON_String;
                char value[5] = {0};
                sprintf(value, "%s", "true");
                //item->valuestring 初始化时为 0
                item->valuestring = new char[5]{0};
                memcpy(item->valuestring, value, strlen(value));
                //TEST_TLOG("cJSON_True after type = [%d], valuestring = [%s]\n", item->type, item->valuestring);
            }
            break;
        case cJSON_Array:
            {
                int size = cJSON_GetArraySize(item);
                //TEST_TLOG("数组长度 = [%d]\n", size);
                for(int i = 0; i < size; i++){
                    cJSON *obj = cJSON_GetArrayItem(item, i);
                    if(obj){
                        CheckDataForm(obj);
                    }
                }
            }
            break;
        case cJSON_Object:
            {
                int size = cJSON_GetArraySize(item);
                //TEST_TLOG("对象长度 = [%d]\n", size);
                for(int i = 0; i < size; i++){
                    cJSON *obj = cJSON_GetArrayItem(item, i);
                    if(obj){
                        //TEST_TLOG("before type = [%d]\n", item->type);
                        CheckDataForm(obj);
                    }
                }
            }
            break;
        default:
            return false;
    }
    return true;
}

bool cJsonInfo::InsertKeyValue(cJSON *item, Values &mapkv, string key){
    vector<string> keyname;
    int size = KPUBFUN::stringSplit(key, ":", keyname);
    if(size == 1){
        cJSON *obj = cJSON_GetObjectItem(item, keyname[0].c_str());
        if(obj){
            if(obj->type == cJSON_String){
                mapkv.insert(make_pair(keyname[0], obj->valuestring));
            }else{
                ERROR_TLOG("配置项 [%s] 格式错误!\n", keyname[0].c_str());
                return false;
            }
        }else{
            ERROR_TLOG("该配置项 [%s] 不存在!\n", keyname[0].c_str());
            return false;
        }
    }else if(size == 2){
        cJSON *obj = cJSON_GetObjectItem(item, keyname[0].c_str());
        if(obj){ 
            if(obj->type == cJSON_Object){
                cJSON *objc = cJSON_GetObjectItem(obj, keyname[1].c_str());
                if(objc){
                    if(objc->type == cJSON_String){
                        string newKey = keyname[0] + "/" + keyname[1];
                        mapkv.insert(make_pair(newKey, objc->valuestring));
                    }else{
                        ERROR_TLOG("配置项 [%s] 中 [%s] 格式错误!\n",
                            keyname[0].c_str(), keyname[1].c_str());
                        return false;
                    }
                }else{
                    ERROR_TLOG("配置项 [%s] 中该配置项 [%s] 不存在!\n",
                        keyname[0].c_str(), keyname[1].c_str());
                    return false;
                }
            }else{
                ERROR_TLOG("配置项 [%s] 格式错误!\n", keyname[0].c_str());
                return false;
            }
        }else{
            ERROR_TLOG("该配置项 [%s] 不存在!\n", keyname[0].c_str());
            return false;
        }
    }else if(size == 3){
        cJSON *obj = cJSON_GetObjectItem(item, keyname[0].c_str());
        if(obj){ 
            if(obj->type == cJSON_Object){
                cJSON *objf = cJSON_GetObjectItem(obj, keyname[1].c_str());
                if(objf){
                    if(objf->type == cJSON_Object){
                        cJSON *objc = cJSON_GetObjectItem(objf, keyname[2].c_str());
                        if(objc){
                            if(objc->type == cJSON_String){
                                string newKey = keyname[0] + "/" + keyname[1] + "/" + keyname[2];
                                mapkv.insert(make_pair(newKey, objc->valuestring));
                            }else{
                                ERROR_TLOG("配置项 [%s] [%s] 中 [%s] 格式错误!\n",
                                    keyname[0].c_str(), keyname[1].c_str(), keyname[2].c_str());
                                return false;
                            }
                        }else{
                            ERROR_TLOG("配置项 [%s] [%s] 中该配置项 [%s] 不存在!\n",
                                keyname[0].c_str(), keyname[1].c_str(), keyname[2].c_str());
                            return false;
                        }
                    }else{
                        ERROR_TLOG("配置项 [%s] 中 [%s] 格式错误!\n",
                            keyname[0].c_str(), keyname[1].c_str());
                        return false;
                    }
                }else{
                    ERROR_TLOG("配置项 [%s] 中 [%s] 不存在!\n",
                        keyname[0].c_str(), keyname[1].c_str());
                    return false;
                }
            }else{
                ERROR_TLOG("配置项 [%s] 格式错误!\n", keyname[0].c_str());
                return false;
            }
        }else{
            ERROR_TLOG("该配置项 [%s] 不存在!\n", keyname[0].c_str());
            return false;
        }
    }
    return true;
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
    
    //校验 json 后面值的不同的类型处理，非 stirng 则需要转换成 string
    if(!CheckDataForm(item)){
        ERROR_TLOG("配置文件 %s, json 数据类型错误，请耐心检查!\n", fileName.c_str());
        delete b;
        return false;
    }

    //读取配置信息到容器
    Values readkv;
    readkv.clear();
    InsertKeyValue(item, readkv, "log_path");
    InsertKeyValue(item, readkv, "log_level");
    InsertKeyValue(item, readkv, "gearman:server_info:server_1");
    InsertKeyValue(item, readkv, "gearman:client_timeout");
    InsertKeyValue(item, readkv, "gearman:connectMode");
    InsertKeyValue(item, readkv, "enable_redis");
    InsertKeyValue(item, readkv, "redis:ip");
    InsertKeyValue(item, readkv, "redis:port");
    InsertKeyValue(item, readkv, "redis:expire_time");
    InsertKeyValue(item, readkv, "redis:zqxx_expire_time");
    InsertKeyValue(item, readkv, "redis:login_expire_time");
    InsertKeyValue(item, readkv, "redis:enable_hq_redis");
    InsertKeyValue(item, readkv, "redis:hq_redis_cfg_path");
    InsertKeyValue(item, readkv, "runningMod");
    g_cfg.insert(make_pair(ctype, readkv));
    
    delete b;
    cJSON_Delete(item);
    return true;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ jsoncpp 配 置 文 件派 生 类 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

bool JsonInfo::CheckDataForm(Json::Value &value){
    switch(value.type()){
        case Json::nullValue:
            value = "";
            break;
        case Json::intValue:
            value = Json::valueToString(value.asInt());
            break;
        case Json::uintValue:
            value = Json::valueToString(value.asUInt());
            break;
        case Json::realValue:
            value = Json::valueToString(value.asDouble());
        case Json::stringValue:
            break;
        case Json::booleanValue:
            value = Json::valueToString(value.asBool());
        case Json::arrayValue:
            {
                //是个数组，需要进入解析对象，依次获取json后面的值
                Json::ArrayIndex size = value.size();
                for(Json::ArrayIndex index = 0; index < size; ++index){
                    //递归查找该对象的 json 后的值
                    CheckDataForm(value[index]);
                }
            }
            break;
        case Json::objectValue:
            {
                //是个对象，需要进入解析对象，依次获取json后面的值
                Json::Value::Members members(value.getMemberNames());
                for(Json::Value::Members::iterator it = members.begin();
                        it != members.end();++it){
                    const std::string &name = *it;
                    //递归查找该对象的 json 后的值
                    CheckDataForm(value[name]);
                }
            }
            break;
        default:
            return false;
    }
    return true;
}

//TODO 考虑到有多种配置文件数据格式，故统一格式，每种格式都采用分隔符处理
bool JsonInfo::InsertKeyValue(Json::Value &value, Values &mapkv, string key){
    vector<string> keyname;
    string Value = "";
    int size = KPUBFUN::stringSplit(key, ":", keyname);
    if(size == 1){
        if(gCfg[keyname[0].c_str()].isString()){
            string newKey = keyname[0];
            Value = gCfg[keyname[0].c_str()].asString();
            mapkv.insert(make_pair(newKey, Value));
        }else{
            ERROR_TLOG("配置项 [%s] 格式错误", keyname[0].c_str());
            return false;
        }
    }else if(size == 2){
        Json::Value obj =  gCfg[keyname[0]];
        if(obj.isNull()){
            ERROR_TLOG("配置项 [%s] 不存在", keyname[0].c_str());
            return false;
        }else if(not obj.isObject()){
            ERROR_TLOG("配置项 [%s] 格式错误", keyname[0].c_str());
            return false;
        }
        if(obj[keyname[1].c_str()].isString()){
            string newKey = keyname[0] + "/" + keyname[1];
            Value = obj[keyname[1].c_str()].asString();
            mapkv.insert(make_pair(newKey, Value));
        }else{
            ERROR_TLOG("配置项 [%s] 中 [%s] 格式错误",
                    keyname[0].c_str(), keyname[1].c_str());
            return false;
        }
    }else if(size == 3){
        Json::Value obj =  gCfg[keyname[0]];
        if(not obj.isObject()){
            ERROR_TLOG("配置项 [%s] 格式错误", keyname[0].c_str());
            return false;
        }
        Json::Value objC = obj[keyname[1]];
        if(not objC.isObject()){
            ERROR_TLOG("配置项 [%s] 中 [%s] 格式错误",
                    keyname[0].c_str(), keyname[1].c_str());
            return false;
        }
        if(objC[keyname[2].c_str()].isString()){
            string newKey = keyname[0] + "/" + keyname[1] + "/" + keyname[2];
            Value = objC[keyname[2].c_str()].asString();
            mapkv.insert(make_pair(newKey, Value));
        }else{
            ERROR_TLOG("配置项 [%s] [%s] 中 [%s] 格式错误",
                    keyname[0].c_str(), keyname[1].c_str(), keyname[2].c_str());
            return false;
        }
    }

    return true;
}

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
    string content = b->file.Data();
    
    Json::Reader reader;
    if(!reader.parse(content, gCfg)){
        ERROR_TLOG("配置文件 %s, json 数据格式错误，请耐心检查!\n", fileName.c_str());
        delete b;
        return false;
    }
    
    //校验 json 后面值的不同的类型处理，非 stirng 则需要转换成 string
    if(!CheckDataForm(gCfg)){
        ERROR_TLOG("配置文件 %s, json 数据类型错误，请耐心检查!\n", fileName.c_str());
        delete b;
        return false;
    }

    //读取配置信息到容器
    Values readkv;
    readkv.clear();
    InsertKeyValue(gCfg, readkv, "log_path");
    InsertKeyValue(gCfg, readkv, "log_level");
    InsertKeyValue(gCfg, readkv, "gearman:server_info:server_1");
    InsertKeyValue(gCfg, readkv, "gearman:client_timeout");
    InsertKeyValue(gCfg, readkv, "gearman:connectMode");
    InsertKeyValue(gCfg, readkv, "enable_redis");
    InsertKeyValue(gCfg, readkv, "redis:ip");
    InsertKeyValue(gCfg, readkv, "redis:port");
    InsertKeyValue(gCfg, readkv, "redis:expire_time");
    InsertKeyValue(gCfg, readkv, "redis:zqxx_expire_time");
    InsertKeyValue(gCfg, readkv, "redis:login_expire_time");
    InsertKeyValue(gCfg, readkv, "redis:enable_hq_redis");
    InsertKeyValue(gCfg, readkv, "redis:hq_redis_cfg_path");
    InsertKeyValue(gCfg, readkv, "runningMod");
    g_cfg.insert(make_pair(ctype, readkv));
    
    delete b;
    return true;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ xml 配 置 文 件 派 生 类 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

// 递归查找子元素节点信息, 读取配置文件，以形参形式插入键值对
// 由于处理较为特别，很难兼容前两种容器形式( key 值舍去拼接 )
// TODO 待后续优化统一
bool DocInfo::InsertKeyValue(TiXmlElement *root, Values &mapkv){
    TiXmlElement *ele_results = root->FirstChildElement();
    TiXmlNode *pNode = root->FirstChildElement();
    TiXmlText *pText = NULL;
    while(ele_results){
        int t = pNode->Type();
        switch(t){
            case TiXmlText::TINYXML_DOCUMENT:
                break;
            //节点类型是Element
            case TiXmlText::TINYXML_ELEMENT:
                {
                    if(ele_results->GetText()){
                        mapkv.insert(make_pair(ele_results->Value(), ele_results->GetText()));
                    }else{
                        InsertKeyValue(ele_results, mapkv);
                    }
                }
                break;
            case TiXmlText::TINYXML_COMMENT:
                break;
            case TiXmlText::TINYXML_UNKNOWN:
                break;
            // 节点类型是 text 节点
            case TiXmlText::TINYXML_TEXT:
                {
                    pText = pNode->ToText();
                    string text = pText->Value();
                    cout<<"text = "<<text<<endl;
                }
                break;
            case TiXmlText::TINYXML_DECLARATION:
                break;
            case TiXmlText::TINYXML_TYPECOUNT:
                break;
            default:
                break;
        }
        pNode = pNode->NextSiblingElement();
        ele_results = ele_results->NextSiblingElement();
    }
    return true;
}

bool DocInfo::LoadConfig(string fileName, CfgType ctype){
    if(fileName.empty() || fileName == ""){
        ERROR_TLOG("配置文件路径不存在!\n");
        return false;
    }

    TiXmlDocument docXml;
    
    // 读文件
    if(!docXml.LoadFile(fileName.c_str())){
        const char *errorStr = docXml.ErrorDesc();
        ERROR_TLOG("加载 [%s] 文件失败, errmsg [%s]\n", fileName.c_str(), errorStr);
        return false;
    }
    //docXml.Print();

    // 获取 xml 的根节点
    TiXmlElement *root = docXml.RootElement();
    // Value() 是个 const char *, 根节点名称 params
    string ElementName = root->Value();
    cout<<"root = "<<ElementName<<endl;
    
    //读取配置信息到容器
    Values readkv;
    InsertKeyValue(root, readkv);
    
    g_cfg.insert(make_pair(ctype, readkv));

    return true;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 节 点 配 置 文 件 派 生 类 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

