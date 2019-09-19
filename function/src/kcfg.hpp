#ifndef __KCFG_H__
#define __KCFG_H__

#include "kfile.hpp"
#include<string>
#include<map>

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

enum CfgType{ TCSTY = 1, KDSSTY, OTHERS, };              //配置类型
typedef std::map < std::string, std::string > Values;      //键值对
typedef std::map < CfgType, Values > ValueDesc;  //描述 + 键值对

static ValueDesc g_cfg;

//设计出发点，考虑有可能有多中配置文件，故统一设计成字符型键值对

//json 配置文件基类
class JsBase{
private:
protected:
    ValueDesc CfgValue;     //所有的配置信息键值对
public:
    Files file;             //文件读写类
    JsBase(){}
    ~JsBase(){}
    //JsBase(const JsBase &p) = delete;       //禁止拷贝构造
    JsBase &operator=(const JsBase &p) = delete;    //禁止赋值拷贝
    virtual bool LoadConfig(std::string fileName, CfgType ctype = TCSTY)=0;  //纯虚函数，读配置到容器
    virtual bool GetCfgValue(std::string key, std::string &value, CfgType ctype = TCSTY)=0;      //纯虚函数，读容器中配置
    //virtual std::string GetCfgValue(std::string pkey, std::string tkey)=0;    //纯虚函数，读容器中配置
    virtual void ShowCfgValue(CfgType ctype = TCSTY)=0;    //显示某配置文件配置
};

//初始化申请空间
//JsBase *JsBase::instance=NULL;

//json 配置文件解析，由于有两种方式解析，考虑使用继承
//cjson 派生类，配置文件中不能含有注释
class cJsonInfo : public JsBase{
public:
    cJsonInfo(){}
    ~cJsonInfo(){}
    //JsonInfo(const JsonInfo &p) = delete;   //禁止拷贝构造
    bool LoadConfig(std::string fileName, CfgType ctype = TCSTY);       //纯虚函数，读配置到容器
    bool GetCfgValue(std::string key, std::string &value, CfgType ctype = TCSTY);           //纯虚函数，读容器中配置
    //std::string GetCfgValue(std::string pkey, std::string tkey);    //纯虚函数，读容器中配置
    void ShowCfgValue(CfgType ctype = TCSTY);         //显示某配置文件配置
};

//jsoncpp 派生类
class JsonInfo : public JsBase{
public:
    JsonInfo(){}
    ~JsonInfo(){}
    //JsonInfo(const JsonInfo &p) = delete;   //禁止拷贝构造
    bool LoadConfig(std::string fileName, CfgType ctype = TCSTY);       //纯虚函数，读配置到容器
    bool GetCfgValue(std::string key, std::string &value, CfgType ctype = TCSTY);           //纯虚函数，读容器中配置
    //std::string GetCfgValue(std::string pkey, std::string tkey);    //纯虚函数，读容器中配置
    void ShowCfgValue(CfgType ctype = TCSTY);         //显示某配置文件配置
};

//xml 配置文件解析
//派生类
class DocInfo : public JsBase{
public:
    DocInfo(){}
    ~DocInfo(){}
    //DocInfo(const DocInfo &p) = delete;     //禁止拷贝构造
    bool LoadConfig(std::string fileName, CfgType ctype = TCSTY);       //纯虚函数，读配置到容器
    bool GetCfgValue(std::string key, std::string &value, CfgType ctype = TCSTY);           //纯虚函数，读容器中配置
    //std::string GetCfgValue(std::string pkey, std::string tkey);    //纯虚函数，读容器中配置
    void ShowCfgValue(CfgType ctype = OTHERS);         //显示某配置文件配置
};

//节点 配置文件解析
class NodeInfo{

};

#endif
