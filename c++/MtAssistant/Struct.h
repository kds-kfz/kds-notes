#ifndef __STRUCT_H__
#define __STRUCT_H__

#include <string>
#include <map>
#include <vector>
#include <iterator>

using namespace std;

#define WEEK_NUM 7		//默认星期天数
#define SERVER_NUM 10	//默认服务个数

struct tagRunTime
{
	int year;
	int month;
	int day;
	int hour;
	int	min;
	int	sec;
	bool lastFlag;
	tagRunTime()
	{
		memset(this, 0, sizeof(tagRunTime));
	}

	std::string toString()
	{
		char src[128] = { 0 };
		if (year == 0 || month == 0)
			sprintf_s(src, "%d:%d:%d", hour, min, sec);
		else
			sprintf_s(src, "%d/%d/%d %d:%d:%d", year, month, day, hour, min, sec);
		return std::string(src);
	}

	int getHM()
	{
		return hour * 60 + min;
	}

	int getSec()
	{
		return (hour * 60 + min) * 60 + sec;
	}

	tagRunTime& fromString(std::string& src)
	{
		const char *value = src.c_str();
		int pos = 0;
		if (src.find('/') == std::string::npos)
		{
			int newHour = -1, newMin = -1, newSec = -1;
			if (sscanf_s(value, " %d:%d:%d %n", &newHour, &newMin, &newSec, &pos) == 3 &&
				value[pos] == '\0' && newHour >= 0 && newHour <= 23 && newMin >= 0 && newMin <= 59 && newSec >= 0 && newSec <= 59)
			{
				hour = newHour;
				min = newMin;
				sec = newSec;
			}
		}
		else
		{
			int newYear = 0, newMonth = 0, newDay = 0, newHour = -1, newMin = -1, newSec = -1;
			if (sscanf_s(value, " %d/%d/%d %d:%d:%d %n", &newYear, &newMonth, &newDay, &newHour, &newMin, &newSec, &pos) == 6 &&
				value[pos] == '\0' && newYear >= 0 && newMonth >= 1 && newMonth <= 12 && newDay >= 1 && newDay <= 31 &&
				newHour >= 0 && newHour <= 23 && newMin >= 0 && newMin <= 59 && newSec >= 0 && newSec <= 59)
			{
				year = newYear;
				month = newMonth;
				day = newDay;
				hour = newHour;
				min = newMin;
				sec = newSec;
			}
		}
		return *this;
	}

	tagRunTime& fromSystemTime(const SYSTEMTIME& date, const SYSTEMTIME& time)
	{
		year = date.wYear;
		month = date.wMonth;
		day = date.wDay;

		hour = time.wHour;
		min = time.wMinute;
		sec = time.wSecond;
		return *this;
	}

	tagRunTime& fromSystemTime(const SYSTEMTIME& time)
	{
		hour = time.wHour;
		min = time.wMinute;
		sec = time.wSecond;
		return *this;
	}

	bool compare(time_t tt)
	{
		struct tm* nt = localtime(&tt);
		if (year > 0)
		{
			if (year < nt->tm_year + 1900)
				return false;
			else if (year > nt->tm_year + 1900)
				return true;
		}

		if (month > 0)
		{
			if (month < nt->tm_mon + 1)
				return false;
			else if (month > nt->tm_mon + 1)
				return true;
		}
		if (day > 0)
		{
			if (day < nt->tm_mday)
				return false;
			else if (day > nt->tm_mday)
				return true;
		}
		if (hour < nt->tm_hour)
			return false;
		else if (hour > nt->tm_hour)
			return true;

		if (min < nt->tm_min)
			return false;
		else if (min > nt->tm_min)
			return true;

		if (sec > nt->tm_sec)
			return true;
		return false;
	}

	//时间差半个小时内有效
	bool isAtTime(time_t NowTime)
	{
		time_t dectm = NowTime - 30 * 60;
		if (dectm < 0)
			return false;
		bool retn = compare(NowTime);
		bool retd = compare(dectm);

		if (!retn && retd && !lastFlag)
		{
			lastFlag = true;
			return true;
		}
		return false;
	}
};

//星期类型
enum WeekInfo
{
	MON = 0,
	TUE,
	WED,
	THU,
	FRI,
	SAT,
	SUN
};

//操作类型
enum OperationType
{
	ADD = 0,	// 增
	DEL,		// 删
	MODIFY		// 改
};

//时间信息
struct TimeInfo
{
	tagRunTime StartTime;// 开始时间
	tagRunTime EndTime;	// 结束时间
};

//服务信息
struct ServiceInfo
{
	string strLabel;	// 服务标签
	string strName;		// 服务文件名
	string strTitle;	// 服务Title
	string strPath;		// 服务路径
	string strCmdParam;	// 命令行参数
	string strCfg;		// 配置路径
	int iEnable;		// 服务使能
	bool bStatus;		// 服务状态
	long lPid;			// 进程号
	int iCheck;			// 是否在非配置时间检测
	int iRow;			// wyl 2026-07-03：XML配置中的服务行号，守护线程回写运行时PID时使用。
	map<WeekInfo, vector<TimeInfo> > mapTimeConf;

	ServiceInfo():strLabel(""), strName(""), strTitle(""), strPath(""), strCmdParam(""), iEnable(0), bStatus(false), lPid(-1), strCfg(""), iCheck(1), iRow(-1)
	{
		mapTimeConf.clear();
	}

	ServiceInfo& operator=(const ServiceInfo &p_ServiceInfo) {
		if (this != &p_ServiceInfo) {
			strLabel = p_ServiceInfo.strLabel;
			strName = p_ServiceInfo.strName;
			strTitle = p_ServiceInfo.strTitle;
			strPath = p_ServiceInfo.strPath;
			iEnable = p_ServiceInfo.iEnable;
			bStatus = p_ServiceInfo.bStatus;
			strCmdParam = p_ServiceInfo.strCmdParam;
			strCfg = p_ServiceInfo.strCfg;
			lPid = p_ServiceInfo.lPid;
			iCheck = p_ServiceInfo.iCheck;
			iRow = p_ServiceInfo.iRow;
			// Must replace the schedule map. std::map::insert keeps old week entries,
			// which makes updated restart times invisible to existing worker threads.
			mapTimeConf = p_ServiceInfo.mapTimeConf;
		}
		return *this;
	}

	ServiceInfo(const ServiceInfo &p_ServiceInfo):iEnable(p_ServiceInfo.iEnable),bStatus(p_ServiceInfo.bStatus), lPid(p_ServiceInfo.lPid), iCheck(p_ServiceInfo.iCheck), iRow(p_ServiceInfo.iRow)
	{
		strLabel = p_ServiceInfo.strLabel;
		strName = p_ServiceInfo.strName;
		strTitle = p_ServiceInfo.strTitle;
		strPath = p_ServiceInfo.strPath;
		strCmdParam = p_ServiceInfo.strCmdParam;
		strCfg = p_ServiceInfo.strCfg;
		mapTimeConf = p_ServiceInfo.mapTimeConf;
	}

	void AddTime(WeekInfo p_enWeek, TimeInfo p_stTimeInfo)
	{
		mapTimeConf[p_enWeek].push_back(p_stTimeInfo);
	}

	void DelTime(WeekInfo p_enWeek, int p_iRow)
	{
		vector<TimeInfo> &vecTimeInfo = mapTimeConf[p_enWeek];
		int iPos = 0;
		for (auto it = vecTimeInfo.begin(); it != vecTimeInfo.end(); ) {
			if (iPos == p_iRow) {
				it = vecTimeInfo.erase(it);
			}
			else {
				++iPos;
				++it;
			}
		}
	}
};

#endif
