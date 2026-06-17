#pragma once

#include <Ice/Ice.h>

#include <map>
#include <string>

// XML 配置读取结果，保留 Ice 属性和旧 ICEPUSH 节的键值。
struct ST_XML_CONFIG_DATA
{
	// 是否成功按 XML 格式加载，失败时继续走旧 properties/INI 兼容路径。
	bool bXmlLoaded;
	// Ice::Properties 需要的 name/value 集合。
	std::map<std::string, std::string> mapIceProperties;
	// 旧 ICEPUSH 节的配置项，XML 中保持同名，便于从 INI 平滑迁移。
	std::map<std::string, std::string> mapPushProperties;
	// 解析失败原因，用于日志诊断，不影响旧配置兼容。
	std::string strError;

	ST_XML_CONFIG_DATA();
};

// 判断配置文件是否按 XML 方式处理。
bool IsXmlConfigFile(const char* p_szCfgFile);
// 读取 XML 配置文件，失败时返回 false 并填充错误信息。
bool LoadXmlConfigFile(const char* p_szCfgFile, ST_XML_CONFIG_DATA& p_refConfig);
// 根据 XML 或旧 properties 文件初始化 Ice 属性。
bool LoadIcePropertiesFromConfig(const char* p_szCfgFile, Ice::PropertiesPtr p_refProperties, ST_XML_CONFIG_DATA* p_pConfig);
// 从 XML 的 ICEPUSH 节或旧 INI 节读取字符串配置。
std::string GetConfigString(const ST_XML_CONFIG_DATA* p_pConfig, const char* p_szCfgFile, const char* p_szSection, const char* p_szKey, const char* p_szDefault);
// 从 XML 的 ICEPUSH 节或旧 INI 节读取整型配置，并保证非法值回退默认值。
int GetConfigInt(const ST_XML_CONFIG_DATA* p_pConfig, const char* p_szCfgFile, const char* p_szSection, const char* p_szKey, int p_iDefault);
