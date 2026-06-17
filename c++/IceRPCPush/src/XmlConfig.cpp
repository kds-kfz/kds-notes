#include "publicfunc.h"
#include "XmlConfig.h"

#include "tinyxml.h"

#include <cctype>
#include <cstdlib>

ST_XML_CONFIG_DATA::ST_XML_CONFIG_DATA()
{
	bXmlLoaded = false;
}

// 去除配置文本首尾空白，避免 XML 中换行缩进影响端口/IP 判断。
static std::string TrimConfigValue(const char* p_szValue)
{
	if (p_szValue == nullptr)
	{
		return "";
	}

	const unsigned char* pBegin = reinterpret_cast<const unsigned char*>(p_szValue);
	while (*pBegin != '\0' && std::isspace(*pBegin))
	{
		++pBegin;
	}

	const unsigned char* pEnd = pBegin + strlen(reinterpret_cast<const char*>(pBegin));
	while (pEnd > pBegin && std::isspace(*(pEnd - 1)))
	{
		--pEnd;
	}

	return std::string(reinterpret_cast<const char*>(pBegin), reinterpret_cast<const char*>(pEnd));
}

// 判断元素名是否一致，XML 节点缺失时统一返回 false。
static bool IsElementName(const TiXmlElement* p_pElement, const char* p_szName)
{
	return p_pElement != nullptr && p_szName != nullptr && _stricmp(p_pElement->Value(), p_szName) == 0;
}

// 支持 <Property name="" value=""/> 和 <Key>Value</Key> 两种表达方式。
static void LoadElementProperties(const TiXmlElement* p_pParent, std::map<std::string, std::string>& p_refProperties)
{
	if (p_pParent == nullptr)
	{
		return;
	}

	for (const TiXmlElement* pElement = p_pParent->FirstChildElement(); pElement != nullptr; pElement = pElement->NextSiblingElement())
	{
		if (IsElementName(pElement, "Property"))
		{
			const char* pName = pElement->Attribute("name");
			const char* pValue = pElement->Attribute("value");
			if (pName == nullptr || pName[0] == '\0')
			{
				continue;
			}
			if (pValue == nullptr)
			{
				pValue = pElement->GetText();
			}
			p_refProperties[pName] = TrimConfigValue(pValue);
			continue;
		}

		p_refProperties[pElement->Value()] = TrimConfigValue(pElement->GetText());
	}
}

// 根节点允许直接是 IceRPCPush，也允许外层包含 IceRPCPush 节。
static const TiXmlElement* GetConfigRoot(const TiXmlDocument& p_refDoc)
{
	const TiXmlElement* pRoot = p_refDoc.RootElement();
	if (pRoot == nullptr)
	{
		return nullptr;
	}
	if (IsElementName(pRoot, "IceRPCPush"))
	{
		return pRoot;
	}
	const TiXmlElement* pChild = pRoot->FirstChildElement("IceRPCPush");
	return pChild != nullptr ? pChild : pRoot;
}

bool IsXmlConfigFile(const char* p_szCfgFile)
{
	if (p_szCfgFile == nullptr || p_szCfgFile[0] == '\0')
	{
		return false;
	}

	const char* pExt = strrchr(p_szCfgFile, '.');
	return pExt != nullptr && _stricmp(pExt, ".xml") == 0;
}

bool LoadXmlConfigFile(const char* p_szCfgFile, ST_XML_CONFIG_DATA& p_refConfig)
{
	p_refConfig = ST_XML_CONFIG_DATA();
	if (!IsXmlConfigFile(p_szCfgFile))
	{
		p_refConfig.strError = "配置文件扩展名不是 xml";
		return false;
	}

	TiXmlDocument clDoc;
	if (!clDoc.LoadFile(p_szCfgFile, TIXML_ENCODING_UTF8))
	{
		p_refConfig.strError = clDoc.ErrorDesc();
		return false;
	}

	const TiXmlElement* pRoot = GetConfigRoot(clDoc);
	if (pRoot == nullptr)
	{
		p_refConfig.strError = "XML 缺少根节点";
		return false;
	}

	LoadElementProperties(pRoot->FirstChildElement("Ice"), p_refConfig.mapIceProperties);
	LoadElementProperties(pRoot->FirstChildElement("ICEPUSH"), p_refConfig.mapPushProperties);

	// 兼容把 Ice 属性直接写在根节点下的简单 XML。
	if (p_refConfig.mapIceProperties.empty())
	{
		LoadElementProperties(pRoot->FirstChildElement("Properties"), p_refConfig.mapIceProperties);
	}

	p_refConfig.bXmlLoaded = true;
	return true;
}

bool LoadIcePropertiesFromConfig(const char* p_szCfgFile, Ice::PropertiesPtr p_refProperties, ST_XML_CONFIG_DATA* p_pConfig)
{
	if (p_refProperties == nullptr)
	{
		return false;
	}

	if (p_pConfig != nullptr && LoadXmlConfigFile(p_szCfgFile, *p_pConfig))
	{
		for (std::map<std::string, std::string>::const_iterator it = p_pConfig->mapIceProperties.begin(); it != p_pConfig->mapIceProperties.end(); ++it)
		{
			p_refProperties->setProperty(it->first, it->second);
		}
		return true;
	}

	p_refProperties->load(p_szCfgFile);
	return true;
}

std::string GetConfigString(const ST_XML_CONFIG_DATA* p_pConfig, const char* p_szCfgFile, const char* p_szSection, const char* p_szKey, const char* p_szDefault)
{
	if (p_szDefault == nullptr)
	{
		p_szDefault = "";
	}

	if (p_pConfig != nullptr && p_pConfig->bXmlLoaded && p_szKey != nullptr)
	{
		const std::map<std::string, std::string>* pMap = &p_pConfig->mapPushProperties;
		if (p_szSection != nullptr && _stricmp(p_szSection, "Ice") == 0)
		{
			pMap = &p_pConfig->mapIceProperties;
		}

		std::map<std::string, std::string>::const_iterator it = pMap->find(p_szKey);
		if (it != pMap->end())
		{
			return it->second;
		}
		return p_szDefault;
	}

	char szBuf[512] = {0};
	GetPrivateProfileStringA(p_szSection, p_szKey, p_szDefault, szBuf, sizeof(szBuf), p_szCfgFile);
	return szBuf;
}

int GetConfigInt(const ST_XML_CONFIG_DATA* p_pConfig, const char* p_szCfgFile, const char* p_szSection, const char* p_szKey, int p_iDefault)
{
	if (p_pConfig != nullptr && p_pConfig->bXmlLoaded)
	{
		std::string strValue = GetConfigString(p_pConfig, p_szCfgFile, p_szSection, p_szKey, "");
		if (strValue.empty())
		{
			return p_iDefault;
		}

		char* pEnd = nullptr;
		long lValue = strtol(strValue.c_str(), &pEnd, 10);
		if (pEnd == strValue.c_str() || *pEnd != '\0')
		{
			return p_iDefault;
		}
		return static_cast<int>(lValue);
	}

	return GetPrivateProfileIntA(p_szSection, p_szKey, p_iDefault, p_szCfgFile);
}
