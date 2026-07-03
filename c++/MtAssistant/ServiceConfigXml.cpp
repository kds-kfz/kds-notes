#include "stdafx.h"
#include "ServiceConfigXml.h"

#include "tinyxml.h"

#include <algorithm>
#include <cctype>
#include <io.h>
#include <vector>

namespace
{
	const int MAX_SERVICE_CFG_COUNT = 256;
	const int MAX_TIME_CFG_COUNT = 128;

	void AddComment(TiXmlNode* p_pParent, const char* p_szValue)
	{
		TiXmlComment* pComment = new TiXmlComment();
		pComment->SetValue(p_szValue);
		p_pParent->LinkEndChild(pComment);
	}

	bool IsLocalPathDelimiter(char p_chValue)
	{
		return p_chValue == '\\' || p_chValue == '/';
	}

	string NormalizeSlash(string p_strPath)
	{
		std::replace(p_strPath.begin(), p_strPath.end(), '/', '\\');
		return p_strPath;
	}

	int ReadIntAttribute(TiXmlElement* p_pElement, const char* p_szName, int p_iDefault)
	{
		if (p_pElement == NULL)
			return p_iDefault;

		int iValue = p_iDefault;
		if (p_pElement->QueryIntAttribute(p_szName, &iValue) != TIXML_SUCCESS)
			return p_iDefault;

		return iValue;
	}

	string ReadStringAttribute(TiXmlElement* p_pElement, const char* p_szName, const char* p_szDefault)
	{
		if (p_pElement == NULL)
			return p_szDefault == NULL ? "" : p_szDefault;

		const char* pValue = p_pElement->Attribute(p_szName);
		return pValue == NULL ? (p_szDefault == NULL ? "" : p_szDefault) : pValue;
	}

	int NormalizeCfgCount(const char* p_szNode, int p_iValue, int p_iMaxValue)
	{
		if (p_iValue < 0)
		{
			MT_WARN("[MtAssistant] invalid xml node count %s=%d, use 0", p_szNode, p_iValue);
			return 0;
		}
		if (p_iValue > p_iMaxValue)
		{
			MT_WARN("[MtAssistant] xml node count %s=%d exceeds max %d, clamped", p_szNode, p_iValue, p_iMaxValue);
			return p_iMaxValue;
		}
		return p_iValue;
	}

	bool ParseRunTime(const char* p_szValue, tagRunTime& p_refTime)
	{
		if (p_szValue == NULL)
			return false;

		int hour = -1, min = -1, sec = -1, pos = 0;
		if (sscanf_s(p_szValue, " %d:%d:%d %n", &hour, &min, &sec, &pos) != 3 || p_szValue[pos] != '\0')
			return false;

		if (hour < 0 || hour > 23 || min < 0 || min > 59 || sec < 0 || sec > 59)
			return false;

		p_refTime.hour = hour;
		p_refTime.min = min;
		p_refTime.sec = sec;
		return true;
	}

	string BuildServiceLabel(int p_iRow)
	{
		char szLabel[64] = { 0 };
		sprintf_s(szLabel, "Info_%d", p_iRow);
		return szLabel;
	}

	void SetServiceRuntimeFields(ServiceInfo& p_refServiceInfo, int p_iRow, const string& p_refCfgPath)
	{
		p_refServiceInfo.iRow = p_iRow;
		p_refServiceInfo.strLabel = BuildServiceLabel(p_iRow);
		p_refServiceInfo.strCfg = p_refCfgPath;
	}
}

bool IsAbsoluteProgramPath(const string& p_refPath)
{
	if (p_refPath.length() >= 3 &&
		std::isalpha((unsigned char)p_refPath[0]) &&
		p_refPath[1] == ':' &&
		IsLocalPathDelimiter(p_refPath[2]))
	{
		return true;
	}

	return p_refPath.length() >= 2 &&
		IsLocalPathDelimiter(p_refPath[0]) &&
		IsLocalPathDelimiter(p_refPath[1]);
}

bool NormalizeFullProgramPath(const string& p_refPath, string& p_refFullPath)
{
	if (p_refPath.empty() || !IsAbsoluteProgramPath(p_refPath))
		return false;

	char szFullPath[4096] = { 0 };
	DWORD dwLen = GetFullPathNameA(p_refPath.c_str(), sizeof(szFullPath), szFullPath, NULL);
	if (dwLen == 0 || dwLen >= sizeof(szFullPath))
		return false;

	p_refFullPath = NormalizeSlash(szFullPath);
	return IsAbsoluteProgramPath(p_refFullPath);
}

string GetProgramFileName(const string& p_refPath)
{
	size_t nPos = p_refPath.find_last_of("\\/");
	if (nPos == string::npos)
		return p_refPath;

	return p_refPath.substr(nPos + 1);
}

const char* WeekToXmlName(WeekInfo p_enWeek)
{
	switch (p_enWeek)
	{
	case MON: return "Mon";
	case TUE: return "Tue";
	case WED: return "Wed";
	case THU: return "Thu";
	case FRI: return "Fri";
	case SAT: return "Sat";
	case SUN: return "Sun";
	default: return "Mon";
	}
}

bool XmlNameToWeek(const char* p_szName, WeekInfo& p_refWeek)
{
	if (p_szName == NULL)
		return false;

	if (_stricmp(p_szName, "Mon") == 0) { p_refWeek = MON; return true; }
	if (_stricmp(p_szName, "Tue") == 0) { p_refWeek = TUE; return true; }
	if (_stricmp(p_szName, "Wed") == 0) { p_refWeek = WED; return true; }
	if (_stricmp(p_szName, "Thu") == 0) { p_refWeek = THU; return true; }
	if (_stricmp(p_szName, "Fri") == 0) { p_refWeek = FRI; return true; }
	if (_stricmp(p_szName, "Sat") == 0) { p_refWeek = SAT; return true; }
	if (_stricmp(p_szName, "Sun") == 0) { p_refWeek = SUN; return true; }

	return false;
}

bool LoadServiceConfigXml(const string& p_refCfgPath, ServiceConfigXmlData& p_refData)
{
	p_refData = ServiceConfigXmlData();

	TiXmlDocument doc(p_refCfgPath.c_str());
	if (!doc.LoadFile(TIXML_ENCODING_UTF8))
	{
		MT_WARN("[MtAssistant] load xml config failed, path=%s,error=%s", p_refCfgPath.c_str(), doc.ErrorDesc());
		return false;
	}

	TiXmlElement* pRoot = doc.RootElement();
	if (pRoot == NULL || _stricmp(pRoot->Value(), "MtAssistant") != 0)
	{
		MT_WARN("[MtAssistant] invalid xml root, path=%s", p_refCfgPath.c_str());
		return false;
	}

	TiXmlElement* pLog = pRoot->FirstChildElement("Log");
	p_refData.strLogLevel = ReadStringAttribute(pLog, "level", "info");
	if (p_refData.strLogLevel.empty())
		p_refData.strLogLevel = "info";

	TiXmlElement* pServices = pRoot->FirstChildElement("Services");
	if (pServices == NULL)
		return true;

	int iRow = 0;
	int iSeenService = 0;
	for (TiXmlElement* pService = pServices->FirstChildElement("Service");
		pService != NULL;
		pService = pService->NextSiblingElement("Service"))
	{
		++iSeenService;
		if (NormalizeCfgCount("Service", iSeenService, MAX_SERVICE_CFG_COUNT) != iSeenService)
			break;

		ServiceInfo stServiceInfo;
		stServiceInfo.strPath = ReadStringAttribute(pService, "path", "");
		string strFullPath;
		if (!NormalizeFullProgramPath(stServiceInfo.strPath, strFullPath))
		{
			MT_WARN("[MtAssistant] invalid service absolute path, path=%s", stServiceInfo.strPath.c_str());
			continue;
		}

		stServiceInfo.strPath = strFullPath;
		stServiceInfo.strName = ReadStringAttribute(pService, "name", "");
		if (stServiceInfo.strName.empty())
			stServiceInfo.strName = GetProgramFileName(stServiceInfo.strPath);

		stServiceInfo.iEnable = ReadIntAttribute(pService, "enable", 0);
		stServiceInfo.lPid = ReadIntAttribute(pService, "pid", -1);
		stServiceInfo.iCheck = ReadIntAttribute(pService, "check", 1);
		if (_stricmp(stServiceInfo.strName.c_str(), "TradingTerminal.exe") == 0)
			stServiceInfo.strTitle = "TradingTerminal.exe";

		int iTimeCount = 0;
		for (TiXmlElement* pSchedule = pService->FirstChildElement("Schedule");
			pSchedule != NULL;
			pSchedule = pSchedule->NextSiblingElement("Schedule"))
		{
			WeekInfo enWeek = MON;
			if (!XmlNameToWeek(pSchedule->Attribute("week"), enWeek))
			{
				MT_WARN("[MtAssistant] invalid schedule week, service=%s", stServiceInfo.strName.c_str());
				continue;
			}

			for (TiXmlElement* pTime = pSchedule->FirstChildElement("Time");
				pTime != NULL;
				pTime = pTime->NextSiblingElement("Time"))
			{
				++iTimeCount;
				if (NormalizeCfgCount("Time", iTimeCount, MAX_TIME_CFG_COUNT) != iTimeCount)
					break;

				TimeInfo stTimeInfo;
				if (!ParseRunTime(pTime->Attribute("start"), stTimeInfo.StartTime))
				{
					MT_WARN("[MtAssistant] invalid start time, service=%s", stServiceInfo.strName.c_str());
					continue;
				}
				if (!ParseRunTime(pTime->Attribute("end"), stTimeInfo.EndTime))
				{
					MT_WARN("[MtAssistant] invalid end time, service=%s", stServiceInfo.strName.c_str());
					continue;
				}
				stServiceInfo.mapTimeConf[enWeek].push_back(stTimeInfo);
			}
		}

		SetServiceRuntimeFields(stServiceInfo, iRow, p_refCfgPath);
		p_refData.mapServiceInfo[iRow] = stServiceInfo;
		++iRow;
	}

	return true;
}

bool SaveServiceConfigXml(const string& p_refCfgPath, const ServiceConfigXmlData& p_refData)
{
	TiXmlDocument doc;
	doc.LinkEndChild(new TiXmlDeclaration("1.0", "utf-8", ""));

	TiXmlElement* pRoot = new TiXmlElement("MtAssistant");
	doc.LinkEndChild(pRoot);

	AddComment(pRoot, "Log: log config. level supports debug/info/warn/error. Default is info.");
	TiXmlElement* pLog = new TiXmlElement("Log");
	pLog->SetAttribute("level", p_refData.strLogLevel.empty() ? "info" : p_refData.strLogLevel.c_str());
	pRoot->LinkEndChild(pLog);

	AddComment(pRoot, "Services: programs managed by MtAssistant.");
	TiXmlElement* pServices = new TiXmlElement("Services");
	pRoot->LinkEndChild(pServices);

	AddComment(pServices, "Service: name is file name; path must be a full absolute program path; enable starts daemon; pid is last runtime pid; check enables launch outside schedule.");
	for (auto it = p_refData.mapServiceInfo.begin(); it != p_refData.mapServiceInfo.end(); ++it)
	{
		ServiceInfo stServiceInfo = it->second;
		string strFullPath;
		if (!NormalizeFullProgramPath(stServiceInfo.strPath, strFullPath))
		{
			MT_WARN("[MtAssistant] skip invalid service path when saving xml, path=%s", stServiceInfo.strPath.c_str());
			continue;
		}

		TiXmlElement* pService = new TiXmlElement("Service");
		pService->SetAttribute("name", stServiceInfo.strName.empty() ? GetProgramFileName(strFullPath).c_str() : stServiceInfo.strName.c_str());
		pService->SetAttribute("path", strFullPath.c_str());
		pService->SetAttribute("enable", stServiceInfo.iEnable);
		pService->SetAttribute("pid", (int)stServiceInfo.lPid);
		pService->SetAttribute("check", stServiceInfo.iCheck);
		pServices->LinkEndChild(pService);

		AddComment(pService, "Schedule: restart time group. week can be Mon/Tue/Wed/Thu/Fri/Sat/Sun.");
		for (int i = 0; i < WEEK_NUM; ++i)
		{
			WeekInfo enWeek = (WeekInfo)i;
			auto itTime = stServiceInfo.mapTimeConf.find(enWeek);
			if (itTime == stServiceInfo.mapTimeConf.end() || itTime->second.empty())
				continue;

			TiXmlElement* pSchedule = new TiXmlElement("Schedule");
			pSchedule->SetAttribute("week", WeekToXmlName(enWeek));
			pService->LinkEndChild(pSchedule);
			AddComment(pSchedule, "Time: restart time range. start/end format is HH:mm:ss.");

			for (size_t j = 0; j < itTime->second.size(); ++j)
			{
				TiXmlElement* pTime = new TiXmlElement("Time");
				pTime->SetAttribute("start", itTime->second[j].StartTime.toString().c_str());
				pTime->SetAttribute("end", itTime->second[j].EndTime.toString().c_str());
				pSchedule->LinkEndChild(pTime);
			}
		}
	}

	if (!doc.SaveFile(p_refCfgPath.c_str()))
	{
		MT_WARN("[MtAssistant] save xml config failed, path=%s", p_refCfgPath.c_str());
		return false;
	}

	return true;
}
