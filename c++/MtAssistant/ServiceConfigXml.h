#pragma once

#include "Struct.h"

#include <map>
#include <string>

using namespace std;

struct ServiceConfigXmlData
{
	string strLogLevel;
	std::map<int, ServiceInfo> mapServiceInfo;

	ServiceConfigXmlData() : strLogLevel("info")
	{
	}
};

bool LoadServiceConfigXml(const string& p_refCfgPath, ServiceConfigXmlData& p_refData);
bool SaveServiceConfigXml(const string& p_refCfgPath, const ServiceConfigXmlData& p_refData);
bool NormalizeFullProgramPath(const string& p_refPath, string& p_refFullPath);
bool IsAbsoluteProgramPath(const string& p_refPath);
string GetProgramFileName(const string& p_refPath);
const char* WeekToXmlName(WeekInfo p_enWeek);
bool XmlNameToWeek(const char* p_szName, WeekInfo& p_refWeek);
