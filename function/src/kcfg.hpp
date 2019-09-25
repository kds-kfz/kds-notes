#ifndef __KCFG_H__
#define __KCFG_H__

#include "kfile.hpp"
#include<string>
#include<map>
#include "cjson/cJSON.h"
#include"json/json.h"

/******************************* 标 签 *******************************
 * 作者：kfz
 * 日期：2019-09-04
 * 描述：文件简要说明
 * 获取配置文件，有三种配置文件
 * (1) json 格式配置文件，可以用 cJSON 或者 jsoncpp 来解析
 * (2) xml 格式配置文件，可以用 tinyxml 来解析
 * (3) 节点配置文件，纯调用底层库和逻辑处理来解析
 * 本文件就是针对这三种配置文件进行解析方法
 * 设计理念：考虑实现读取的文件格式较多，故才分成头文件和源文件
 ********************************************************************/

//配置文件的读取与内存文件读写不一样，前者只读一次，后者存在多次读写

# ifndef LOG_MODULE
# define LOG_MODULE "KCFG "
# endif

using namespace std;

enum CfgType{ TCSTY = 1, KDSSTY, OTHERS, };     //配置类型
typedef map < string, string > Values;          //键值对
typedef map < CfgType, Values > ValueDesc;      //描述 + 键值对

static ValueDesc g_cfg;                         //所有的配置信息键值对

// 设计出发点，考虑有可能有多中配置文件，故统一设计成字符型键值对
// json 配置文件解析，由于有两种方式解析，考虑使用继承
// cjson 派生类，配置文件中不能含有注释

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ json 配 置 文 件 基 类 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
class JsBase{
private:
protected:
public:
    Files file;                                 //文件读写类
    JsBase(){}
    // 基类里有虚函数, 定义了基类指针指向派生类, 就会需要定义基类虚析构
    // 基类指针析构的时候, 就会先析构派生类,再析构基类
    // 如果不定义虚析构, 就会基类指针直接析构基类, 这样派生类对象销毁不完整
    virtual ~JsBase(){}
    JsBase &operator=(const JsBase &p) = delete;                                    //禁止赋值拷贝
    void ShowCfgValue(CfgType ctype = TCSTY);                                       //显示某配置文件配置
    virtual bool LoadConfig(string fileName, CfgType ctype = TCSTY)=0;              //纯虚函数，读配置到容器
    const string GetCfgValue(const string key, CfgType ctype = TCSTY);                                                  //读容器中配置
    const string GetCfgValue(const string objName, const string key, CfgType ctype = TCSTY);                            //读容器中配置
    const string GetCfgValue(const string objFather, const string objChild, const string key, CfgType ctype = TCSTY);   //读容器中配置
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ cjson 配 置 文 件派 生 类 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
class cJsonInfo : public JsBase{
public:
    cJsonInfo(){}
    ~cJsonInfo(){}
    bool LoadConfig(string fileName, CfgType ctype = TCSTY);                    //纯虚函数，读配置到容器
    bool CheckDataForm(cJSON *item);                                            //转换 json 数据格式
    bool InsertKeyValue(cJSON *item, Values &mapkv, string key);                //读取配置信息到容器
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ jsoncpp 配 置 文 件派 生 类 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
class JsonInfo : public JsBase{
protected:
    Json::Value gCfg;
public:
    JsonInfo(){}
    ~JsonInfo(){}
    bool LoadConfig(string fileName, CfgType ctype = TCSTY);                    //纯虚函数，读配置到容器
    bool CheckDataForm(Json::Value &value);                                     //转换 json 数据格式
    bool InsertKeyValue(Json::Value &value, Values &mapkv, string key);         //读取配置信息到容器
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ xml 配 置 文 件 派 生 类 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
class DocInfo : public JsBase{
public:
    DocInfo(){}
    ~DocInfo(){}
    bool LoadConfig(string fileName, CfgType ctype = TCSTY);                    //读取配置信息到容器
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 节 点 配 置 文 件 派 生 类 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
class NodeInfo : public JsBase{
    NodeInfo(){}
    ~NodeInfo(){}
    bool LoadConfig(string fileName, CfgType ctype = TCSTY);                    //读取配置信息到容器
};

#endif
