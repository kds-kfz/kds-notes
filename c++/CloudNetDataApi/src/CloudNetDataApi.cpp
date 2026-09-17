#include "CloudNetDataApi.h"

#include <algorithm>
#include <chrono>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <atomic>
#include <cctype>
#include <condition_variable>
#include <cstring>
#include <direct.h>
#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <set>
#include <thread>
#include <string>
#include <vector>

#include "IceRPCPush.h"
#include "tinyxml.h"
#ifdef StartService
#undef StartService
#endif

namespace
{
// 统一错误码，便于 C 调用方判断失败原因。
const int s_iCloudNetOk = 0;
const int s_iCloudNetInvalidParam = -1;
const int s_iCloudNetNotFound = -2;
const int s_iCloudNetCreateFailed = -3;
const int s_iCloudNetCallFailed = -4;
const int s_iCloudNetStateError = -5;
const int s_iCloudNetConfigError = -6;
const int s_iCloudNetMemoryError = -7;
const int s_iCloudNetInternalError = -8;
const int s_iCloudNetBinaryProtocolError = -9;
const int s_iCloudNetBinaryCallFailed = -10;

// Ice 注册结果只有同时具备有效地址和非空代理字符串时才表示成功。
constexpr bool IsPushRegistrationResultValid(const char* p_szResult)
{
	return p_szResult != NULL && p_szResult[0] != '\0';
}

static_assert(!IsPushRegistrationResultValid(NULL), "null push registration result must fail");
static_assert(!IsPushRegistrationResultValid(""), "empty push registration result must fail");
static_assert(IsPushRegistrationResultValid("proxy"), "non-empty push registration result must pass");

struct ST_CLOUD_NET_CODE_MSG
{
	int iCode;                 // 错误码，0 表示成功，负数表示失败。
	const char* szCode;        // 错误码英文简称，便于上层日志检索。
	const char* szMsg;         // 默认英文错误描述，不包含业务上下文。
};

const ST_CLOUD_NET_CODE_MSG s_aCloudNetCodeMsg[] =
{
	{s_iCloudNetOk, "OK", "OK"},
	{s_iCloudNetInvalidParam, "INVALID_PARAM", "INVALID_PARAM: input parameter is invalid"},
	{s_iCloudNetNotFound, "NOT_FOUND", "NOT_FOUND: target object was not found"},
	{s_iCloudNetCreateFailed, "CREATE_FAILED", "CREATE_FAILED: internal object creation failed"},
	{s_iCloudNetCallFailed, "CALL_FAILED", "CALL_FAILED: remote call failed"},
	{s_iCloudNetStateError, "STATE_ERROR", "STATE_ERROR: object state does not allow this operation"},
	{s_iCloudNetConfigError, "CONFIG_ERROR", "CONFIG_ERROR: configuration is invalid"},
	{s_iCloudNetMemoryError, "MEMORY_ERROR", "MEMORY_ERROR: memory allocation failed"},
	{s_iCloudNetInternalError, "INTERNAL_ERROR", "INTERNAL_ERROR: unexpected internal exception"},
	{s_iCloudNetBinaryProtocolError, "BINARY_PROTOCOL_ERROR", "BINARY_PROTOCOL_ERROR: binary call structure or payload is invalid"},
	{s_iCloudNetBinaryCallFailed, "BINARY_CALL_FAILED", "BINARY_CALL_FAILED: binary remote call failed"},
};

thread_local int s_iThreadLastErrorCode = s_iCloudNetOk;
thread_local std::string s_strThreadLastError = "OK";

// 前置声明用于底层日志在文件路径和错误正文中复用统一 ASCII 转义规则。
std::string MakeApiErrorAscii(const std::string& p_strText);

// CloudNetDataApi 进程级错误日志，只负责记录本 DLL 的连接管理和调用转换错误。
// 日志对象内部串行化目录配置和文件追加；不持有业务回调、Ice 句柄或上层对象。
class CCloudNetFileLog
{
public:
	// 返回进程级日志实例；六服务采用独立进程，因此一个进程只对应一份 CloudNet 配置。
	static CCloudNetFileLog& Instance()
	{
		static CCloudNetFileLog clLog;
		return clLog;
	}

	// 应用 XML 日志配置。路径初始化失败只输出调试信息，不阻断业务网络启动。
	void Configure(const std::string& p_refStrPath, const std::string& p_refStrName,
		const std::string& p_refStrLevel)
	{
		std::lock_guard<std::mutex> clLock(m_clMutex);
		m_bEnable = _stricmp(p_refStrLevel.c_str(), "off") != 0;
		m_strName = p_refStrName.empty() ? "CloudNetDataApi" : p_refStrName;
		m_strDirectory = ResolveDirectory(p_refStrPath);
		if (!m_bEnable)
		{
			return;
		}
		if (!EnsureDirectory(m_strDirectory))
		{
			std::string strError = "CloudNetDataApi log initialize failed path=" +
				MakeApiErrorAscii(m_strDirectory) + "\r\n";
			OutputDebugStringA(strError.c_str());
			m_bEnable = false;
		}
	}

	// 追加一条错误日志。函数在内部加锁，每次写入后立即关闭文件，DLL 卸载时不遗留文件句柄。
	void Write(const char* p_szFunction, int p_iErrorCode, const std::string& p_refStrDetail)
	{
		WriteLevel("ERROR", p_szFunction, p_iErrorCode, p_refStrDetail);
	}

	// 记录低频依赖生命周期信息；不得用于逐请求或正文日志。
	void WriteInfo(const char* p_szFunction, const std::string& p_refStrDetail)
	{
		WriteLevel("INFO", p_szFunction, s_iCloudNetOk, p_refStrDetail);
	}

private:
	// 按指定级别追加一条日志，调用方只传递非敏感连接标识和阶段信息。
	void WriteLevel(const char* p_szLevel, const char* p_szFunction,
		int p_iErrorCode, const std::string& p_refStrDetail)
	{
		std::lock_guard<std::mutex> clLock(m_clMutex);
		if (!m_bEnable || m_strDirectory.empty())
		{
			return;
		}

		std::time_t ttNow = std::time(NULL);
		std::tm stLocalTime = {};
		localtime_s(&stLocalTime, &ttNow);
		char szDate[16] = {0};
		char szTime[32] = {0};
		strftime(szDate, sizeof(szDate), "%Y%m%d", &stLocalTime);
		strftime(szTime, sizeof(szTime), "%Y%m%d %H:%M:%S", &stLocalTime);

		std::string strFilePath = m_strDirectory + "\\" + m_strName + "_" + szDate + ".log";
		FILE* pFile = NULL;
		if (fopen_s(&pFile, strFilePath.c_str(), "a+") != 0 || pFile == NULL)
		{
			std::string strError = "CloudNetDataApi log open failed path=" +
				MakeApiErrorAscii(strFilePath) + "\r\n";
			OutputDebugStringA(strError.c_str());
			return;
		}
		fprintf(pFile, "[%s] [%s] func=%s code=%d detail=%s\n", szTime,
			p_szLevel != NULL ? p_szLevel : "INFO",
			p_szFunction != NULL ? p_szFunction : "unknown", p_iErrorCode,
			MakeApiErrorAscii(p_refStrDetail).c_str());
		fclose(pFile);
	}

	CCloudNetFileLog()
		: m_clMutex()
		, m_strDirectory()
		, m_strName("CloudNetDataApi")
		, m_bEnable(false)
	{
	}

	// 将相对路径固定解释为 EXE 同级目录，避免 DLL 所在位置影响部署日志路径。
	std::string ResolveDirectory(const std::string& p_refStrPath) const
	{
		std::string strPath = p_refStrPath.empty() ? "log" : p_refStrPath;
		std::replace(strPath.begin(), strPath.end(), '/', '\\');
		if ((strPath.size() > 1 && strPath[1] == ':') ||
			(strPath.size() > 1 && strPath[0] == '\\' && strPath[1] == '\\'))
		{
			return strPath;
		}

		char szExePath[MAX_PATH] = {0};
		DWORD uPathLen = GetModuleFileNameA(NULL, szExePath, static_cast<DWORD>(sizeof(szExePath)));
		std::string strRoot;
		if (uPathLen > 0 && uPathLen < sizeof(szExePath))
		{
			strRoot.assign(szExePath, uPathLen);
			std::string::size_type uSeparator = strRoot.find_last_of("\\/");
			if (uSeparator != std::string::npos)
			{
				strRoot.erase(uSeparator);
			}
		}
		if (strRoot.empty())
		{
			return strPath;
		}
		return strRoot + "\\" + strPath;
	}

	// 逐级创建日志目录；目录已经存在视为成功，其他系统错误由调用方降级处理。
	bool EnsureDirectory(const std::string& p_refStrDirectory) const
	{
		if (p_refStrDirectory.empty())
		{
			return false;
		}
		std::string strCurrent;
		for (std::size_t uIndex = 0; uIndex < p_refStrDirectory.size(); ++uIndex)
		{
			const char chValue = p_refStrDirectory[uIndex];
			strCurrent += chValue;
			if (chValue != '\\' || strCurrent.size() <= 3)
			{
				continue;
			}
			if (_mkdir(strCurrent.c_str()) != 0 && errno != EEXIST)
			{
				return false;
			}
		}
		return _mkdir(strCurrent.c_str()) == 0 || errno == EEXIST;
	}

	std::mutex m_clMutex;       // 保护配置和单条日志写入，避免后台重连线程并发写文件。
	std::string m_strDirectory; // 已解析的绝对日志目录，生命周期覆盖当前进程。
	std::string m_strName;      // 日志文件名前缀，不包含日期和扩展名。
	bool m_bEnable;             // level=off 时关闭文件日志，但最后错误查询仍保持有效。
};

const char* GetCloudNetErrorMsg(int p_iErrorCode)
{
	for (size_t uIndex = 0; uIndex < sizeof(s_aCloudNetCodeMsg) / sizeof(s_aCloudNetCodeMsg[0]); ++uIndex)
	{
		if (s_aCloudNetCodeMsg[uIndex].iCode == p_iErrorCode)
		{
			return s_aCloudNetCodeMsg[uIndex].szMsg;
		}
	}
	return "UNKNOWN_ERROR: error code is not registered";
}

std::string MakeApiErrorAscii(const std::string& p_strText)
{
	// 对外错误描述只允许 ASCII；中文路径或系统异常文本按字节转义，避免调用方编码不一致。
	static const char s_aHex[] = "0123456789ABCDEF";
	std::string strResult;
	strResult.reserve(p_strText.size());
	for (std::string::const_iterator it = p_strText.begin(); it != p_strText.end(); ++it)
	{
		unsigned char chValue = static_cast<unsigned char>(*it);
		if (chValue >= 32 && chValue <= 126)
		{
			strResult += static_cast<char>(chValue);
		}
		else if (chValue == '\r')
		{
			strResult += "\\r";
		}
		else if (chValue == '\n')
		{
			strResult += "\\n";
		}
		else if (chValue == '\t')
		{
			strResult += "\\t";
		}
		else
		{
			strResult += "\\x";
			strResult += s_aHex[(chValue >> 4) & 0x0F];
			strResult += s_aHex[chValue & 0x0F];
		}
	}
	return strResult;
}

std::string MakeApiErrorAscii(const char* p_szText)
{
	return MakeApiErrorAscii(std::string(p_szText != NULL ? p_szText : ""));
}

void SetThreadLastError(int p_iErrorCode, const std::string& p_strDetail)
{
	s_iThreadLastErrorCode = p_iErrorCode;
	s_strThreadLastError = p_strDetail.empty() ? GetCloudNetErrorMsg(p_iErrorCode) : MakeApiErrorAscii(p_strDetail);
}

void WriteCloudNetErrorLog(const char* p_szFunction, int p_iErrorCode, const std::string& p_strDetail)
{
	if (p_iErrorCode == s_iCloudNetOk)
	{
		return;
	}
	std::string strSafeDetail = MakeApiErrorAscii(p_strDetail);
	char szLog[1024] = {0};
	sprintf_s(szLog, sizeof(szLog), "CloudNetDataApi error func=%s code=%d detail=%s\r\n", p_szFunction != NULL ? p_szFunction : "unknown", p_iErrorCode, strSafeDetail.c_str());
	OutputDebugStringA(szLog);
	CCloudNetFileLog::Instance().Write(p_szFunction, p_iErrorCode, strSafeDetail);
}

bool StartsWithText(const char* p_szText, const char* p_szPrefix)
{
	if (p_szText == NULL || p_szPrefix == NULL)
	{
		return false;
	}
	while (*p_szPrefix != '\0')
	{
		if (*p_szText == '\0' || *p_szText != *p_szPrefix)
		{
			return false;
		}
		++p_szText;
		++p_szPrefix;
	}
	return true;
}

int InferCloudNetErrorCode(const char* p_szError)
{
	// 通过统一英文错误前缀归类错误码，便于旧接口不改签名也能查询明确错误。
	if (p_szError == NULL || p_szError[0] == '\0' || StartsWithText(p_szError, "OK"))
	{
		return s_iCloudNetOk;
	}
	if (StartsWithText(p_szError, "INVALID_PARAM") || StartsWithText(p_szError, "INVALID_HANDLE"))
	{
		return s_iCloudNetInvalidParam;
	}
	if (StartsWithText(p_szError, "CONFIG_") || StartsWithText(p_szError, "RUNTIME_CONFIG_") || StartsWithText(p_szError, "TEMP_PATH_"))
	{
		return s_iCloudNetConfigError;
	}
	if (StartsWithText(p_szError, "CONNECTION_NOT_FOUND") || StartsWithText(p_szError, "SERVICE_NOT_STARTED") || StartsWithText(p_szError, "NOT_FOUND"))
	{
		return s_iCloudNetNotFound;
	}
	if (StartsWithText(p_szError, "CONNECTION_CLOSING") || StartsWithText(p_szError, "CONFIG_NOT_LOADED") || StartsWithText(p_szError, "STATE_ERROR"))
	{
		return s_iCloudNetStateError;
	}
	if (StartsWithText(p_szError, "MEMORY_ERROR"))
	{
		return s_iCloudNetMemoryError;
	}
	if (StartsWithText(p_szError, "ICE_SERVER_CREATE_FAILED") || StartsWithText(p_szError, "ICE_CLIENT_CREATE_FAILED") || StartsWithText(p_szError, "CREATE_FAILED"))
	{
		return s_iCloudNetCreateFailed;
	}
	if (StartsWithText(p_szError, "RPC_") || StartsWithText(p_szError, "PUT_") || StartsWithText(p_szError, "PUSH_") || StartsWithText(p_szError, "PUBLISH_") || StartsWithText(p_szError, "CALL_FAILED"))
	{
		return s_iCloudNetCallFailed;
	}
	if (StartsWithText(p_szError, "BINARY_PROTOCOL_") || StartsWithText(p_szError, "BINARY_REQUEST_INVALID"))
	{
		return s_iCloudNetBinaryProtocolError;
	}
	if (StartsWithText(p_szError, "BINARY_"))
	{
		return s_iCloudNetBinaryCallFailed;
	}
	return s_iCloudNetInternalError;
}

std::string MakeCloudNetStdExceptionDetail(const char* p_szFunction, const std::exception& p_refException)
{
	std::string strDetail = "INTERNAL_EXCEPTION: func=";
	strDetail += p_szFunction != NULL ? p_szFunction : "unknown";
	strDetail += ", what=";
	strDetail += MakeApiErrorAscii(p_refException.what());
	return strDetail;
}

std::string MakeCloudNetUnknownExceptionDetail(const char* p_szFunction)
{
	std::string strDetail = "UNKNOWN_EXCEPTION: func=";
	strDetail += p_szFunction != NULL ? p_szFunction : "unknown";
	return strDetail;
}

void SetCloudNetBoundaryThreadError(int p_iErrorCode, const std::string& p_strDetail, const char* p_szFunction)
{
	SetThreadLastError(p_iErrorCode, p_strDetail);
	WriteCloudNetErrorLog(p_szFunction, p_iErrorCode, p_strDetail);
}

const char* s_szDefaultConfigFile = "IceRPCPush.xml";
const char* s_szRuntimeConfigPrefix = "CloudNetRuntime_";

// 连接上下文保存底层客户端句柄和推送回调信息，使用 shared_ptr 防止回调期间对象被释放。
struct ST_CONNECTION_CONTEXT
{
	mutable std::mutex clMutex;              // 保护当前连接上下文的句柄、关闭状态和回调信息。
	std::condition_variable clIdleCond;      // 等待连接上正在执行的调用全部退出。
	HANDLE hClient;                          // IceRPCPush 客户端句柄，由 Connect 创建，Disconnect 释放。
	PFN_CLOUD_NET_PUSH pfnPush;              // 推送回调函数，注册成功后由底层推送线程触发。
	void* pPushParam;                        // 推送回调透传参数，CloudNetDataApi 不拥有其生命周期。
	bool bClosing;                           // 连接关闭标记，防止 Disconnect 后继续进入底层调用。
	int iActiveCall;                         // 当前正在使用 hClient 的调用数量，用于安全断开连接。
	std::string strConnName;                 // 连接名称，对应 XML Services.Service.id 或调用方传入名称。
	std::string strRegisterRet;              // RegisterPush 返回的底层注册结果缓存。

	ST_CONNECTION_CONTEXT()
	{
		hClient = NULL;
		pfnPush = NULL;
		pPushParam = NULL;
		bClosing = false;
		iActiveCall = 0;
		strConnName.clear();
		strRegisterRet.clear();
	}
};

typedef std::shared_ptr<ST_CONNECTION_CONTEXT> ST_CONNECTION_CONTEXT_PTR;


// 本地插件配置，对应 CloudNet/Local 节。
struct ST_LOCAL_CONFIG
{
	std::string strId;        // 当前插件唯一标识，对应 Local.id。
	std::string strService;   // 当前插件启动本地 Ice 服务时使用的服务名。
	bool bStartService;       // 是否启动本地服务，true 表示 StartNetwork 自动启动服务端。
	int iSnappy;              // 是否启用 snappy 压缩，非 0 表示启用。

	ST_LOCAL_CONFIG()
	{
		strId.clear();
		strService.clear();
		bStartService = true;
		iSnappy = 1;
	}
};

// 远端服务配置，对应 CloudNet/Services/Service 节。
struct ST_SERVICE_CONFIG
{
	std::string strId;        // 远端服务逻辑名称，作为连接表中的连接名。
	std::string strProxy;     // Ice 代理配置名或 Locator 服务名。
	bool bAutoConnect;        // StartNetwork 后是否自动连接该远端服务。

	ST_SERVICE_CONFIG()
	{
		strId.clear();
		strProxy.clear();
		bAutoConnect = true;
	}
};

// 推送订阅配置，对应 CloudNet/Subscriptions/Subscribe 节。
struct ST_SUBSCRIPTION_CONFIG
{
	std::string strTarget;    // 要订阅的目标服务 id，必须能在 Services 中找到。
	std::string strPluginId;  // 注册到目标服务的插件标识，支持 ${PluginId} 替换。
	std::string strSubInfo;   // 订阅条件字符串，由业务层约定内容。
	bool bEnable;             // 是否启用该订阅，false 时仅保留配置不执行注册。

	ST_SUBSCRIPTION_CONFIG()
	{
		strTarget.clear();
		strPluginId.clear();
		strSubInfo.clear();
		bEnable = true;
	}
};

// 运行参数配置，对应 CloudNet/Runtime 节。
struct ST_RUNTIME_CONFIG
{
	int iReconnectIntervalMs; // 后台维护线程重连间隔，单位毫秒。
	int iRenewIntervalSec;    // RegisterPush 续约间隔，单位秒。
	int iThreadPool;          // 底层 Ice 客户端线程池大小，0 表示使用默认值。

	ST_RUNTIME_CONFIG()
	{
		iReconnectIntervalMs = 5000;
		iRenewIntervalSec = 240;
		iThreadPool = 0;
	}
};

// CloudNetDataApi 自身日志配置，对应 CloudNet/Log 节，不传递给 IceRPCPush。
struct ST_CLOUD_NET_LOG_CONFIG
{
	std::string strPath;       // 相对 EXE 或绝对日志目录。
	std::string strName;       // CloudNetDataApi 日志文件名前缀。
	std::string strLevel;      // debug/info/warn/error/off，当前错误日志按 off 开关控制。

	ST_CLOUD_NET_LOG_CONFIG()
	{
		strPath.clear();
		strName.clear();
		strLevel = "info";
	}
};

// 完整组网配置，内部维护使用，不向上层插件暴露。
struct ST_CLOUD_NET_CONFIG
{
	std::string strXmlPath;                       // 已加载的 XML 配置文件路径。
	std::string strPluginId;                      // 当前进程选择的插件 id。
	ST_LOCAL_CONFIG stLocal;                      // 当前插件本地服务配置。
	ST_RUNTIME_CONFIG stRuntime;                  // 后台线程运行参数。
	ST_CLOUD_NET_LOG_CONFIG stLog;                // CloudNetDataApi 独立错误日志配置。
	std::vector<ST_SERVICE_CONFIG> aService;      // 当前插件需要连接的远端服务列表。
	std::vector<ST_SUBSCRIPTION_CONFIG> aSubscription; // 当前插件需要注册的推送订阅列表。
	bool bLoaded;                                 // 配置是否已经成功加载。

	ST_CLOUD_NET_CONFIG()
	{
		strXmlPath.clear();
		strPluginId.clear();
		aService.clear();
		aSubscription.clear();
		bLoaded = false;
	}
};

std::string GetAttrText(const TiXmlElement* p_pElement, const char* p_szName, const char* p_szDefault)
{
	if (p_pElement == NULL || p_szName == NULL)
	{
		return p_szDefault != NULL ? p_szDefault : "";
	}
	const char* pValue = p_pElement->Attribute(p_szName);
	if (pValue == NULL)
	{
		return p_szDefault != NULL ? p_szDefault : "";
	}
	return pValue;
}

int GetAttrInt(const TiXmlElement* p_pElement, const char* p_szName, int p_iDefault)
{
	std::string strValue = GetAttrText(p_pElement, p_szName, "");
	if (strValue.empty())
	{
		return p_iDefault;
	}
	char* pEnd = NULL;
	long lValue = strtol(strValue.c_str(), &pEnd, 10);
	if (pEnd == strValue.c_str() || *pEnd != '\0')
	{
		return p_iDefault;
	}
	return static_cast<int>(lValue);
}

bool GetAttrBool(const TiXmlElement* p_pElement, const char* p_szName, bool p_bDefault)
{
	std::string strValue = GetAttrText(p_pElement, p_szName, "");
	if (strValue.empty())
	{
		return p_bDefault;
	}
	if (_stricmp(strValue.c_str(), "1") == 0 || _stricmp(strValue.c_str(), "true") == 0 || _stricmp(strValue.c_str(), "yes") == 0)
	{
		return true;
	}
	if (_stricmp(strValue.c_str(), "0") == 0 || _stricmp(strValue.c_str(), "false") == 0 || _stricmp(strValue.c_str(), "no") == 0)
	{
		return false;
	}
	return p_bDefault;
}

std::string ReplacePluginId(const std::string& p_strText, const std::string& p_strPluginId)
{
	std::string strResult = p_strText;
	const std::string strToken = "${PluginId}";
	std::string::size_type uPos = 0;
	while ((uPos = strResult.find(strToken, uPos)) != std::string::npos)
	{
		strResult.replace(uPos, strToken.length(), p_strPluginId);
		uPos += p_strPluginId.length();
	}
	return strResult;
}

std::string MakeSubscriptionKey(const ST_SUBSCRIPTION_CONFIG& p_refSub)
{
	return p_refSub.strTarget + "\n" + p_refSub.strPluginId + "\n" + p_refSub.strSubInfo;
}

bool HasServiceConfig(const ST_CLOUD_NET_CONFIG& p_refConfig, const std::string& p_strServiceId)
{
	for (std::vector<ST_SERVICE_CONFIG>::const_iterator it = p_refConfig.aService.begin(); it != p_refConfig.aService.end(); ++it)
	{
		if (it->strId == p_strServiceId)
		{
			return true;
		}
	}
	return false;
}

bool HasEnabledSubscription(const ST_CLOUD_NET_CONFIG& p_refConfig)
{
	for (std::vector<ST_SUBSCRIPTION_CONFIG>::const_iterator it = p_refConfig.aSubscription.begin(); it != p_refConfig.aSubscription.end(); ++it)
	{
		if (it->bEnable)
		{
			return true;
		}
	}
	return false;
}

// 判断配置节点是否属于当前插件；owner 为空表示兼容旧配置，对所有插件生效。
bool IsOwnerMatched(const TiXmlElement* p_pElement, const std::string& p_strPluginId)
{
	std::string strOwner = GetAttrText(p_pElement, "owner", "");
	if (strOwner.empty())
	{
		return true;
	}

	std::string strToken;
	for (std::string::size_type uIndex = 0; uIndex <= strOwner.size(); ++uIndex)
	{
		char chValue = uIndex < strOwner.size() ? strOwner[uIndex] : ',';
		if (chValue == ',' || chValue == ';' || chValue == '|' || isspace(static_cast<unsigned char>(chValue)))
		{
			if (strToken == "*" || strToken == p_strPluginId)
			{
				return true;
			}
			strToken.clear();
			continue;
		}
		strToken.push_back(chValue);
	}
	return false;
}

bool IsXmlPath(const char* p_szPath)
{
	if (p_szPath == NULL)
	{
		return false;
	}
	const char* pExt = strrchr(p_szPath, '.');
	return pExt != NULL && _stricmp(pExt, ".xml") == 0;
}

std::string MakeSafeFilePart(const char* p_szText)
{
	std::string strResult;
	if (p_szText == NULL)
	{
		return "Service";
	}
	for (const unsigned char* p = reinterpret_cast<const unsigned char*>(p_szText); *p != '\0'; ++p)
	{
		if (std::isalnum(*p) || *p == '_' || *p == '-')
		{
			strResult += static_cast<char>(*p);
		}
		else
		{
			strResult += '_';
		}
	}
	return strResult.empty() ? "Service" : strResult;
}

TiXmlElement* EnsureChildElement(TiXmlElement* p_pParent, const char* p_szName)
{
	if (p_pParent == NULL || p_szName == NULL)
	{
		return NULL;
	}
	TiXmlElement* pChild = p_pParent->FirstChildElement(p_szName);
	if (pChild != NULL)
	{
		return pChild;
	}
	pChild = new TiXmlElement(p_szName);
	p_pParent->LinkEndChild(pChild);
	return pChild;
}

TiXmlElement* FindPropertyElement(TiXmlElement* p_pParent, const char* p_szName)
{
	if (p_pParent == NULL || p_szName == NULL)
	{
		return NULL;
	}
	for (TiXmlElement* pElement = p_pParent->FirstChildElement("Property"); pElement != NULL; pElement = pElement->NextSiblingElement("Property"))
	{
		const char* pName = pElement->Attribute("name");
		if (pName != NULL && _stricmp(pName, p_szName) == 0)
		{
			return pElement;
		}
	}
	return NULL;
}

void SetIceProperty(TiXmlElement* p_pIce, const char* p_szName, const char* p_szValue)
{
	if (p_pIce == NULL || p_szName == NULL || p_szValue == NULL)
	{
		return;
	}
	TiXmlElement* pProperty = FindPropertyElement(p_pIce, p_szName);
	if (pProperty == NULL)
	{
		pProperty = new TiXmlElement("Property");
		pProperty->SetAttribute("name", p_szName);
		p_pIce->LinkEndChild(pProperty);
	}
	pProperty->SetAttribute("value", p_szValue);
}

bool MakeRuntimeConfigPath(const char* p_szServiceName, std::string& p_refPath, std::string& p_refError)
{
	char szTempDir[MAX_PATH] = {0};
	DWORD dwLen = GetTempPathA(sizeof(szTempDir), szTempDir);
	if (dwLen == 0 || dwLen >= sizeof(szTempDir))
	{
		p_refError = "TEMP_PATH_FAILED: GetTempPathA failed";
		return false;
	}

	char szFileName[MAX_PATH] = {0};
	int iRet = sprintf_s(szFileName, sizeof(szFileName), "%s%s%s_%lu.xml", szTempDir, s_szRuntimeConfigPrefix, MakeSafeFilePart(p_szServiceName).c_str(), GetCurrentProcessId());
	if (iRet <= 0)
	{
		p_refError = "RUNTIME_CONFIG_PATH_FAILED: failed to build runtime xml path";
		return false;
	}
	p_refPath = szFileName;
	return true;
}

bool BuildServerRuntimeConfig(const char* p_szXmlPath, const char* p_szServiceName, std::string& p_refRuntimePath, std::string& p_refError)
{
	p_refRuntimePath.clear();
	p_refError.clear();
	if (!IsXmlPath(p_szXmlPath))
	{
		return true;
	}

	std::string strXmlPath = (p_szXmlPath != NULL && p_szXmlPath[0] != '\0') ? p_szXmlPath : s_szDefaultConfigFile;
	TiXmlDocument clDoc;
	if (!clDoc.LoadFile(strXmlPath.c_str(), TIXML_ENCODING_UTF8))
	{
		p_refError = std::string("CONFIG_LOAD_FAILED: path=") + MakeApiErrorAscii(strXmlPath) + ", detail=" + MakeApiErrorAscii(clDoc.ErrorDesc());
		return false;
	}

	TiXmlElement* pRoot = clDoc.RootElement();
	if (pRoot == NULL)
	{
		p_refError = "CONFIG_NODE_MISSING: node=root";
		return false;
	}
	TiXmlElement* pIceRPCPush = _stricmp(pRoot->Value(), "IceRPCPush") == 0 ? pRoot : pRoot->FirstChildElement("IceRPCPush");
	if (pIceRPCPush == NULL)
	{
		p_refError = "CONFIG_NODE_MISSING: node=IceRPCPush";
		return false;
	}
	TiXmlElement* pIce = EnsureChildElement(pIceRPCPush, "Ice");
	if (pIce == NULL)
	{
		p_refError = "CONFIG_NODE_MISSING: node=IceRPCPush.Ice";
		return false;
	}

	// 底层 IceRPCPush 只能读取固定 Identity，这里按当前插件服务名生成运行时配置，避免多个插件共用 XML 时对象标识冲突。
	SetIceProperty(pIce, "Identity", p_szServiceName);
	if (!MakeRuntimeConfigPath(p_szServiceName, p_refRuntimePath, p_refError))
	{
		return false;
	}
	if (!clDoc.SaveFile(p_refRuntimePath.c_str()))
	{
		p_refError = "RUNTIME_CONFIG_SAVE_FAILED: failed to save runtime IceRPCPush.xml";
		return false;
	}
	return true;
}

// 异步调用上下文负责把 IceRPCPush 回调桥接为本库 C 回调。
struct ST_ASYNC_CONTEXT
{
	std::atomic_long lRef;                       // 引用计数，底层回调和发起线程各持有一次引用。
	std::atomic_bool bCallbackArrived;           // 底层回调是否已经到达，用于失败路径避免重复释放。
	PFN_CLOUD_NET_ASYNC pfnCallback;             // 上层异步结果回调函数。
	void* pParam;                                // 上层异步回调透传参数。
	ST_CONNECTION_CONTEXT_PTR refConnection;     // 异步期间持有连接上下文，防止 Disconnect 提前释放。
	std::atomic_bool bHoldConnection;            // 是否已经增加连接活动计数。

	ST_ASYNC_CONTEXT()
	{
		lRef = 2;
		bCallbackArrived = false;
		pfnCallback = NULL;
		pParam = NULL;
		refConnection.reset();
		bHoldConnection = false;
	}
};

// Binary 异步上下文独立保存请求标识、上层回调和连接引用，避免与 JSON 结果结构耦合。
struct ST_BINARY_ASYNC_CONTEXT
{
	std::atomic_long lRef;                           // 发起线程和底层回调各持有一个引用。
	PFN_CLOUD_NET_BINARY_ASYNC pfnCallback;          // 上层二进制异步结果回调。
	void* pParam;                                    // 上层透传参数，CloudNetDataApi 不拥有生命周期。
	long long lSynId;                                // 原请求序号，用于补入 CloudNetDataApi 结果。
	long long lFuncId;                               // 原业务功能号，用于补入 CloudNetDataApi 结果。
	ST_CONNECTION_CONTEXT_PTR refConnection;         // 异步完成前持有连接上下文。
	std::atomic_bool bHoldConnection;                // 是否持有连接活动调用计数。

	ST_BINARY_ASYNC_CONTEXT()
	{
		lRef = 2;
		pfnCallback = NULL;
		pParam = NULL;
		lSynId = 0;
		lFuncId = 0;
		refConnection.reset();
		bHoldConnection = false;
	}
};

// 编码异步上下文由提交线程和最终结果各持有一个引用。
// Ice 回调只安装底层拥有型结果并转交上层，最终释放可在任意回收线程执行。
struct ST_BINARY_ENCODED_ASYNC_CONTEXT
{
	std::atomic_long lRef;                           // 提交线程和编码结果所有者各持有一个引用。
	PFN_CLOUD_NET_BINARY_ENCODED_ASYNC pfnCallback; // 上层拥有型编码结果回调。
	void* pParam;                                    // 上层透传参数，CloudNetDataApi 不拥有生命周期。
	long long lSynId;                                // 原请求序号，用于补入编码结果。
	long long lFuncId;                               // 原功能号，用于补入编码结果。
	ST_CONNECTION_CONTEXT_PTR refConnection;         // Ice 回调到达前保持连接上下文有效。
	std::atomic_bool bHoldConnection;                // 是否持有连接活动调用计数。
	std::atomic_bool bResultReleased;                 // 防止回调异常路径重复释放拥有型结果。
	ST_BINARY_ENCODED_RESULT* pIceResult;             // 底层拥有型编码结果，由本上下文析构。
	ST_CLOUD_NET_BINARY_ENCODED_RESULT stResult;      // 对外结果，生命周期与本上下文一致。

	ST_BINARY_ENCODED_ASYNC_CONTEXT()
	{
		lRef = 2;
		pfnCallback = NULL;
		pParam = NULL;
		lSynId = 0;
		lFuncId = 0;
		refConnection.reset();
		bHoldConnection = false;
		bResultReleased = false;
		pIceResult = NULL;
		stResult.pInternalOwner = this;
	}

	~ST_BINARY_ENCODED_ASYNC_CONTEXT()
	{
		BinaryEncodedResultFree(pIceResult);
		pIceResult = NULL;
	}
};

bool RetainConnectionRef(const ST_CONNECTION_CONTEXT_PTR& p_refContext, HANDLE& p_refHandle)
{
	p_refHandle = NULL;
	if (!p_refContext)
	{
		return false;
	}
	std::lock_guard<std::mutex> clLock(p_refContext->clMutex);
	if (p_refContext->bClosing || p_refContext->hClient == NULL)
	{
		return false;
	}
	++p_refContext->iActiveCall;
	p_refHandle = p_refContext->hClient;
	return true;
}

void ReleaseConnectionRef(const ST_CONNECTION_CONTEXT_PTR& p_refContext)
{
	if (!p_refContext)
	{
		return;
	}
	std::lock_guard<std::mutex> clLock(p_refContext->clMutex);
	if (p_refContext->iActiveCall > 0)
	{
		--p_refContext->iActiveCall;
	}
	if (p_refContext->iActiveCall == 0)
	{
		p_refContext->clIdleCond.notify_all();
	}
}

void ReleaseAsyncContext(ST_ASYNC_CONTEXT* p_pContext)
{
	if (p_pContext == NULL)
	{
		return;
	}
	if (p_pContext->lRef.fetch_sub(1) == 1)
	{
		delete p_pContext;
	}
}

// 释放 Binary 异步上下文引用，最后一个持有者负责销毁对象。
void ReleaseBinaryAsyncContext(ST_BINARY_ASYNC_CONTEXT* p_pContext)
{
	if (p_pContext == NULL)
	{
		return;
	}
	if (p_pContext->lRef.fetch_sub(1) == 1)
	{
		delete p_pContext;
	}
}

// 临时执行引用保证上层可在回调内部立即释放结果，CloudNet 回调返回前上下文仍有效。
void RetainBinaryEncodedAsyncContext(
	ST_BINARY_ENCODED_ASYNC_CONTEXT* p_pContext)
{
	if (p_pContext != NULL)
	{
		p_pContext->lRef.fetch_add(1);
	}
}

void ReleaseBinaryEncodedAsyncContext(
	ST_BINARY_ENCODED_ASYNC_CONTEXT* p_pContext)
{
	if (p_pContext == NULL)
	{
		return;
	}
	if (p_pContext->lRef.fetch_sub(1) == 1)
	{
		delete p_pContext;
	}
}

// 连接调用守卫在调用底层 IceRPCPush 期间保持句柄有效，避免 Disconnect 并发释放。
class CConnectionCallGuard
{
public:
	CConnectionCallGuard(const ST_CONNECTION_CONTEXT_PTR& p_refContext)
	{
		m_refContext = p_refContext;
		m_hClient = NULL;
		m_bActive = RetainConnectionRef(m_refContext, m_hClient);
	}

	~CConnectionCallGuard()
	{
		if (m_bActive)
		{
			ReleaseConnectionRef(m_refContext);
		}
	}

	bool IsValid() const
	{
		return m_bActive && m_hClient != NULL;
	}

	HANDLE GetHandle() const
	{
		return m_hClient;
	}

private:
	CConnectionCallGuard(const CConnectionCallGuard&);
	CConnectionCallGuard& operator=(const CConnectionCallGuard&);

	ST_CONNECTION_CONTEXT_PTR m_refContext;                 // 当前调用持有的连接上下文，防止调用期间被 Disconnect 释放。
	HANDLE m_hClient;                                      // 本次调用使用的 IceRPCPush 客户端句柄，仅在守卫有效期内访问。
	bool m_bActive;                                        // 是否已经成功增加连接活动计数，析构时据此决定是否释放计数。
};

long SafeBufferLen(long p_lLen)
{
	if (p_lLen < 0)
	{
		return 0;
	}
	return p_lLen;
}

void SafeCopyError(char* p_szDest, size_t p_uDestLen, const char* p_szSrc)
{
	if (p_szDest == NULL || p_uDestLen == 0)
	{
		return;
	}
	p_szDest[0] = '\0';
	if (p_szSrc == NULL)
	{
		return;
	}
	std::string strSafeSrc = MakeApiErrorAscii(p_szSrc);
	strncpy_s(p_szDest, p_uDestLen, strSafeSrc.c_str(), _TRUNCATE);
}

std::string MakeIceRPCPushErrorDetail(HANDLE p_hHandle, const char* p_szPrefix)
{
	// 底层 IceRPCPush 已经捕获 Ice/std 异常，这里把其最后错误透传到业务网络库错误中。
	int iIceCode = IceRPCPushGetLastErrorCode(p_hHandle);
	const char* pIceDetail = IceRPCPushGetLastError(p_hHandle);
	if (pIceDetail == NULL || pIceDetail[0] == '\0')
	{
		pIceDetail = IceRPCPushGetErrorMsg(iIceCode);
	}
	std::string strDetail = p_szPrefix != NULL ? p_szPrefix : "ICE_RPC_PUSH_FAILED";
	strDetail += ": ice_code=";
	strDetail += std::to_string(iIceCode);
	strDetail += ", ice_detail=";
	strDetail += MakeApiErrorAscii(pIceDetail != NULL ? pIceDetail : "UNKNOWN_ERROR: IceRPCPush returned empty error detail");
	return strDetail;
}

bool CopyBuffer(ST_CLOUD_NET_BUFFER& p_refDest, const unsigned char* p_pSrc, long p_lLen)
{
	p_refDest.lLen = 0;
	p_refDest.pBuffer = NULL;
	long lLen = SafeBufferLen(p_lLen);
	if (lLen <= 0)
	{
		return true;
	}
	if (p_pSrc == NULL)
	{
		return false;
	}
	p_refDest.pBuffer = new (std::nothrow) unsigned char[static_cast<size_t>(lLen)];
	if (p_refDest.pBuffer == NULL)
	{
		return false;
	}
	memcpy_s(p_refDest.pBuffer, static_cast<size_t>(lLen), p_pSrc, static_cast<size_t>(lLen));
	p_refDest.lLen = lLen;
	return true;
}

void ReleaseBuffer(ST_CLOUD_NET_BUFFER& p_refBuffer)
{
	delete[] p_refBuffer.pBuffer;
	p_refBuffer.pBuffer = NULL;
	p_refBuffer.lLen = 0;
}

void ReleaseResult(ST_CLOUD_NET_RESULT* p_pResult)
{
	if (p_pResult == NULL)
	{
		return;
	}
	ReleaseBuffer(p_pResult->stLParam);
	ReleaseBuffer(p_pResult->stWParam);
	ReleaseBuffer(p_pResult->stJsonReq);
	delete p_pResult;
}

ST_CLOUD_NET_RESULT* CopyResult(const ST_JSON_M_RESULT_TOP* p_pSrc)
{
	if (p_pSrc == NULL)
	{
		return NULL;
	}
	ST_CLOUD_NET_RESULT* pResult = new (std::nothrow) ST_CLOUD_NET_RESULT;
	if (pResult == NULL)
	{
		return NULL;
	}
	memset(pResult, 0, sizeof(ST_CLOUD_NET_RESULT));
	pResult->lRetVal = p_pSrc->lRetVal;
	pResult->lParam = p_pSrc->lParam;
	pResult->wParam = p_pSrc->wParam;
	pResult->lSynId = p_pSrc->lSynId;
	pResult->lFuncId = p_pSrc->lFuncId;
	pResult->pParam = p_pSrc->pParam;
	SafeCopyError(pResult->szErrInfo, sizeof(pResult->szErrInfo), p_pSrc->szErrInfo);
	if (!CopyBuffer(pResult->stLParam, p_pSrc->stLParam.pBuffer, p_pSrc->stLParam.lLen) ||
		!CopyBuffer(pResult->stWParam, p_pSrc->stWParam.pBuffer, p_pSrc->stWParam.lLen) ||
		!CopyBuffer(pResult->stJsonReq, p_pSrc->stJsonReq.pBuffer, p_pSrc->stJsonReq.lLen))
	{
		ReleaseResult(pResult);
		return NULL;
	}
	return pResult;
}

// 深拷贝底层 Binary 缓冲区，确保 CloudNetDataApi 结果不依赖 IceRPCPush 内存。
bool CopyBinaryBuffer(ST_CLOUD_NET_BINARY_BUFFER& p_refDest, const unsigned char* p_pSrc, int p_iLen)
{
	p_refDest.iLen = 0;
	p_refDest.pBuffer = NULL;
	if (p_iLen < 0 || (p_iLen > 0 && p_pSrc == NULL))
	{
		return false;
	}
	if (p_iLen == 0)
	{
		return true;
	}
	p_refDest.pBuffer = new (std::nothrow) unsigned char[static_cast<size_t>(p_iLen)];
	if (p_refDest.pBuffer == NULL)
	{
		return false;
	}
	memcpy_s(p_refDest.pBuffer, static_cast<size_t>(p_iLen), p_pSrc, static_cast<size_t>(p_iLen));
	p_refDest.iLen = p_iLen;
	return true;
}

// 释放 CloudNetDataApi 自己分配的 Binary 结果及两个载荷缓冲区。
void ReleaseCloudBinaryResult(ST_CLOUD_NET_BINARY_RESULT* p_pResult)
{
	if (p_pResult == NULL)
	{
		return;
	}
	delete[] p_pResult->stPayload.pBuffer;
	p_pResult->stPayload.pBuffer = NULL;
	p_pResult->stPayload.iLen = 0;
	delete[] p_pResult->stExtra.pBuffer;
	p_pResult->stExtra.pBuffer = NULL;
	p_pResult->stExtra.iLen = 0;
	delete p_pResult;
}

// 把底层 Binary 结果转换为公共 C 结果，补齐原请求序号和功能号。
ST_CLOUD_NET_BINARY_RESULT* CopyBinaryResult(const ST_BINARY_RESULT* p_pSrc, long long p_lSynId, long long p_lFuncId)
{
	if (p_pSrc == NULL)
	{
		return NULL;
	}
	ST_CLOUD_NET_BINARY_RESULT* pResult = new (std::nothrow) ST_CLOUD_NET_BINARY_RESULT;
	if (pResult == NULL)
	{
		return NULL;
	}
	pResult->iVersion = p_pSrc->iVersion;
	pResult->lRetVal = p_pSrc->lRetVal;
	pResult->iErrorCode = p_pSrc->iErrorCode;
	pResult->lSynId = p_lSynId;
	pResult->lFuncId = p_lFuncId;
	pResult->lParam = p_pSrc->lParam;
	pResult->wParam = p_pSrc->wParam;
	pResult->pParam = p_pSrc->pParam;
	SafeCopyError(pResult->szErrInfo, sizeof(pResult->szErrInfo), p_pSrc->szErrInfo);
	if (!CopyBinaryBuffer(pResult->stPayload, p_pSrc->stPayload.pBuffer, p_pSrc->stPayload.lLen) ||
		!CopyBinaryBuffer(pResult->stExtra, p_pSrc->stExtra.pBuffer, p_pSrc->stExtra.lLen))
	{
		ReleaseCloudBinaryResult(pResult);
		return NULL;
	}
	return pResult;
}

// 映射底层 BinaryPayload 只读视图，不复制也不解压编码正文。
void FillCloudBinaryEncodedBuffer(
	const ST_BINARY_ENCODED_BUFFER& p_refSrc,
	ST_CLOUD_NET_BINARY_ENCODED_BUFFER& p_refDest)
{
	p_refDest.iVersion = p_refSrc.iVersion;
	p_refDest.iCompression = p_refSrc.iCompression;
	p_refDest.iRawSize = p_refSrc.iRawSize;
	p_refDest.iWireSize = p_refSrc.iWireSize;
	p_refDest.iMaxPayloadBytes = p_refSrc.iMaxPayloadBytes;
	p_refDest.pBuffer = p_refSrc.pBuffer;
}

// 底层拥有型结果安装到预分配上下文后，由上层回收线程决定最终释放时机。
void FillCloudBinaryEncodedResult(
	ST_BINARY_ENCODED_ASYNC_CONTEXT& p_refContext,
	ST_BINARY_ENCODED_RESULT& p_refSrc)
{
	ST_CLOUD_NET_BINARY_ENCODED_RESULT& refDest = p_refContext.stResult;
	refDest.iVersion = p_refSrc.iVersion;
	refDest.lRetVal = p_refSrc.lRetVal;
	refDest.iErrorCode = p_refSrc.iErrorCode;
	refDest.lSynId = p_refContext.lSynId;
	refDest.lFuncId = p_refContext.lFuncId;
	refDest.lParam = p_refSrc.lParam;
	refDest.wParam = p_refSrc.wParam;
	refDest.pParam = p_refContext.pParam;
	refDest.pInternalOwner = &p_refContext;
	SafeCopyError(refDest.szErrInfo, sizeof(refDest.szErrInfo),
		p_refSrc.szErrInfo);
	FillCloudBinaryEncodedBuffer(p_refSrc.stPayload, refDest.stPayload);
	FillCloudBinaryEncodedBuffer(p_refSrc.stExtra, refDest.stExtra);
}

// 释放编码结果持有的上下文引用；上下文析构时再调用 IceRPCPush 释放真实字节容器。
void ReleaseCloudBinaryEncodedResult(
	ST_CLOUD_NET_BINARY_ENCODED_RESULT* p_pResult)
{
	if (p_pResult == NULL || p_pResult->pInternalOwner == NULL)
	{
		return;
	}
	ST_BINARY_ENCODED_ASYNC_CONTEXT* pContext =
		static_cast<ST_BINARY_ENCODED_ASYNC_CONTEXT*>(
			p_pResult->pInternalOwner);
	if (pContext->bResultReleased.exchange(true))
	{
		return;
	}
	p_pResult->pInternalOwner = NULL;
	p_pResult->stPayload.pBuffer = NULL;
	p_pResult->stPayload.iWireSize = 0;
	p_pResult->stExtra.pBuffer = NULL;
	p_pResult->stExtra.iWireSize = 0;
	ReleaseBinaryEncodedAsyncContext(pContext);
}

// 校验公共 Binary 调用结构并映射到底层 IceRPCPush 结构，不复制请求正文。
bool FillIceBinaryCall(const ST_CLOUD_NET_BINARY_CALL* p_pSrc, ST_BINARY_CALL& p_refDest, std::string& p_refError)
{
	p_refError.clear();
	if (p_pSrc == NULL)
	{
		p_refError = "BINARY_PROTOCOL_INVALID: call is null";
		return false;
	}
	if (p_pSrc->iVersion != 1)
	{
		p_refError = "BINARY_PROTOCOL_VERSION_INVALID: expected=1, actual=" + std::to_string(p_pSrc->iVersion);
		return false;
	}
	if (p_pSrc->stPayload.iLen < 0 || p_pSrc->stExtra.iLen < 0 ||
		(p_pSrc->stPayload.iLen > 0 && p_pSrc->stPayload.pBuffer == NULL) ||
		(p_pSrc->stExtra.iLen > 0 && p_pSrc->stExtra.pBuffer == NULL))
	{
		p_refError = "BINARY_PROTOCOL_PAYLOAD_INVALID: payload length or pointer is invalid";
		return false;
	}
	p_refDest.iVersion = p_pSrc->iVersion;
	p_refDest.lSynId = p_pSrc->lSynId;
	p_refDest.lFuncId = p_pSrc->lFuncId;
	p_refDest.lRouteCode = p_pSrc->lRouteCode;
	p_refDest.lParam = p_pSrc->lParam;
	p_refDest.stPayload.lLen = p_pSrc->stPayload.iLen;
	p_refDest.stPayload.pBuffer = p_pSrc->stPayload.pBuffer;
	p_refDest.wParam = p_pSrc->wParam;
	p_refDest.stExtra.lLen = p_pSrc->stExtra.iLen;
	p_refDest.stExtra.pBuffer = p_pSrc->stExtra.pBuffer;
	return true;
}

// 把上层服务回调结果映射为底层应答；载荷只在 BinaryResponseData 调用期间被读取。
void FillIceBinaryResult(ST_BINARY_RESULT& p_refDest, const ST_CLOUD_NET_BINARY_RESULT& p_refSrc)
{
	p_refDest.iVersion = p_refSrc.iVersion;
	p_refDest.lRetVal = p_refSrc.lRetVal;
	p_refDest.iErrorCode = p_refSrc.iErrorCode;
	p_refDest.lParam = p_refSrc.lParam;
	p_refDest.stPayload.lLen = p_refSrc.stPayload.iLen;
	p_refDest.stPayload.pBuffer = p_refSrc.stPayload.pBuffer;
	p_refDest.wParam = p_refSrc.wParam;
	p_refDest.stExtra.lLen = p_refSrc.stExtra.iLen;
	p_refDest.stExtra.pBuffer = p_refSrc.stExtra.pBuffer;
	SafeCopyError(p_refDest.szErrInfo, sizeof(p_refDest.szErrInfo), p_refSrc.szErrInfo);
}

// 创建不包含底层回包句柄的公共服务端请求，保持业务实现与 IceRPCPush 隔离。
ST_CLOUD_NET_BINARY_REQUEST MakeCloudBinaryRequest(const ST_BINARY_REQUEST& p_refSrc, void* p_pParam)
{
	ST_CLOUD_NET_BINARY_REQUEST stRequest;
	stRequest.iVersion = p_refSrc.iVersion;
	stRequest.lSynId = p_refSrc.lSynId;
	stRequest.lFuncId = p_refSrc.lFuncId;
	stRequest.lRouteCode = p_refSrc.lRouteCode;
	stRequest.lParam = p_refSrc.lParam;
	stRequest.stPayload.iLen = p_refSrc.stPayload.lLen;
	stRequest.stPayload.pBuffer = p_refSrc.stPayload.pBuffer;
	stRequest.wParam = p_refSrc.wParam;
	stRequest.stExtra.iLen = p_refSrc.stExtra.lLen;
	stRequest.stExtra.pBuffer = p_refSrc.stExtra.pBuffer;
	stRequest.chMode = p_refSrc.chMode;
	stRequest.pParam = p_pParam;
	stRequest.hResponse = p_refSrc.hResponse;
	return stRequest;
}

void FillJsonResult(ST_JSON_M_RESULT_TOP& p_refDest, const ST_CLOUD_NET_RESULT* p_pSrc)
{
	memset(&p_refDest, 0, sizeof(ST_JSON_M_RESULT_TOP));
	if (p_pSrc == NULL)
	{
		return;
	}
	p_refDest.lRetVal = p_pSrc->lRetVal;
	p_refDest.lParam = p_pSrc->lParam;
	p_refDest.wParam = p_pSrc->wParam;
	p_refDest.lSynId = p_pSrc->lSynId;
	p_refDest.lFuncId = p_pSrc->lFuncId;
	p_refDest.pParam = p_pSrc->pParam;
	SafeCopyError(p_refDest.szErrInfo, sizeof(p_refDest.szErrInfo), p_pSrc->szErrInfo);
	p_refDest.stLParam.lLen = static_cast<int>(SafeBufferLen(p_pSrc->stLParam.lLen));
	p_refDest.stLParam.pBuffer = p_pSrc->stLParam.pBuffer;
	p_refDest.stWParam.lLen = static_cast<int>(SafeBufferLen(p_pSrc->stWParam.lLen));
	p_refDest.stWParam.pBuffer = p_pSrc->stWParam.pBuffer;
	p_refDest.stJsonReq.lLen = static_cast<int>(SafeBufferLen(p_pSrc->stJsonReq.lLen));
	p_refDest.stJsonReq.pBuffer = p_pSrc->stJsonReq.pBuffer;
}

ST_CLOUD_NET_REQUEST MakeRequest(short p_chMode, long long p_lSetCode, ST_JSON_M_RESULT_TOP* p_pResult, void* p_pParam)
{
	ST_CLOUD_NET_REQUEST stRequest;
	memset(&stRequest, 0, sizeof(ST_CLOUD_NET_REQUEST));
	if (p_pResult == NULL)
	{
		return stRequest;
	}
	stRequest.lSynId = p_pResult->lSynId;
	stRequest.lFuncId = p_pResult->lFuncId;
	stRequest.lSetCode = p_lSetCode;
	stRequest.lParam = p_pResult->lParam;
	stRequest.wParam = p_pResult->wParam;
	stRequest.chMode = p_chMode;
	stRequest.stJsonReq.lLen = p_pResult->stJsonReq.lLen;
	stRequest.stJsonReq.pBuffer = p_pResult->stJsonReq.pBuffer;
	stRequest.stLParam.lLen = p_pResult->stLParam.lLen;
	stRequest.stLParam.pBuffer = p_pResult->stLParam.pBuffer;
	stRequest.stWParam.lLen = p_pResult->stWParam.lLen;
	stRequest.stWParam.pBuffer = p_pResult->stWParam.pBuffer;
	stRequest.pParam = p_pParam;
	return stRequest;
}

unsigned long __stdcall AsyncCallback(void* p_pParam)
{
	ST_JSON_M_RESULT_LEVEL* pJsonResult = reinterpret_cast<ST_JSON_M_RESULT_LEVEL*>(p_pParam);
	ST_ASYNC_CONTEXT* pContext = NULL;
	ST_CONNECTION_CONTEXT_PTR refConnection;
	bool bHoldConnection = false;
	PFN_CLOUD_NET_ASYNC pfnCallback = NULL;
	void* pUserParam = NULL;
	if (pJsonResult != NULL)
	{
		pContext = reinterpret_cast<ST_ASYNC_CONTEXT*>(pJsonResult->pParam);
	}
	if (pContext != NULL)
	{
		pContext->bCallbackArrived = true;
		refConnection = pContext->refConnection;
		bHoldConnection = pContext->bHoldConnection.exchange(false);
		pfnCallback = pContext->pfnCallback;
		pUserParam = pContext->pParam;
	}

	ST_CLOUD_NET_RESULT* pResult = CopyResult(pJsonResult);
	if (pJsonResult != NULL)
	{
		IJsonMutiResultFree(pJsonResult);
	}
	if (bHoldConnection)
	{
		ReleaseConnectionRef(refConnection);
	}
	if (pfnCallback != NULL && pResult != NULL)
	{
		pResult->pParam = pUserParam;
		try
		{
			pfnCallback(pResult, pUserParam);
		}
		catch (const std::exception& ex)
		{
			SetCloudNetBoundaryThreadError(s_iCloudNetInternalError, MakeCloudNetStdExceptionDetail("AsyncCallback", ex), "AsyncCallback");
		}
		catch (...)
		{
			SetCloudNetBoundaryThreadError(s_iCloudNetInternalError, MakeCloudNetUnknownExceptionDetail("AsyncCallback"), "AsyncCallback");
		}
	}
	ReleaseResult(pResult);
	ReleaseAsyncContext(pContext);
	return 0;
}

// 接收 IceRPCPush Binary 异步结果，深拷贝后在回调期间交给上层并立即释放。
unsigned long __stdcall BinaryAsyncCallback(void* p_pParam)
{
	ST_BINARY_RESULT* pIceResult = reinterpret_cast<ST_BINARY_RESULT*>(p_pParam);
	ST_BINARY_ASYNC_CONTEXT* pContext = pIceResult != NULL ? reinterpret_cast<ST_BINARY_ASYNC_CONTEXT*>(pIceResult->pParam) : NULL;
	ST_CONNECTION_CONTEXT_PTR refConnection;
	bool bHoldConnection = false;
	PFN_CLOUD_NET_BINARY_ASYNC pfnCallback = NULL;
	void* pUserParam = NULL;
	long long lSynId = 0;
	long long lFuncId = 0;
	if (pContext != NULL)
	{
		refConnection = pContext->refConnection;
		bHoldConnection = pContext->bHoldConnection.exchange(false);
		pfnCallback = pContext->pfnCallback;
		pUserParam = pContext->pParam;
		lSynId = pContext->lSynId;
		lFuncId = pContext->lFuncId;
	}

	ST_CLOUD_NET_BINARY_RESULT* pResult = CopyBinaryResult(pIceResult, lSynId, lFuncId);
	if (pIceResult != NULL)
	{
		BinaryResultFree(pIceResult);
	}
	if (bHoldConnection)
	{
		ReleaseConnectionRef(refConnection);
	}

	ST_CLOUD_NET_BINARY_RESULT stMemoryError;
	const ST_CLOUD_NET_BINARY_RESULT* pCallbackResult = pResult;
	if (pCallbackResult == NULL)
	{
		stMemoryError.lRetVal = s_iCloudNetMemoryError;
		stMemoryError.iErrorCode = s_iCloudNetMemoryError;
		stMemoryError.lSynId = lSynId;
		stMemoryError.lFuncId = lFuncId;
		stMemoryError.pParam = pUserParam;
		SafeCopyError(stMemoryError.szErrInfo, sizeof(stMemoryError.szErrInfo), "MEMORY_ERROR: failed to copy binary async result");
		pCallbackResult = &stMemoryError;
	}
	else
	{
		pResult->pParam = pUserParam;
	}

	if (pfnCallback != NULL)
	{
		try
		{
			pfnCallback(pCallbackResult, pUserParam);
		}
		catch (const std::exception& ex)
		{
			SetCloudNetBoundaryThreadError(s_iCloudNetInternalError, MakeCloudNetStdExceptionDetail("BinaryAsyncCallback", ex), "BinaryAsyncCallback");
		}
		catch (...)
		{
			SetCloudNetBoundaryThreadError(s_iCloudNetInternalError, MakeCloudNetUnknownExceptionDetail("BinaryAsyncCallback"), "BinaryAsyncCallback");
		}
	}
	ReleaseCloudBinaryResult(pResult);
	ReleaseBinaryAsyncContext(pContext);
	return 0;
}

// 编码回调不解压也不复制正文，只把底层所有权安装到预分配上下文并交给上层。
unsigned long __stdcall BinaryEncodedAsyncCallback(void* p_pParam)
{
	ST_BINARY_ENCODED_RESULT* pIceResult =
		reinterpret_cast<ST_BINARY_ENCODED_RESULT*>(p_pParam);
	ST_BINARY_ENCODED_ASYNC_CONTEXT* pContext = pIceResult != NULL ?
		reinterpret_cast<ST_BINARY_ENCODED_ASYNC_CONTEXT*>(
			pIceResult->pParam) : NULL;
	if (pContext == NULL)
	{
		BinaryEncodedResultFree(pIceResult);
		SetCloudNetBoundaryThreadError(s_iCloudNetMemoryError,
			"MEMORY_ERROR: encoded Binary callback has no owning context",
			"BinaryEncodedAsyncCallback");
		return 0;
	}

	// 回调内部允许调用方立即释放结果，因此额外持有一次执行引用到本函数返回。
	RetainBinaryEncodedAsyncContext(pContext);
	pContext->pIceResult = pIceResult;
	FillCloudBinaryEncodedResult(*pContext, *pIceResult);
	ST_CONNECTION_CONTEXT_PTR refConnection = pContext->refConnection;
	if (pContext->bHoldConnection.exchange(false))
	{
		ReleaseConnectionRef(refConnection);
	}

	PFN_CLOUD_NET_BINARY_ENCODED_ASYNC pfnCallback =
		pContext->pfnCallback;
	if (pfnCallback != NULL)
	{
		try
		{
			pfnCallback(&pContext->stResult, pContext->pParam);
		}
		catch (const std::exception& ex)
		{
			SetCloudNetBoundaryThreadError(s_iCloudNetInternalError,
				MakeCloudNetStdExceptionDetail("BinaryEncodedAsyncCallback", ex),
				"BinaryEncodedAsyncCallback");
			ReleaseCloudBinaryEncodedResult(&pContext->stResult);
		}
		catch (...)
		{
			SetCloudNetBoundaryThreadError(s_iCloudNetInternalError,
				MakeCloudNetUnknownExceptionDetail(
					"BinaryEncodedAsyncCallback"),
				"BinaryEncodedAsyncCallback");
			ReleaseCloudBinaryEncodedResult(&pContext->stResult);
		}
	}
	else
	{
		ReleaseCloudBinaryEncodedResult(&pContext->stResult);
	}
	ReleaseBinaryEncodedAsyncContext(pContext);
	return 0;
}

void PushCallback(long long p_lReqNo, const char* p_pBuf, int p_iLen, void* p_pParam)
{
	ST_CONNECTION_CONTEXT* pContext = reinterpret_cast<ST_CONNECTION_CONTEXT*>(p_pParam);
	if (pContext == NULL)
	{
		return;
	}

	PFN_CLOUD_NET_PUSH pfnPush = NULL;
	void* pPushParam = NULL;
	{
		std::lock_guard<std::mutex> clLock(pContext->clMutex);
		if (pContext->bClosing)
		{
			return;
		}
		pfnPush = pContext->pfnPush;
		pPushParam = pContext->pPushParam;
	}
	if (pfnPush != NULL)
	{
		try
		{
			pfnPush(p_lReqNo, p_pBuf, p_iLen, pPushParam);
		}
		catch (const std::exception& ex)
		{
			SetCloudNetBoundaryThreadError(s_iCloudNetInternalError, MakeCloudNetStdExceptionDetail("PushCallback", ex), "PushCallback");
		}
		catch (...)
		{
			SetCloudNetBoundaryThreadError(s_iCloudNetInternalError, MakeCloudNetUnknownExceptionDetail("PushCallback"), "PushCallback");
		}
	}
}

bool LoadCloudNetConfigFile(const char* p_szXmlPath, const char* p_szPluginId, ST_CLOUD_NET_CONFIG& p_refConfig, std::string& p_refError)
{
	p_refConfig = ST_CLOUD_NET_CONFIG();
	p_refError.clear();
	if (p_szPluginId == NULL || p_szPluginId[0] == '\0')
	{
		p_refError = "CONFIG_PLUGIN_ID_EMPTY: plugin id is empty";
		return false;
	}
	std::string strXmlPath = (p_szXmlPath != NULL && p_szXmlPath[0] != '\0') ? p_szXmlPath : s_szDefaultConfigFile;
	TiXmlDocument clDoc;
	if (!clDoc.LoadFile(strXmlPath.c_str(), TIXML_ENCODING_UTF8))
	{
		p_refError = std::string("CONFIG_LOAD_FAILED: path=") + MakeApiErrorAscii(strXmlPath) + ", detail=" + MakeApiErrorAscii(clDoc.ErrorDesc());
		return false;
	}
	const TiXmlElement* pRoot = clDoc.RootElement();
	if (pRoot == NULL || _stricmp(pRoot->Value(), "CloudNetDataApi") != 0)
	{
		p_refError = "CONFIG_ROOT_INVALID: root node must be CloudNetDataApi";
		return false;
	}
	const TiXmlElement* pCloudNet = pRoot->FirstChildElement("CloudNet");
	if (pCloudNet == NULL)
	{
		p_refError = "CONFIG_NODE_MISSING: node=CloudNet";
		return false;
	}

	const TiXmlElement* pLocal = NULL;
	for (const TiXmlElement* pNode = pCloudNet->FirstChildElement("Local"); pNode != NULL; pNode = pNode->NextSiblingElement("Local"))
	{
		std::string strId = GetAttrText(pNode, "id", "");
		if (strId == p_szPluginId)
		{
			pLocal = pNode;
			break;
		}
		if (pLocal == NULL)
		{
			pLocal = pNode;
		}
	}
	if (pLocal == NULL)
	{
		p_refError = "CONFIG_NODE_MISSING: node=CloudNet.Local";
		return false;
	}

	p_refConfig.strXmlPath = strXmlPath;
	p_refConfig.strPluginId = p_szPluginId;
	p_refConfig.stLog.strPath = "log/" + p_refConfig.strPluginId;
	p_refConfig.stLog.strName = p_refConfig.strPluginId + "CloudNet";
	const TiXmlElement* pLog = pCloudNet->FirstChildElement("Log");
	p_refConfig.stLog.strPath = ReplacePluginId(
		GetAttrText(pLog, "path", p_refConfig.stLog.strPath.c_str()), p_refConfig.strPluginId);
	p_refConfig.stLog.strName = ReplacePluginId(
		GetAttrText(pLog, "name", p_refConfig.stLog.strName.c_str()), p_refConfig.strPluginId);
	p_refConfig.stLog.strLevel = GetAttrText(pLog, "level", "info");
	std::transform(p_refConfig.stLog.strLevel.begin(), p_refConfig.stLog.strLevel.end(),
		p_refConfig.stLog.strLevel.begin(),
		[](unsigned char p_uValue) { return static_cast<char>(std::tolower(p_uValue)); });
	if (p_refConfig.stLog.strLevel != "debug" && p_refConfig.stLog.strLevel != "info" &&
		p_refConfig.stLog.strLevel != "warn" && p_refConfig.stLog.strLevel != "warning" &&
		p_refConfig.stLog.strLevel != "error" && p_refConfig.stLog.strLevel != "off")
	{
		p_refError = "CONFIG_LOG_LEVEL_INVALID: CloudNet.Log.level must be debug|info|warn|error|off";
		return false;
	}
	p_refConfig.stLocal.strId = GetAttrText(pLocal, "id", "");
	p_refConfig.stLocal.strService = GetAttrText(pLocal, "service", "");
	p_refConfig.stLocal.bStartService = GetAttrBool(pLocal, "startService", true);
	p_refConfig.stLocal.iSnappy = GetAttrInt(pLocal, "snappy", 1);
	if (p_refConfig.stLocal.strId.empty())
	{
		p_refError = "CONFIG_LOCAL_ID_EMPTY: Local.id is empty";
		return false;
	}
	if (p_refConfig.stLocal.strId != p_refConfig.strPluginId)
	{
		p_refError = "CONFIG_LOCAL_ID_MISMATCH: Local.id does not match LoadConfig plugin id";
		return false;
	}
	if (p_refConfig.stLocal.strService.empty())
	{
		p_refError = "CONFIG_LOCAL_SERVICE_EMPTY: Local.service is empty";
		return false;
	}

	const TiXmlElement* pRuntime = pCloudNet->FirstChildElement("Runtime");
	p_refConfig.stRuntime.iReconnectIntervalMs = GetAttrInt(pRuntime, "reconnectIntervalMs", 5000);
	p_refConfig.stRuntime.iRenewIntervalSec = GetAttrInt(pRuntime, "renewIntervalSec", 240);
	p_refConfig.stRuntime.iThreadPool = GetAttrInt(pRuntime, "threadPool", 0);
	// 远端连接和订阅失败按低频恢复，避免依赖故障时形成一秒级空转和重复日志。
	if (p_refConfig.stRuntime.iReconnectIntervalMs < 5000)
	{
		p_refConfig.stRuntime.iReconnectIntervalMs = 5000;
	}
	else if (p_refConfig.stRuntime.iReconnectIntervalMs > 60000)
	{
		p_refConfig.stRuntime.iReconnectIntervalMs = 60000;
	}
	if (p_refConfig.stRuntime.iRenewIntervalSec <= 0)
	{
		p_refConfig.stRuntime.iRenewIntervalSec = 240;
	}
	if (p_refConfig.stRuntime.iThreadPool < 0)
	{
		p_refConfig.stRuntime.iThreadPool = 0;
	}

	const TiXmlElement* pServices = pCloudNet->FirstChildElement("Services");
	if (pServices != NULL)
	{
		for (const TiXmlElement* pService = pServices->FirstChildElement("Service"); pService != NULL; pService = pService->NextSiblingElement("Service"))
		{
			if (!IsOwnerMatched(pService, p_refConfig.strPluginId))
			{
				continue;
			}
			ST_SERVICE_CONFIG stService;
			stService.strId = GetAttrText(pService, "id", "");
			stService.strProxy = GetAttrText(pService, "proxy", "");
			stService.bAutoConnect = GetAttrBool(pService, "autoConnect", true);
			if (stService.strId.empty())
			{
				p_refError = "CONFIG_SERVICE_ID_EMPTY: Service.id is empty";
				return false;
			}
			if (stService.strProxy.empty())
			{
				p_refError = "CONFIG_SERVICE_PROXY_EMPTY: Service.proxy is empty";
				return false;
			}
			if (stService.strId != p_refConfig.stLocal.strId)
			{
				p_refConfig.aService.push_back(stService);
			}
		}
	}

	const TiXmlElement* pSubscriptions = pCloudNet->FirstChildElement("Subscriptions");
	if (pSubscriptions != NULL)
	{
		for (const TiXmlElement* pSubscribe = pSubscriptions->FirstChildElement("Subscribe"); pSubscribe != NULL; pSubscribe = pSubscribe->NextSiblingElement("Subscribe"))
		{
			if (!IsOwnerMatched(pSubscribe, p_refConfig.strPluginId))
			{
				continue;
			}
			ST_SUBSCRIPTION_CONFIG stSub;
			stSub.strTarget = GetAttrText(pSubscribe, "target", "");
			stSub.strPluginId = ReplacePluginId(GetAttrText(pSubscribe, "pluginId", "${PluginId}"), p_refConfig.strPluginId);
			stSub.strSubInfo = ReplacePluginId(GetAttrText(pSubscribe, "subInfo", ""), p_refConfig.strPluginId);
			stSub.bEnable = GetAttrBool(pSubscribe, "enable", true);
			if (stSub.bEnable && stSub.strTarget.empty())
			{
				p_refError = "CONFIG_SUB_TARGET_EMPTY: Subscribe.target is empty";
				return false;
			}
			if (stSub.strTarget == p_refConfig.stLocal.strId)
			{
				continue;
			}
			if (stSub.bEnable && stSub.strPluginId.empty())
			{
				p_refError = "CONFIG_SUB_PLUGIN_EMPTY: Subscribe.pluginId is empty";
				return false;
			}
			p_refConfig.aSubscription.push_back(stSub);
		}
	}
	p_refConfig.bLoaded = true;
	return true;
}

}

// CloudNetDataApi 内部实现类，公共头只暴露不透明句柄。
class CCloudNetDataApi
{
public:
	// 初始化业务网络库实例状态，句柄和回调均置空，后台线程默认未启动。
	CCloudNetDataApi()
	{
		m_hServer = NULL;
		m_hServerSem = NULL;
		m_pfnRequest = NULL;
		m_pRequestParam = NULL;
		m_pfnBinaryRequest = NULL;
		m_pfnBinaryRequestEx = NULL;
		m_pfnBinaryPriorityClassifier = NULL;
		m_pBinaryRequestParam = NULL;
		m_bClosing = false;
		m_bNetworkStop = false;
		m_bNetworkStarted = false;
		m_iActiveServiceCall = 0;
		m_pfnNetworkPush = NULL;
		m_pNetworkParam = NULL;
		m_iLastErrorCode = s_iCloudNetOk;
		m_strLastError = GetCloudNetErrorMsg(s_iCloudNetOk);
	}

	// 析构时停止组网线程并释放所有底层 IceRPCPush 连接，防止 DLL 卸载后仍有回调。
	~CCloudNetDataApi()
	{
		m_bClosing = true;
		StopNetwork();
		std::map<std::string, ST_CONNECTION_CONTEXT_PTR> mapConnection;
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			mapConnection.swap(m_mapConnection);
		}
		for (std::map<std::string, ST_CONNECTION_CONTEXT_PTR>::iterator it = mapConnection.begin(); it != mapConnection.end(); ++it)
		{
			CloseConnection(it->second);
		}
	}

	// 启动本地 IceRPCPush 服务端，并登记上层请求回调。
	int StartService(const char* p_szXmlPath, const char* p_szServiceName, int p_iSnappyCompress, PFN_CLOUD_NET_REQUEST p_pfnCallback, void* p_pParam)
	{
		if (p_szXmlPath == NULL || p_szServiceName == NULL || p_pfnCallback == NULL)
		{
			SetLastError("INVALID_PARAM: StartService requires xml path, service name and request callback");
			return s_iCloudNetInvalidParam;
		}
		StopService();
		std::string strRuntimeConfigPath;
		std::string strConfigError;
		if (!BuildServerRuntimeConfig(p_szXmlPath, p_szServiceName, strRuntimeConfigPath, strConfigError))
		{
			SetLastError(strConfigError.c_str());
			return s_iCloudNetInvalidParam;
		}
		const char* pServerXmlPath = strRuntimeConfigPath.empty() ? p_szXmlPath : strRuntimeConfigPath.c_str();
		HANDLE hSem = NULL;
		HANDLE hServer = CreateJsonICEServer(pServerXmlPath, p_szServiceName, hSem, p_iSnappyCompress != 0);
		if (hServer == NULL)
		{
			if (!strRuntimeConfigPath.empty())
			{
				std::remove(strRuntimeConfigPath.c_str());
			}
			SetLastError(MakeIceRPCPushErrorDetail(NULL, "ICE_SERVER_CREATE_FAILED: CreateJsonICEServer returned null").c_str());
			return s_iCloudNetCreateFailed;
		}
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			m_hServer = hServer;
			m_hServerSem = hSem;
			m_pfnRequest = p_pfnCallback;
			m_pRequestParam = p_pParam;
			m_pfnBinaryRequest = NULL;
			m_pfnBinaryRequestEx = NULL;
			m_pfnBinaryPriorityClassifier = NULL;
			m_pBinaryRequestParam = NULL;
			m_strServerRuntimeConfigPath = strRuntimeConfigPath;
		}
		RegServerCallBackFunc(hServer, &CCloudNetDataApi::ServerCallback, this);
		SetLastError(s_iCloudNetOk, GetCloudNetErrorMsg(s_iCloudNetOk));
		return s_iCloudNetOk;
	}

	// 启动二进制 Ice 服务端，服务端回调桥负责隐藏 IceRPCPush 请求和回包句柄。
	int StartBinaryService(const char* p_szXmlPath, const char* p_szServiceName, int p_iSnappyCompress, PFN_CLOUD_NET_BINARY_REQUEST p_pfnCallback, void* p_pParam)
	{
		if (p_szXmlPath == NULL || p_szServiceName == NULL || p_pfnCallback == NULL)
		{
			SetLastError("INVALID_PARAM: StartBinaryService requires xml path, service name and binary request callback");
			return s_iCloudNetInvalidParam;
		}
		StopService();
		std::string strRuntimeConfigPath;
		std::string strConfigError;
		if (!BuildServerRuntimeConfig(p_szXmlPath, p_szServiceName, strRuntimeConfigPath, strConfigError))
		{
			SetLastError(strConfigError.c_str());
			return s_iCloudNetConfigError;
		}
		const char* pServerXmlPath = strRuntimeConfigPath.empty() ? p_szXmlPath : strRuntimeConfigPath.c_str();
		HANDLE hSem = NULL;
		HANDLE hServer = CreateJsonICEServer(pServerXmlPath, p_szServiceName, hSem, p_iSnappyCompress != 0);
		if (hServer == NULL)
		{
			if (!strRuntimeConfigPath.empty())
			{
				std::remove(strRuntimeConfigPath.c_str());
			}
			SetLastError(MakeIceRPCPushErrorDetail(NULL, "ICE_SERVER_CREATE_FAILED: binary CreateJsonICEServer returned null").c_str());
			return s_iCloudNetCreateFailed;
		}
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			m_hServer = hServer;
			m_hServerSem = hSem;
			m_pfnRequest = NULL;
			m_pRequestParam = NULL;
			m_pfnBinaryRequest = p_pfnCallback;
			m_pfnBinaryRequestEx = NULL;
			m_pfnBinaryPriorityClassifier = NULL;
			m_pBinaryRequestParam = p_pParam;
			m_strServerRuntimeConfigPath = strRuntimeConfigPath;
		}
		RegBinaryServerCallBackFunc(hServer, &CCloudNetDataApi::BinaryServerCallback, this);
		if (IceRPCPushGetLastErrorCode(hServer) != 0)
		{
			std::string strError = MakeIceRPCPushErrorDetail(hServer, "BINARY_SERVER_CALLBACK_REGISTER_FAILED: RegBinaryServerCallBackFunc failed");
			StopService();
			SetLastError(s_iCloudNetBinaryCallFailed, strError.c_str(), __FUNCTION__);
			return s_iCloudNetBinaryCallFailed;
		}
		SetLastError(s_iCloudNetOk, GetCloudNetErrorMsg(s_iCloudNetOk));
		return s_iCloudNetOk;
	}

	// 启动支持延迟应答的 Binary 服务，控制分类器由业务层注入，底层不解释 FuncId。
	int StartBinaryServiceEx(const char* p_szXmlPath, const char* p_szServiceName,
		int p_iSnappyCompress, PFN_CLOUD_NET_BINARY_REQUEST_EX p_pfnCallback,
		PFN_CLOUD_NET_BINARY_PRIORITY_CLASSIFIER p_pfnPriorityClassifier,
		void* p_pParam)
	{
		if (p_szXmlPath == NULL || p_szServiceName == NULL ||
			p_pfnCallback == NULL)
		{
			SetLastError("INVALID_PARAM: StartBinaryServiceEx requires xml path, service name and binary request callback");
			return s_iCloudNetInvalidParam;
		}
		StopService();
		std::string strRuntimeConfigPath;
		std::string strConfigError;
		if (!BuildServerRuntimeConfig(p_szXmlPath, p_szServiceName,
			strRuntimeConfigPath, strConfigError))
		{
			SetLastError(strConfigError.c_str());
			return s_iCloudNetConfigError;
		}
		const char* pServerXmlPath = strRuntimeConfigPath.empty() ?
			p_szXmlPath : strRuntimeConfigPath.c_str();
		HANDLE hSem = NULL;
		HANDLE hServer = CreateJsonICEServer(pServerXmlPath, p_szServiceName,
			hSem, p_iSnappyCompress != 0);
		if (hServer == NULL)
		{
			if (!strRuntimeConfigPath.empty())
			{
				std::remove(strRuntimeConfigPath.c_str());
			}
			SetLastError(MakeIceRPCPushErrorDetail(NULL,
				"ICE_SERVER_CREATE_FAILED: binary Ex CreateJsonICEServer returned null").c_str());
			return s_iCloudNetCreateFailed;
		}
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			m_hServer = hServer;
			m_hServerSem = hSem;
			m_pfnRequest = NULL;
			m_pRequestParam = NULL;
			m_pfnBinaryRequest = NULL;
			m_pfnBinaryRequestEx = p_pfnCallback;
			m_pfnBinaryPriorityClassifier = p_pfnPriorityClassifier;
			m_pBinaryRequestParam = p_pParam;
			m_strServerRuntimeConfigPath = strRuntimeConfigPath;
		}
		RegBinaryServerCallBackFuncEx(hServer,
			&CCloudNetDataApi::BinaryServerCallbackEx,
			p_pfnPriorityClassifier != NULL ?
			&CCloudNetDataApi::BinaryPriorityClassifier : NULL, this);
		if (IceRPCPushGetLastErrorCode(hServer) != 0)
		{
			std::string strError = MakeIceRPCPushErrorDetail(hServer,
				"BINARY_SERVER_CALLBACK_REGISTER_FAILED: RegBinaryServerCallBackFuncEx failed");
			StopService();
			SetLastError(s_iCloudNetBinaryCallFailed, strError.c_str(), __FUNCTION__);
			return s_iCloudNetBinaryCallFailed;
		}
		SetLastError(s_iCloudNetOk, GetCloudNetErrorMsg(s_iCloudNetOk));
		return s_iCloudNetOk;
	}

	// 停止本地服务端，等待正在执行的服务端回调结束后再释放底层句柄。
	int StopService()
	{
		HANDLE hServer = NULL;
		std::string strRuntimeConfigPath;
		{
			std::unique_lock<std::mutex> clLock(m_clMutex);
			hServer = m_hServer;
			m_hServer = NULL;
			m_hServerSem = NULL;
			m_pfnRequest = NULL;
			m_pRequestParam = NULL;
			m_pfnBinaryRequest = NULL;
			m_pfnBinaryRequestEx = NULL;
			m_pfnBinaryPriorityClassifier = NULL;
			m_pBinaryRequestParam = NULL;
			strRuntimeConfigPath.swap(m_strServerRuntimeConfigPath);
			while (m_iActiveServiceCall > 0)
			{
				m_clServiceIdleCond.wait(clLock);
			}
		}
		if (hServer != NULL)
		{
			DeleteJsonICERPC(hServer);
		}
		if (!strRuntimeConfigPath.empty())
		{
			std::remove(strRuntimeConfigPath.c_str());
		}
		return s_iCloudNetOk;
	}

	// 建立到远端服务的客户端连接，并写入连接表供 RPC、PUT 和推送注册复用。
	int Connect(const char* p_szXmlPath, const char* p_szProxyName, const char* p_szConnName, int p_iThreadPool)
	{
		if (p_szXmlPath == NULL || p_szProxyName == NULL || p_szConnName == NULL || p_szConnName[0] == '\0')
		{
			SetLastError("INVALID_PARAM: Connect requires xml path, proxy name and connection name");
			return s_iCloudNetInvalidParam;
		}
		RemoveConnectionIfExists(p_szConnName);
		HANDLE hSem = NULL;
		HANDLE hClient = CreateJsonICEClient(p_szXmlPath, p_szProxyName, hSem, p_iThreadPool);
		if (hClient == NULL)
		{
			SetLastError(MakeIceRPCPushErrorDetail(NULL, "ICE_CLIENT_CREATE_FAILED: CreateJsonICEClient returned null").c_str());
			return s_iCloudNetCreateFailed;
		}

		ST_CONNECTION_CONTEXT_PTR pContext(new (std::nothrow) ST_CONNECTION_CONTEXT);
		if (!pContext)
		{
			DeleteJsonICERPC(hClient);
			SetLastError("MEMORY_ERROR: connection context allocation failed");
			return s_iCloudNetCreateFailed;
		}
		pContext->hClient = hClient;
		pContext->strConnName = p_szConnName;
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			m_mapConnection[p_szConnName] = pContext;
		}
		SetLastError(s_iCloudNetOk, GetCloudNetErrorMsg(s_iCloudNetOk));
		return s_iCloudNetOk;
	}

	// 按连接名断开远端服务，等待连接上已有调用结束后释放底层客户端句柄。
	int Disconnect(const char* p_szConnName)
	{
		if (p_szConnName == NULL || p_szConnName[0] == '\0')
		{
			SetLastError("INVALID_PARAM: Disconnect requires connection name");
			return s_iCloudNetInvalidParam;
		}
		ST_CONNECTION_CONTEXT_PTR pContext;
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			std::map<std::string, ST_CONNECTION_CONTEXT_PTR>::iterator it = m_mapConnection.find(p_szConnName);
			if (it == m_mapConnection.end())
			{
				SetLastError("CONNECTION_NOT_FOUND: connection does not exist");
				return s_iCloudNetNotFound;
			}
			pContext = it->second;
			m_mapConnection.erase(it);
		}
		CloseConnection(pContext);
		RetireConnection(pContext);
		SetLastError(s_iCloudNetOk, GetCloudNetErrorMsg(s_iCloudNetOk));
		return s_iCloudNetOk;
	}

	// 向目标服务注册推送订阅，并保存上层推送回调用于底层回调桥接。
	int RegisterPush(const char* p_szConnName, const char* p_szPluginId, const char* p_szSubInfo, PFN_CLOUD_NET_PUSH p_pfnCallback, void* p_pParam)
	{
		if (p_szPluginId == NULL || p_pfnCallback == NULL)
		{
			SetLastError("INVALID_PARAM: RegisterPush requires conn name, plugin id and callback");
			return s_iCloudNetInvalidParam;
		}
		ST_CONNECTION_CONTEXT_PTR pContext = FindConnection(p_szConnName);
		if (!pContext)
		{
			return s_iCloudNetNotFound;
		}
		CConnectionCallGuard clCallGuard(pContext);
		if (!clCallGuard.IsValid())
		{
			SetLastError("CONNECTION_CLOSING: connection is closing or closed");
			return s_iCloudNetNotFound;
		}
		{
			std::lock_guard<std::mutex> clLock(pContext->clMutex);
			pContext->pfnPush = p_pfnCallback;
			pContext->pPushParam = p_pParam;
		}
		const char* pRet = RegisterJsonICEClient2(clCallGuard.GetHandle(), p_szPluginId, p_szSubInfo != NULL ? p_szSubInfo : "", PushCallback, 1, pContext.get());
		if (!IsPushRegistrationResultValid(pRet))
		{
			std::lock_guard<std::mutex> clLock(pContext->clMutex);
			pContext->pfnPush = NULL;
			pContext->pPushParam = NULL;
			std::string strDetail =
				"PUSH_REGISTER_FAILED: RegisterJsonICEClient2 returned empty result,connection=" +
				std::string(p_szConnName != NULL ? p_szConnName : "") +
				",pluginId=" + p_szPluginId + "," +
				MakeIceRPCPushErrorDetail(clCallGuard.GetHandle(), "ice_detail");
			SetLastError(strDetail.c_str());
			return s_iCloudNetCallFailed;
		}
		pContext->strRegisterRet = pRet;
		SetLastError(s_iCloudNetOk, GetCloudNetErrorMsg(s_iCloudNetOk));
		return s_iCloudNetOk;
	}

	// 向目标服务注销推送订阅，并清理当前连接保存的推送回调信息。
	int UnregisterPush(const char* p_szConnName, const char* p_szPluginId, const char* p_szSubInfo)
	{
		if (p_szPluginId == NULL)
		{
			SetLastError("INVALID_PARAM: UnregisterPush requires conn name and plugin id");
			return s_iCloudNetInvalidParam;
		}
		ST_CONNECTION_CONTEXT_PTR pContext = FindConnection(p_szConnName);
		if (!pContext)
		{
			return s_iCloudNetNotFound;
		}
		CConnectionCallGuard clCallGuard(pContext);
		if (!clCallGuard.IsValid())
		{
			SetLastError("CONNECTION_CLOSING: connection is closing or closed");
			return s_iCloudNetNotFound;
		}
		const char* pRet = RegisterJsonICEClient2(clCallGuard.GetHandle(), p_szPluginId, p_szSubInfo != NULL ? p_szSubInfo : "", PushCallback, 0, pContext.get());
		if (!IsPushRegistrationResultValid(pRet))
		{
			SetLastError(MakeIceRPCPushErrorDetail(clCallGuard.GetHandle(), "PUSH_UNREGISTER_FAILED: RegisterJsonICEClient2 unregister returned empty result").c_str());
			return s_iCloudNetCallFailed;
		}
		{
			std::lock_guard<std::mutex> clLock(pContext->clMutex);
			pContext->pfnPush = NULL;
			pContext->pPushParam = NULL;
			pContext->strRegisterRet = pRet;
		}
		SetLastError(s_iCloudNetOk, GetCloudNetErrorMsg(s_iCloudNetOk));
		return s_iCloudNetOk;
	}

	// 执行同步 RPC 调用，并把 IceRPCPush 结果深拷贝为本库结果对象。
	ST_CLOUD_NET_RESULT* CallSync(const char* p_szConnName, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lJsonLen)
	{
		ST_CONNECTION_CONTEXT_PTR pContext = FindConnection(p_szConnName);
		if (!pContext)
		{
			return NULL;
		}
		CConnectionCallGuard clCallGuard(pContext);
		if (!clCallGuard.IsValid())
		{
			SetLastError("CONNECTION_CLOSING: connection is closing or closed");
			return NULL;
		}
		ST_JSON_M_RESULT_LEVEL* pJsonResult = JsonBinClientRPC(clCallGuard.GetHandle(), p_lSynId, p_lFuncId, p_lSetCode, p_szJsonReq != NULL ? p_szJsonReq : "", SafeBufferLen(p_lJsonLen));
		ST_CLOUD_NET_RESULT* pResult = CopyResult(pJsonResult);
		if (pJsonResult != NULL)
		{
			IJsonMutiResultFree(pJsonResult);
		}
		SetLastError(pResult != NULL ? "" : MakeIceRPCPushErrorDetail(clCallGuard.GetHandle(), "RPC_SYNC_FAILED: JsonBinClientRPC returned null").c_str());
		return pResult;
	}

	// 执行异步 RPC 调用，内部用引用计数上下文保护回调和连接生命周期。
	long long CallAsync(const char* p_szConnName, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lJsonLen, PFN_CLOUD_NET_ASYNC p_pfnCallback, void* p_pParam)
	{
		if (p_pfnCallback == NULL)
		{
			SetLastError("INVALID_PARAM: CallAsync callback is null");
			return s_iCloudNetInvalidParam;
		}
		ST_CONNECTION_CONTEXT_PTR pContext = FindConnection(p_szConnName);
		if (!pContext)
		{
			return s_iCloudNetNotFound;
		}
		ST_ASYNC_CONTEXT* pAsync = CreateAsyncContext(p_pfnCallback, p_pParam);
		if (pAsync == NULL)
		{
			return s_iCloudNetCreateFailed;
		}
		HANDLE hClient = NULL;
		if (!RetainConnectionRef(pContext, hClient))
		{
			ReleaseAsyncContext(pAsync);
			ReleaseAsyncContext(pAsync);
			SetLastError("CONNECTION_CLOSING: connection is closing or closed");
			return s_iCloudNetNotFound;
		}
		pAsync->refConnection = pContext;
		pAsync->bHoldConnection = true;
		long long lRet = JsonBinClientRPCAsync(hClient, p_lSynId, p_lFuncId, p_lSetCode, p_szJsonReq != NULL ? p_szJsonReq : "", SafeBufferLen(p_lJsonLen), AsyncCallback, pAsync);
		if (lRet <= 0)
		{
			if (pAsync->bHoldConnection.exchange(false))
			{
				ReleaseConnectionRef(pContext);
				ReleaseAsyncContext(pAsync);
			}
			ReleaseAsyncContext(pAsync);
			SetLastError(MakeIceRPCPushErrorDetail(hClient, "RPC_ASYNC_FAILED: JsonBinClientRPCAsync returned failure").c_str());
		}
		else
		{
			ReleaseAsyncContext(pAsync);
			SetLastError(s_iCloudNetOk, GetCloudNetErrorMsg(s_iCloudNetOk));
		}
		return lRet;
	}

	// 执行同步 PUT 调用，携带调用方提供的二进制返回数据结构。
	ST_CLOUD_NET_RESULT* PutSync(const char* p_szConnName, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lJsonLen, const ST_CLOUD_NET_RESULT* p_pPutData)
	{
		ST_CONNECTION_CONTEXT_PTR pContext = FindConnection(p_szConnName);
		if (!pContext)
		{
			return NULL;
		}
		CConnectionCallGuard clCallGuard(pContext);
		if (!clCallGuard.IsValid())
		{
			SetLastError("CONNECTION_CLOSING: connection is closing or closed");
			return NULL;
		}
		ST_JSON_M_RESULT_TOP stPutData;
		FillJsonResult(stPutData, p_pPutData);
		ST_JSON_M_RESULT_LEVEL* pJsonResult = JsonBinClientPUT(clCallGuard.GetHandle(), p_lSynId, p_lFuncId, p_lSetCode, p_szJsonReq != NULL ? p_szJsonReq : "", SafeBufferLen(p_lJsonLen), &stPutData);
		ST_CLOUD_NET_RESULT* pResult = CopyResult(pJsonResult);
		if (pJsonResult != NULL)
		{
			IJsonMutiResultFree(pJsonResult);
		}
		SetLastError(pResult != NULL ? "" : MakeIceRPCPushErrorDetail(clCallGuard.GetHandle(), "PUT_SYNC_FAILED: JsonBinClientPUT returned null").c_str());
		return pResult;
	}

	// 执行异步 PUT 调用，失败时立即释放异步上下文避免泄漏。
	long long PutAsync(const char* p_szConnName, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lJsonLen, const ST_CLOUD_NET_RESULT* p_pPutData, PFN_CLOUD_NET_ASYNC p_pfnCallback, void* p_pParam)
	{
		if (p_pfnCallback == NULL)
		{
			SetLastError("INVALID_PARAM: PutAsync callback is null");
			return s_iCloudNetInvalidParam;
		}
		ST_CONNECTION_CONTEXT_PTR pContext = FindConnection(p_szConnName);
		if (!pContext)
		{
			return s_iCloudNetNotFound;
		}
		ST_ASYNC_CONTEXT* pAsync = CreateAsyncContext(p_pfnCallback, p_pParam);
		if (pAsync == NULL)
		{
			return s_iCloudNetCreateFailed;
		}
		HANDLE hClient = NULL;
		if (!RetainConnectionRef(pContext, hClient))
		{
			ReleaseAsyncContext(pAsync);
			ReleaseAsyncContext(pAsync);
			SetLastError("CONNECTION_CLOSING: connection is closing or closed");
			return s_iCloudNetNotFound;
		}
		pAsync->refConnection = pContext;
		pAsync->bHoldConnection = true;
		ST_JSON_M_RESULT_TOP stPutData;
		FillJsonResult(stPutData, p_pPutData);
		long long lRet = JsonBinClientPUTAsync(hClient, p_lSynId, p_lFuncId, p_lSetCode, p_szJsonReq != NULL ? p_szJsonReq : "", SafeBufferLen(p_lJsonLen), &stPutData, AsyncCallback, pAsync);
		if (lRet <= 0)
		{
			if (pAsync->bHoldConnection.exchange(false))
			{
				ReleaseConnectionRef(pContext);
				ReleaseAsyncContext(pAsync);
			}
			ReleaseAsyncContext(pAsync);
			SetLastError(MakeIceRPCPushErrorDetail(hClient, "PUT_ASYNC_FAILED: JsonBinClientPUTAsync returned failure").c_str());
		}
		else
		{
			ReleaseAsyncContext(pAsync);
			SetLastError(s_iCloudNetOk, GetCloudNetErrorMsg(s_iCloudNetOk));
		}
		return lRet;
	}

	// 执行同步 Binary RPC，返回对象的所有权转交给调用方。
	ST_CLOUD_NET_BINARY_RESULT* CallBinarySync(const char* p_szConnName, const ST_CLOUD_NET_BINARY_CALL* p_pCall)
	{
		return BinaryCallSync(p_szConnName, p_pCall, false, "CallBinarySync");
	}

	// 提交异步 Binary RPC，底层受理后保证回调一次。
	long long CallBinaryAsync(const char* p_szConnName, const ST_CLOUD_NET_BINARY_CALL* p_pCall, PFN_CLOUD_NET_BINARY_ASYNC p_pfnCallback, void* p_pParam)
	{
		return BinaryCallAsync(p_szConnName, p_pCall, false, 0,
			p_pfnCallback, p_pParam, "CallBinaryAsync");
	}

	// 使用调用方期限覆盖本次 Binary RPC 超时，不修改连接共享配置。
	long long CallBinaryAsyncEx(const char* p_szConnName,
		const ST_CLOUD_NET_BINARY_CALL* p_pCall, int p_iTimeoutMs,
		PFN_CLOUD_NET_BINARY_ASYNC p_pfnCallback, void* p_pParam)
	{
		return BinaryCallAsync(p_szConnName, p_pCall, false, p_iTimeoutMs,
			p_pfnCallback, p_pParam, "CallBinaryAsyncEx");
	}

	// 编码异步 RPC 让最终消费 Worker 决定解压时机，CloudNet 不接触正文。
	long long CallBinaryEncodedAsyncEx(const char* p_szConnName,
		const ST_CLOUD_NET_BINARY_CALL* p_pCall, int p_iTimeoutMs,
		PFN_CLOUD_NET_BINARY_ENCODED_ASYNC p_pfnCallback, void* p_pParam)
	{
		return BinaryCallEncodedAsync(p_szConnName, p_pCall, false,
			p_iTimeoutMs, p_pfnCallback, p_pParam,
			"CallBinaryEncodedAsyncEx");
	}

	// 执行同步 Binary PUT，支持主载荷、扩展载荷和两组整型参数。
	ST_CLOUD_NET_BINARY_RESULT* PutBinarySync(const char* p_szConnName, const ST_CLOUD_NET_BINARY_CALL* p_pCall)
	{
		return BinaryCallSync(p_szConnName, p_pCall, true, "PutBinarySync");
	}

	// 提交异步 Binary PUT，提交失败时不触发上层回调。
	long long PutBinaryAsync(const char* p_szConnName, const ST_CLOUD_NET_BINARY_CALL* p_pCall, PFN_CLOUD_NET_BINARY_ASYNC p_pfnCallback, void* p_pParam)
	{
		return BinaryCallAsync(p_szConnName, p_pCall, true, 0,
			p_pfnCallback, p_pParam, "PutBinaryAsync");
	}

	// 使用调用方期限覆盖本次 Binary PUT 超时。
	long long PutBinaryAsyncEx(const char* p_szConnName,
		const ST_CLOUD_NET_BINARY_CALL* p_pCall, int p_iTimeoutMs,
		PFN_CLOUD_NET_BINARY_ASYNC p_pfnCallback, void* p_pParam)
	{
		return BinaryCallAsync(p_szConnName, p_pCall, true, p_iTimeoutMs,
			p_pfnCallback, p_pParam, "PutBinaryAsyncEx");
	}

	// 编码异步 PUT 保留 payload/extra 各自的 BinaryPayload 包络和所有权。
	long long PutBinaryEncodedAsyncEx(const char* p_szConnName,
		const ST_CLOUD_NET_BINARY_CALL* p_pCall, int p_iTimeoutMs,
		PFN_CLOUD_NET_BINARY_ENCODED_ASYNC p_pfnCallback, void* p_pParam)
	{
		return BinaryCallEncodedAsync(p_szConnName, p_pCall, true,
			p_iTimeoutMs, p_pfnCallback, p_pParam,
			"PutBinaryEncodedAsyncEx");
	}

	// 返回当前实例最后一次错误描述，返回指针为线程局部缓存。
	const char* GetLastErrorText() const
	{
		static thread_local std::string s_strErrorText;
		std::lock_guard<std::mutex> clLock(m_clErrorMutex);
		s_strErrorText = m_strLastError;
		return s_strErrorText.c_str();
	}


	// 读取统一 XML 组网配置，并按插件 id 生成本实例的本地服务、连接和订阅配置。
	int LoadConfig(const char* p_szXmlPath, const char* p_szPluginId)
	{
		ST_CLOUD_NET_CONFIG stConfig;
		std::string strError;
		if (!LoadCloudNetConfigFile(p_szXmlPath, p_szPluginId, stConfig, strError))
		{
			SetLastError(strError.c_str());
			return s_iCloudNetInvalidParam;
		}
		// 配置校验完成后再切换日志目标，避免非法 XML 把现有运行日志关闭。
		CCloudNetFileLog::Instance().Configure(stConfig.stLog.strPath,
			stConfig.stLog.strName, stConfig.stLog.strLevel);
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			m_stConfig = stConfig;
			m_mapRenewTime.clear();
		}
		SetLastError(s_iCloudNetOk, GetCloudNetErrorMsg(s_iCloudNetOk));
		return s_iCloudNetOk;
	}

	// 重新读取上一次配置文件，并关闭已经从新配置中删除的连接和订阅。
	int ReloadConfig()
	{
		ST_CLOUD_NET_CONFIG stOldConfig;
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			if (!m_stConfig.bLoaded)
			{
				SetLastError("CONFIG_NOT_LOADED: call LoadConfig before this operation");
				return s_iCloudNetStateError;
			}
			stOldConfig = m_stConfig;
		}
		ST_CLOUD_NET_CONFIG stNewConfig;
		std::string strError;
		if (!LoadCloudNetConfigFile(stOldConfig.strXmlPath.c_str(), stOldConfig.strPluginId.c_str(), stNewConfig, strError))
		{
			SetLastError(strError.c_str());
			return s_iCloudNetInvalidParam;
		}
		// Reload 成功后同步日志配置，后台维护线程后续错误立即写入新的目标文件。
		CCloudNetFileLog::Instance().Configure(stNewConfig.stLog.strPath,
			stNewConfig.stLog.strName, stNewConfig.stLog.strLevel);
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			m_stConfig = stNewConfig;
			m_mapRenewTime.clear();
		}
		CloseRemovedServices(stOldConfig, stNewConfig);
		// 重载只唤醒维护线程，禁止调用线程同步连接远端或注册推送。
		m_clNetworkCond.notify_all();
		SetLastError(s_iCloudNetOk, GetCloudNetErrorMsg(s_iCloudNetOk));
		return s_iCloudNetOk;
	}

	// 按已加载配置启动本地服务、自动连接远端服务，并开启后台重连和续约线程。
	int StartNetwork(PFN_CLOUD_NET_REQUEST p_pfnRequest, PFN_CLOUD_NET_PUSH p_pfnPush, void* p_pParam)
	{
		ST_CLOUD_NET_CONFIG stConfig;
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			if (!m_stConfig.bLoaded)
			{
				SetLastError("CONFIG_NOT_LOADED: call LoadConfig before this operation");
				return s_iCloudNetStateError;
			}
			stConfig = m_stConfig;
		}
		if (stConfig.stLocal.bStartService && p_pfnRequest == NULL)
		{
			SetLastError("INVALID_PARAM: StartNetwork request callback is null");
			return s_iCloudNetInvalidParam;
		}
		if (HasEnabledSubscription(stConfig) && p_pfnPush == NULL)
		{
			SetLastError("INVALID_PARAM: StartNetwork push callback is null");
			return s_iCloudNetInvalidParam;
		}
		StopNetwork();
		if (stConfig.stLocal.bStartService)
		{
			int iRet = StartService(stConfig.strXmlPath.c_str(), stConfig.stLocal.strService.c_str(), stConfig.stLocal.iSnappy, p_pfnRequest, p_pParam);
			if (iRet != s_iCloudNetOk)
			{
				return iRet;
			}
		}
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			m_pfnNetworkPush = p_pfnPush;
			m_pNetworkParam = p_pParam;
			m_bNetworkStop = false;
			m_bNetworkStarted = true;
		}
		m_clNetworkThread = std::thread(&CCloudNetDataApi::NetworkThreadProc, this);
		CCloudNetFileLog::Instance().WriteInfo("StartNetwork",
			"dependency startup step begin,service=" +
			stConfig.stLocal.strService +
			",step=cloudnet_remote_maintenance,pluginId=" +
			stConfig.strPluginId);
		SetLastError(s_iCloudNetOk, GetCloudNetErrorMsg(s_iCloudNetOk));
		return s_iCloudNetOk;
	}

	// 按已加载配置启动 Binary 服务和连接维护，推送订阅继续复用统一组网线程。
	int StartBinaryNetwork(PFN_CLOUD_NET_BINARY_REQUEST p_pfnRequest, PFN_CLOUD_NET_PUSH p_pfnPush, void* p_pParam)
	{
		ST_CLOUD_NET_CONFIG stConfig;
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			if (!m_stConfig.bLoaded)
			{
				SetLastError("CONFIG_NOT_LOADED: call LoadConfig before StartBinaryNetwork");
				return s_iCloudNetStateError;
			}
			stConfig = m_stConfig;
		}
		if (stConfig.stLocal.bStartService && p_pfnRequest == NULL)
		{
			SetLastError("INVALID_PARAM: StartBinaryNetwork binary request callback is null");
			return s_iCloudNetInvalidParam;
		}
		if (HasEnabledSubscription(stConfig) && p_pfnPush == NULL)
		{
			SetLastError("INVALID_PARAM: StartBinaryNetwork push callback is null while subscriptions are enabled");
			return s_iCloudNetInvalidParam;
		}
		StopNetwork();
		if (stConfig.stLocal.bStartService)
		{
			int iRet = StartBinaryService(stConfig.strXmlPath.c_str(), stConfig.stLocal.strService.c_str(), stConfig.stLocal.iSnappy, p_pfnRequest, p_pParam);
			if (iRet != s_iCloudNetOk)
			{
				return iRet;
			}
		}
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			m_pfnNetworkPush = p_pfnPush;
			m_pNetworkParam = p_pParam;
			m_bNetworkStop = false;
			m_bNetworkStarted = true;
		}
		m_clNetworkThread = std::thread(&CCloudNetDataApi::NetworkThreadProc, this);
		CCloudNetFileLog::Instance().WriteInfo("StartBinaryNetwork",
			"dependency startup step begin,service=" +
			stConfig.stLocal.strService +
			",step=cloudnet_remote_maintenance,pluginId=" +
			stConfig.strPluginId);
		SetLastError(s_iCloudNetOk, GetCloudNetErrorMsg(s_iCloudNetOk));
		return s_iCloudNetOk;
	}

	// 按已加载配置启动支持延迟应答的 Binary 服务和连接维护。
	int StartBinaryNetworkEx(PFN_CLOUD_NET_BINARY_REQUEST_EX p_pfnRequest,
		PFN_CLOUD_NET_BINARY_PRIORITY_CLASSIFIER p_pfnPriorityClassifier,
		PFN_CLOUD_NET_PUSH p_pfnPush, void* p_pParam)
	{
		ST_CLOUD_NET_CONFIG stConfig;
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			if (!m_stConfig.bLoaded)
			{
				SetLastError("CONFIG_NOT_LOADED: call LoadConfig before StartBinaryNetworkEx");
				return s_iCloudNetStateError;
			}
			stConfig = m_stConfig;
		}
		if (stConfig.stLocal.bStartService && p_pfnRequest == NULL)
		{
			SetLastError("INVALID_PARAM: StartBinaryNetworkEx binary request callback is null");
			return s_iCloudNetInvalidParam;
		}
		if (HasEnabledSubscription(stConfig) && p_pfnPush == NULL)
		{
			SetLastError("INVALID_PARAM: StartBinaryNetworkEx push callback is null while subscriptions are enabled");
			return s_iCloudNetInvalidParam;
		}
		StopNetwork();
		if (stConfig.stLocal.bStartService)
		{
			int iRet = StartBinaryServiceEx(stConfig.strXmlPath.c_str(),
				stConfig.stLocal.strService.c_str(), stConfig.stLocal.iSnappy,
				p_pfnRequest, p_pfnPriorityClassifier, p_pParam);
			if (iRet != s_iCloudNetOk)
			{
				return iRet;
			}
		}
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			m_pfnNetworkPush = p_pfnPush;
			m_pNetworkParam = p_pParam;
			m_bNetworkStop = false;
			m_bNetworkStarted = true;
		}
		m_clNetworkThread = std::thread(&CCloudNetDataApi::NetworkThreadProc, this);
		CCloudNetFileLog::Instance().WriteInfo("StartBinaryNetworkEx",
			"dependency startup step begin,service=" +
			stConfig.stLocal.strService +
			",step=cloudnet_remote_maintenance,pluginId=" +
			stConfig.strPluginId);
		SetLastError(s_iCloudNetOk, GetCloudNetErrorMsg(s_iCloudNetOk));
		return s_iCloudNetOk;
	}

	// 停止后台维护线程，注销配置订阅，关闭配置连接并停止本地服务。
	int StopNetwork()
	{
		bool bNeedJoin = false;
		std::string strServiceName;
		std::string strPluginId;
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			m_bNetworkStop = true;
			bNeedJoin = m_clNetworkThread.joinable();
			strServiceName = m_stConfig.stLocal.strService;
			strPluginId = m_stConfig.strPluginId;
		}
		m_clNetworkCond.notify_all();
		if (bNeedJoin)
		{
			CCloudNetFileLog::Instance().WriteInfo("StopNetwork",
				"dependency startup step stopping,service=" + strServiceName +
				",step=cloudnet_remote_maintenance,pluginId=" + strPluginId);
			m_clNetworkThread.join();
		}
		ST_CLOUD_NET_CONFIG stConfig;
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			stConfig = m_stConfig;
			m_bNetworkStarted = false;
			m_pfnNetworkPush = NULL;
			m_pNetworkParam = NULL;
			m_mapRenewTime.clear();
			m_setDependencyPending.clear();
			m_setDependencyEverReady.clear();
			m_mapDependencyError.clear();
			m_mapDependencyLogTime.clear();
		}
		UnregisterConfiguredSubscriptions(stConfig);
		CloseConfiguredServices(stConfig);
		StopService();
		if (bNeedJoin)
		{
			CCloudNetFileLog::Instance().WriteInfo("StopNetwork",
				"dependency startup step stopped,service=" + strServiceName +
				",step=cloudnet_remote_maintenance,pluginId=" + strPluginId);
		}
		SetLastError(s_iCloudNetOk, GetCloudNetErrorMsg(s_iCloudNetOk));
		return s_iCloudNetOk;
	}

	// 通过本地服务端向已注册的订阅方发布推送数据。
	long long Publish(long long p_lReqNo, const char* p_pBuf, long p_lBufLen, int p_iAsync)
	{
		if (p_lReqNo < INT_MIN || p_lReqNo > INT_MAX)
		{
			SetLastError("INVALID_PARAM: Publish request number is outside int range");
			return s_iCloudNetInvalidParam;
		}
		if (p_lBufLen < 0 || (p_lBufLen > 0 && p_pBuf == NULL))
		{
			SetLastError("INVALID_PARAM: Publish buffer pointer or length is invalid");
			return s_iCloudNetInvalidParam;
		}
		HANDLE hServer = NULL;
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			if (m_hServer == NULL)
			{
				SetLastError("SERVICE_NOT_STARTED: local service handle is null");
				return s_iCloudNetNotFound;
			}
			hServer = m_hServer;
			++m_iActiveServiceCall;
		}
		long long lRet = PushJsonICEServerData(hServer, static_cast<int>(p_lReqNo), p_pBuf != NULL ? p_pBuf : "", SafeBufferLen(p_lBufLen), p_iAsync != 0);
		ReleaseServiceCall();
		SetLastError(lRet >= 0 ? "" : MakeIceRPCPushErrorDetail(hServer, "PUBLISH_FAILED: PushJsonICEServerData returned failure").c_str());
		return lRet;
	}

private:
	// 公共 Binary 同步调用实现，统一执行参数校验、连接保护和结果深拷贝。
	ST_CLOUD_NET_BINARY_RESULT* BinaryCallSync(const char* p_szConnName, const ST_CLOUD_NET_BINARY_CALL* p_pCall, bool p_bPut, const char* p_szFunction)
	{
		ST_BINARY_CALL stIceCall;
		std::string strError;
		if (!FillIceBinaryCall(p_pCall, stIceCall, strError))
		{
			SetLastError(s_iCloudNetBinaryProtocolError, strError.c_str(), p_szFunction);
			return NULL;
		}
		ST_CONNECTION_CONTEXT_PTR pContext = FindConnection(p_szConnName);
		if (!pContext)
		{
			return NULL;
		}
		CConnectionCallGuard clCallGuard(pContext);
		if (!clCallGuard.IsValid())
		{
			SetLastError("CONNECTION_CLOSING: binary connection is closing or closed");
			return NULL;
		}
		ST_BINARY_RESULT* pIceResult = p_bPut ? BinaryClientPUT(clCallGuard.GetHandle(), &stIceCall) : BinaryClientRPC(clCallGuard.GetHandle(), &stIceCall);
		if (pIceResult == NULL)
		{
			std::string strDetail = MakeIceRPCPushErrorDetail(clCallGuard.GetHandle(), p_bPut ? "BINARY_PUT_SYNC_FAILED: BinaryClientPUT returned null" : "BINARY_RPC_SYNC_FAILED: BinaryClientRPC returned null");
			SetLastError(s_iCloudNetBinaryCallFailed, strDetail.c_str(), p_szFunction);
			return NULL;
		}
		ST_CLOUD_NET_BINARY_RESULT* pResult = CopyBinaryResult(pIceResult, p_pCall->lSynId, p_pCall->lFuncId);
		BinaryResultFree(pIceResult);
		if (pResult == NULL)
		{
			SetLastError(s_iCloudNetMemoryError, "MEMORY_ERROR: failed to copy binary sync result", p_szFunction);
			return NULL;
		}
		if (pResult->iErrorCode != 0)
		{
			std::string strDetail = "BINARY_REMOTE_ERROR: conn=";
			strDetail += p_szConnName != NULL ? p_szConnName : "";
			strDetail += ", funcId=" + std::to_string(p_pCall->lFuncId);
			strDetail += ", binary_code=" + std::to_string(pResult->iErrorCode);
			strDetail += ", detail=" + MakeApiErrorAscii(pResult->szErrInfo);
			SetLastError(s_iCloudNetBinaryCallFailed, strDetail.c_str(), p_szFunction);
		}
		else
		{
			SetLastError(s_iCloudNetOk, GetCloudNetErrorMsg(s_iCloudNetOk));
		}
		return pResult;
	}

	// 公共 Binary 异步提交实现，失败路径释放上下文且绝不触发回调。
	long long BinaryCallAsync(const char* p_szConnName,
		const ST_CLOUD_NET_BINARY_CALL* p_pCall, bool p_bPut,
		int p_iTimeoutMs, PFN_CLOUD_NET_BINARY_ASYNC p_pfnCallback,
		void* p_pParam, const char* p_szFunction)
	{
		if (p_pfnCallback == NULL || p_iTimeoutMs < 0)
		{
			SetLastError(s_iCloudNetInvalidParam,
				"INVALID_PARAM: binary async callback or timeout is invalid",
				p_szFunction);
			return s_iCloudNetInvalidParam;
		}
		ST_BINARY_CALL stIceCall;
		std::string strError;
		if (!FillIceBinaryCall(p_pCall, stIceCall, strError))
		{
			SetLastError(s_iCloudNetBinaryProtocolError, strError.c_str(), p_szFunction);
			return s_iCloudNetBinaryProtocolError;
		}
		ST_CONNECTION_CONTEXT_PTR pContext = FindConnection(p_szConnName);
		if (!pContext)
		{
			return s_iCloudNetNotFound;
		}
		ST_BINARY_ASYNC_CONTEXT* pAsync = new (std::nothrow) ST_BINARY_ASYNC_CONTEXT;
		if (pAsync == NULL)
		{
			SetLastError(s_iCloudNetMemoryError, "MEMORY_ERROR: binary async context allocation failed", p_szFunction);
			return s_iCloudNetMemoryError;
		}
		pAsync->pfnCallback = p_pfnCallback;
		pAsync->pParam = p_pParam;
		pAsync->lSynId = p_pCall->lSynId;
		pAsync->lFuncId = p_pCall->lFuncId;

		HANDLE hClient = NULL;
		if (!RetainConnectionRef(pContext, hClient))
		{
			ReleaseBinaryAsyncContext(pAsync);
			ReleaseBinaryAsyncContext(pAsync);
			SetLastError("CONNECTION_CLOSING: binary connection is closing or closed");
			return s_iCloudNetNotFound;
		}
		pAsync->refConnection = pContext;
		pAsync->bHoldConnection = true;
		long long lRet = 0;
		if (p_iTimeoutMs > 0)
		{
			lRet = p_bPut ?
				BinaryClientPUTAsyncEx(hClient, &stIceCall, p_iTimeoutMs,
					BinaryAsyncCallback, pAsync) :
				BinaryClientRPCAsyncEx(hClient, &stIceCall, p_iTimeoutMs,
					BinaryAsyncCallback, pAsync);
		}
		else
		{
			lRet = p_bPut ?
				BinaryClientPUTAsync(hClient, &stIceCall,
					BinaryAsyncCallback, pAsync) :
				BinaryClientRPCAsync(hClient, &stIceCall,
					BinaryAsyncCallback, pAsync);
		}
		if (lRet < 0)
		{
			std::string strDetail = MakeIceRPCPushErrorDetail(hClient, p_bPut ? "BINARY_PUT_ASYNC_FAILED: BinaryClientPUTAsync rejected request" : "BINARY_RPC_ASYNC_FAILED: BinaryClientRPCAsync rejected request");
			if (pAsync->bHoldConnection.exchange(false))
			{
				ReleaseConnectionRef(pContext);
			}
			ReleaseBinaryAsyncContext(pAsync);
			ReleaseBinaryAsyncContext(pAsync);
			SetLastError(s_iCloudNetBinaryCallFailed, strDetail.c_str(), p_szFunction);
			return lRet;
		}
		ReleaseBinaryAsyncContext(pAsync);
		SetLastError(s_iCloudNetOk, GetCloudNetErrorMsg(s_iCloudNetOk));
		return lRet;
	}

	// 拥有型编码异步提交与旧异步调用共用连接保护，但回调后所有权由调用方显式释放。
	long long BinaryCallEncodedAsync(const char* p_szConnName,
		const ST_CLOUD_NET_BINARY_CALL* p_pCall, bool p_bPut,
		int p_iTimeoutMs,
		PFN_CLOUD_NET_BINARY_ENCODED_ASYNC p_pfnCallback,
		void* p_pParam, const char* p_szFunction)
	{
		if (p_pfnCallback == NULL || p_iTimeoutMs < 0)
		{
			SetLastError(s_iCloudNetInvalidParam,
				"INVALID_PARAM: encoded binary callback or timeout is invalid",
				p_szFunction);
			return s_iCloudNetInvalidParam;
		}
		ST_BINARY_CALL stIceCall;
		std::string strError;
		if (!FillIceBinaryCall(p_pCall, stIceCall, strError))
		{
			SetLastError(s_iCloudNetBinaryProtocolError, strError.c_str(),
				p_szFunction);
			return s_iCloudNetBinaryProtocolError;
		}
		ST_CONNECTION_CONTEXT_PTR pContext = FindConnection(p_szConnName);
		if (!pContext)
		{
			return s_iCloudNetNotFound;
		}
		ST_BINARY_ENCODED_ASYNC_CONTEXT* pAsync =
			new (std::nothrow) ST_BINARY_ENCODED_ASYNC_CONTEXT;
		if (pAsync == NULL)
		{
			SetLastError(s_iCloudNetMemoryError,
				"MEMORY_ERROR: encoded binary async context allocation failed",
				p_szFunction);
			return s_iCloudNetMemoryError;
		}
		pAsync->pfnCallback = p_pfnCallback;
		pAsync->pParam = p_pParam;
		pAsync->lSynId = p_pCall->lSynId;
		pAsync->lFuncId = p_pCall->lFuncId;
		pAsync->stResult.pParam = p_pParam;

		HANDLE hClient = NULL;
		if (!RetainConnectionRef(pContext, hClient))
		{
			ReleaseBinaryEncodedAsyncContext(pAsync);
			ReleaseBinaryEncodedAsyncContext(pAsync);
			SetLastError("CONNECTION_CLOSING: binary connection is closing or closed");
			return s_iCloudNetNotFound;
		}
		pAsync->refConnection = pContext;
		pAsync->bHoldConnection = true;
		const long long lRet = p_bPut ?
			BinaryClientPUTAsyncEncodedEx(hClient, &stIceCall,
				p_iTimeoutMs, BinaryEncodedAsyncCallback, pAsync) :
			BinaryClientRPCAsyncEncodedEx(hClient, &stIceCall,
				p_iTimeoutMs, BinaryEncodedAsyncCallback, pAsync);
		if (lRet < 0)
		{
			std::string strDetail = MakeIceRPCPushErrorDetail(hClient,
				p_bPut ?
				"BINARY_PUT_ENCODED_ASYNC_FAILED: request was rejected" :
				"BINARY_RPC_ENCODED_ASYNC_FAILED: request was rejected");
			if (pAsync->bHoldConnection.exchange(false))
			{
				ReleaseConnectionRef(pContext);
			}
			ReleaseBinaryEncodedAsyncContext(pAsync);
			ReleaseBinaryEncodedAsyncContext(pAsync);
			SetLastError(s_iCloudNetBinaryCallFailed, strDetail.c_str(),
				p_szFunction);
			return lRet;
		}
		ReleaseBinaryEncodedAsyncContext(pAsync);
		SetLastError(s_iCloudNetOk, GetCloudNetErrorMsg(s_iCloudNetOk));
		return lRet;
	}

	// 查找可用连接上下文；连接不存在或正在关闭时同步设置最后错误。
	ST_CONNECTION_CONTEXT_PTR FindConnection(const char* p_szConnName)
	{
		if (p_szConnName == NULL || p_szConnName[0] == '\0')
		{
			SetLastError("INVALID_PARAM: connection name is empty");
			return ST_CONNECTION_CONTEXT_PTR();
		}
		std::lock_guard<std::mutex> clLock(m_clMutex);
		std::map<std::string, ST_CONNECTION_CONTEXT_PTR>::iterator it = m_mapConnection.find(p_szConnName);
		if (it == m_mapConnection.end() || !it->second)
		{
			SetLastError("CONNECTION_NOT_FOUND: connection does not exist");
			return ST_CONNECTION_CONTEXT_PTR();
		}
		{
			std::lock_guard<std::mutex> clConnLock(it->second->clMutex);
			if (it->second->bClosing || it->second->hClient == NULL)
			{
				SetLastError("CONNECTION_CLOSING: connection is closing or closed");
				return ST_CONNECTION_CONTEXT_PTR();
			}
		}
		return it->second;
	}

	// 创建同名连接前先移除旧连接，避免连接表保留过期句柄。
	void RemoveConnectionIfExists(const char* p_szConnName)
	{
		if (p_szConnName == NULL || p_szConnName[0] == '\0')
		{
			return;
		}
		ST_CONNECTION_CONTEXT_PTR pContext;
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			std::map<std::string, ST_CONNECTION_CONTEXT_PTR>::iterator it = m_mapConnection.find(p_szConnName);
			if (it == m_mapConnection.end())
			{
				return;
			}
			pContext = it->second;
			m_mapConnection.erase(it);
		}
		CloseConnection(pContext);
		RetireConnection(pContext);
	}

	// 判断连接表中的指定连接是否仍持有有效底层客户端句柄。
	bool IsConnectionAlive(const std::string& p_strConnName)
	{
		std::lock_guard<std::mutex> clLock(m_clMutex);
		std::map<std::string, ST_CONNECTION_CONTEXT_PTR>::iterator it = m_mapConnection.find(p_strConnName);
		if (it == m_mapConnection.end() || !it->second)
		{
			return false;
		}
		std::lock_guard<std::mutex> clConnLock(it->second->clMutex);
		return !it->second->bClosing && it->second->hClient != NULL;
	}

	// 执行一次网络维护，包含自动重连和订阅续约。
	void MaintainNetworkOnce()
	{
		ST_CLOUD_NET_CONFIG stConfig;
		PFN_CLOUD_NET_PUSH pfnPush = NULL;
		void* pPushParam = NULL;
		bool bStarted = false;
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			stConfig = m_stConfig;
			pfnPush = m_pfnNetworkPush;
			pPushParam = m_pNetworkParam;
			bStarted = m_bNetworkStarted && !m_bNetworkStop;
		}
		if (!bStarted || !stConfig.bLoaded)
		{
			return;
		}

		for (std::vector<ST_SERVICE_CONFIG>::const_iterator it = stConfig.aService.begin(); it != stConfig.aService.end(); ++it)
		{
			if (!it->bAutoConnect || IsConnectionAlive(it->strId))
			{
				continue;
			}
			const std::string strKey = "connect:" + it->strId;
			LogDependencyStepBegin(strKey, "remote_ice_connect",
				stConfig.stLocal.strService, it->strId,
				stConfig.strPluginId);
			const int iRet = Connect(stConfig.strXmlPath.c_str(),
				it->strProxy.c_str(), it->strId.c_str(),
				stConfig.stRuntime.iThreadPool);
			if (iRet == s_iCloudNetOk)
			{
				LogDependencyStepReady(strKey, "remote_ice_connect",
					stConfig.stLocal.strService, it->strId,
					stConfig.strPluginId);
			}
			else
			{
				LogDependencyStepPending(strKey, "remote_ice_connect",
					stConfig.stLocal.strService, it->strId,
					stConfig.strPluginId, GetLastErrorText(),
					stConfig.stRuntime.iReconnectIntervalMs);
			}
		}
		RenewSubscriptions(stConfig, pfnPush, pPushParam);
	}

	// 按配置和续约时间向远端服务刷新 RegisterPush 注册。
	void RenewSubscriptions(const ST_CLOUD_NET_CONFIG& p_refConfig, PFN_CLOUD_NET_PUSH p_pfnPush, void* p_pParam)
	{
		if (p_pfnPush == NULL)
		{
			return;
		}
		std::chrono::steady_clock::time_point tmNow = std::chrono::steady_clock::now();
		for (std::vector<ST_SUBSCRIPTION_CONFIG>::const_iterator it = p_refConfig.aSubscription.begin(); it != p_refConfig.aSubscription.end(); ++it)
		{
			if (!it->bEnable || !IsConnectionAlive(it->strTarget))
			{
				continue;
			}
			std::string strKey = MakeSubscriptionKey(*it);
			bool bNeedRenew = false;
			{
				std::lock_guard<std::mutex> clLock(m_clMutex);
				std::map<std::string, std::chrono::steady_clock::time_point>::iterator itRenew = m_mapRenewTime.find(strKey);
				bNeedRenew = itRenew == m_mapRenewTime.end() || tmNow >= itRenew->second;
			}
			if (!bNeedRenew)
			{
				continue;
			}
			const std::string strLogKey = "subscription:" + it->strTarget +
				":" + it->strPluginId;
			LogDependencyStepBegin(strLogKey, "remote_push_subscription",
				p_refConfig.stLocal.strService, it->strTarget,
				it->strPluginId);
			if (RegisterPush(it->strTarget.c_str(), it->strPluginId.c_str(), it->strSubInfo.c_str(), p_pfnPush, p_pParam) == s_iCloudNetOk)
			{
				std::lock_guard<std::mutex> clLock(m_clMutex);
				m_mapRenewTime[strKey] = tmNow + std::chrono::seconds(p_refConfig.stRuntime.iRenewIntervalSec);
				LogDependencyStepReadyNoLock(strLogKey,
					"remote_push_subscription",
					p_refConfig.stLocal.strService, it->strTarget,
					it->strPluginId);
			}
			else
			{
				LogDependencyStepPending(strLogKey,
					"remote_push_subscription",
					p_refConfig.stLocal.strService, it->strTarget,
					it->strPluginId, GetLastErrorText(),
					p_refConfig.stRuntime.iReconnectIntervalMs);
			}
		}
	}

	// 首次进入一个外部依赖步骤时记录开始；后续轮询复用同一等待状态。
	void LogDependencyStepBegin(const std::string& p_refKey,
		const char* p_szStep, const std::string& p_refService,
		const std::string& p_refConnection,
		const std::string& p_refPluginId)
	{
		std::lock_guard<std::mutex> clLock(m_clMutex);
		if (m_setDependencyPending.insert(p_refKey).second)
		{
			CCloudNetFileLog::Instance().WriteInfo("NetworkThreadProc",
				"dependency startup step begin,service=" + p_refService +
				",step=" + std::string(p_szStep) +
				",connection=" + p_refConnection + ",pluginId=" + p_refPluginId);
		}
	}

	// 失败只在原因变化或六十秒汇总时记录，避免按重连周期刷屏。
	void LogDependencyStepPending(const std::string& p_refKey,
		const char* p_szStep, const std::string& p_refService,
		const std::string& p_refConnection,
		const std::string& p_refPluginId, const std::string& p_refError,
		int p_iRetryMs)
	{
		const std::chrono::steady_clock::time_point tmNow =
			std::chrono::steady_clock::now();
		bool bLog = false;
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			const std::map<std::string, std::chrono::steady_clock::time_point>::iterator itTime =
				m_mapDependencyLogTime.find(p_refKey);
			bLog = m_mapDependencyError[p_refKey] != p_refError ||
				itTime == m_mapDependencyLogTime.end() ||
				tmNow - itTime->second >= std::chrono::seconds(60);
			if (bLog)
			{
				m_mapDependencyError[p_refKey] = p_refError;
				m_mapDependencyLogTime[p_refKey] = tmNow;
			}
		}
		if (bLog)
		{
			CCloudNetFileLog::Instance().WriteInfo("NetworkThreadProc",
				"dependency startup step pending,service=" + p_refService +
				",step=" + std::string(p_szStep) +
				",connection=" + p_refConnection + ",pluginId=" + p_refPluginId +
				",retryMs=" + std::to_string(p_iRetryMs) + ",detail=" + p_refError);
		}
	}

	// 成功后清理等待状态，并区分首次完成和故障后恢复。
	void LogDependencyStepReady(const std::string& p_refKey,
		const char* p_szStep, const std::string& p_refService,
		const std::string& p_refConnection,
		const std::string& p_refPluginId)
	{
		std::lock_guard<std::mutex> clLock(m_clMutex);
		LogDependencyStepReadyNoLock(p_refKey, p_szStep, p_refService,
			p_refConnection, p_refPluginId);
	}

	// 调用方持有 m_clMutex 时完成依赖成功状态切换。
	void LogDependencyStepReadyNoLock(const std::string& p_refKey,
		const char* p_szStep, const std::string& p_refService,
		const std::string& p_refConnection,
		const std::string& p_refPluginId)
	{
		if (m_setDependencyPending.erase(p_refKey) == 0U)
		{
			return;
		}
		const bool bRecovered = m_setDependencyEverReady.find(p_refKey) !=
			m_setDependencyEverReady.end();
		m_setDependencyEverReady.insert(p_refKey);
		m_mapDependencyError.erase(p_refKey);
		m_mapDependencyLogTime.erase(p_refKey);
		CCloudNetFileLog::Instance().WriteInfo("NetworkThreadProc",
			"dependency startup step " +
			std::string(bRecovered ? "recovered" : "completed") +
			",service=" + p_refService + ",step=" + p_szStep +
			",connection=" + p_refConnection +
			",pluginId=" + p_refPluginId);
	}

	// 后台维护线程入口，周期执行重连和推送订阅续约。
	void NetworkThreadProc()
	{
		for (;;)
		{
			ST_CLOUD_NET_CONFIG stConfig;
			{
				std::unique_lock<std::mutex> clLock(m_clMutex);
				if (m_bNetworkStop)
				{
					break;
				}
				stConfig = m_stConfig;
			}
			try
			{
				MaintainNetworkOnce();
			}
			catch (const std::exception& ex)
			{
				SetCloudNetBoundaryThreadError(s_iCloudNetInternalError, MakeCloudNetStdExceptionDetail("NetworkThreadProc", ex), "NetworkThreadProc");
			}
			catch (...)
			{
				SetCloudNetBoundaryThreadError(s_iCloudNetInternalError, MakeCloudNetUnknownExceptionDetail("NetworkThreadProc"), "NetworkThreadProc");
			}
			std::unique_lock<std::mutex> clLock(m_clMutex);
			if (m_clNetworkCond.wait_for(clLock, std::chrono::milliseconds(stConfig.stRuntime.iReconnectIntervalMs), [this] { return m_bNetworkStop.load(); }))
			{
				break;
			}
		}
	}

	// 停止网络或重载配置前注销配置中的启用订阅。
	void UnregisterConfiguredSubscriptions(const ST_CLOUD_NET_CONFIG& p_refConfig)
	{
		for (std::vector<ST_SUBSCRIPTION_CONFIG>::const_iterator it = p_refConfig.aSubscription.begin(); it != p_refConfig.aSubscription.end(); ++it)
		{
			if (it->bEnable && IsConnectionAlive(it->strTarget))
			{
				UnregisterPush(it->strTarget.c_str(), it->strPluginId.c_str(), it->strSubInfo.c_str());
			}
		}
	}

	// 关闭配置中声明的远端服务连接。
	void CloseConfiguredServices(const ST_CLOUD_NET_CONFIG& p_refConfig)
	{
		for (std::vector<ST_SERVICE_CONFIG>::const_iterator it = p_refConfig.aService.begin(); it != p_refConfig.aService.end(); ++it)
		{
			if (IsConnectionAlive(it->strId))
			{
				Disconnect(it->strId.c_str());
			}
		}
	}

	// 配置重载后关闭新配置不再包含的旧服务连接。
	void CloseRemovedServices(const ST_CLOUD_NET_CONFIG& p_refOldConfig, const ST_CLOUD_NET_CONFIG& p_refNewConfig)
	{
		for (std::vector<ST_SUBSCRIPTION_CONFIG>::const_iterator it = p_refOldConfig.aSubscription.begin(); it != p_refOldConfig.aSubscription.end(); ++it)
		{
			if (it->bEnable && IsConnectionAlive(it->strTarget))
			{
				UnregisterPush(it->strTarget.c_str(), it->strPluginId.c_str(), it->strSubInfo.c_str());
			}
		}
		for (std::vector<ST_SERVICE_CONFIG>::const_iterator it = p_refOldConfig.aService.begin(); it != p_refOldConfig.aService.end(); ++it)
		{
			if (!HasServiceConfig(p_refNewConfig, it->strId) && IsConnectionAlive(it->strId))
			{
				Disconnect(it->strId.c_str());
			}
		}
	}

	// 创建异步调用上下文，封装回调、透传参数和连接引用。
	ST_ASYNC_CONTEXT* CreateAsyncContext(PFN_CLOUD_NET_ASYNC p_pfnCallback, void* p_pParam)
	{
		ST_ASYNC_CONTEXT* pAsync = new (std::nothrow) ST_ASYNC_CONTEXT;
		if (pAsync == NULL)
		{
			SetLastError("MEMORY_ERROR: async context allocation failed");
			return NULL;
		}
		pAsync->pfnCallback = p_pfnCallback;
		pAsync->pParam = p_pParam;
		return pAsync;
	}

	// 保存已断开的连接上下文，保证底层回调仍可能到达时对象不被立即析构。
	void RetireConnection(const ST_CONNECTION_CONTEXT_PTR& p_refContext)
	{
		if (!p_refContext)
		{
			return;
		}
		std::lock_guard<std::mutex> clLock(m_clMutex);
		m_aRetiredConnection.push_back(p_refContext);
	}

	// 标记连接关闭，等待活动调用退出后释放 IceRPCPush 客户端句柄。
	void CloseConnection(const ST_CONNECTION_CONTEXT_PTR& p_refContext)
	{
		if (!p_refContext)
		{
			return;
		}
		HANDLE hClient = NULL;
		{
			std::unique_lock<std::mutex> clLock(p_refContext->clMutex);
			p_refContext->bClosing = true;
			p_refContext->pfnPush = NULL;
			p_refContext->pPushParam = NULL;
			while (p_refContext->iActiveCall > 0)
			{
				p_refContext->clIdleCond.wait(clLock);
			}
			hClient = p_refContext->hClient;
			p_refContext->hClient = NULL;
		}
		if (hClient != NULL)
		{
			DeleteJsonICERPC(hClient);
		}
	}

	// 设置实例最后错误，所有对外错误文本在这里统一清洗为 ASCII。
	void SetLastError(int p_iErrorCode, const char* p_szError, const char* p_szFunction = NULL)
	{
		std::string strDetail = MakeApiErrorAscii((p_szError != NULL && p_szError[0] != '\0') ? p_szError : GetCloudNetErrorMsg(p_iErrorCode));
		{
			std::lock_guard<std::mutex> clLock(m_clErrorMutex);
			m_iLastErrorCode = p_iErrorCode;
			m_strLastError = strDetail;
		}
		SetThreadLastError(p_iErrorCode, strDetail);
		WriteCloudNetErrorLog(p_szFunction, p_iErrorCode, strDetail);
	}

	// 根据错误前缀推断错误码，并写入实例和线程最后错误。
	void SetLastError(const char* p_szError)
	{
		if (p_szError == NULL || p_szError[0] == '\0')
		{
			SetLastError(s_iCloudNetOk, GetCloudNetErrorMsg(s_iCloudNetOk));
			return;
		}
		SetLastError(InferCloudNetErrorCode(p_szError), p_szError, __FUNCTION__);
	}

public:
	// 返回当前实例最后一次错误码，供 C API 查询。
	int GetLastErrorCodeValue() const
	{
		std::lock_guard<std::mutex> clLock(m_clErrorMutex);
		return m_iLastErrorCode;
	}

	// C API 边界捕获异常后通过该函数写入实例最后错误。
	void SetBoundaryError(int p_iErrorCode, const std::string& p_strDetail, const char* p_szFunction)
	{
		SetLastError(p_iErrorCode, p_strDetail.c_str(), p_szFunction);
	}

private:
	// 服务端回调计数守卫，保证上层回调异常时 StopService 也不会永久等待。
	class CServiceCallGuard
	{
	public:
		// 构造服务端调用守卫，记录需要释放调用计数的宿主对象。
		CServiceCallGuard(CCloudNetDataApi* p_pThis)
		{
			m_pThis = p_pThis;
		}

		// 离开服务端回调路径时自动释放活动调用计数。
		~CServiceCallGuard()
		{
			if (m_pThis != NULL)
			{
				m_pThis->ReleaseServiceCall();
			}
		}

	private:
		CServiceCallGuard(const CServiceCallGuard&);
		CServiceCallGuard& operator=(const CServiceCallGuard&);

		CCloudNetDataApi* m_pThis;                              // 需要在析构时释放活动服务端调用计数的宿主对象。
	};

	// 释放一次服务端活动调用计数，并在计数归零时唤醒 StopService。
	void ReleaseServiceCall()
	{
		std::lock_guard<std::mutex> clLock(m_clMutex);
		if (m_iActiveServiceCall > 0)
		{
			--m_iActiveServiceCall;
		}
		if (m_iActiveServiceCall == 0)
		{
			m_clServiceIdleCond.notify_all();
		}
	}
	// IceRPCPush 服务端回调静态入口，负责把透传参数还原为 CCloudNetDataApi 实例。
	static void ServerCallback(void* p_pParam, short p_chMode, long long p_lSetCode, ST_JSON_M_RESULT_TOP* p_pResult)
	{
		CCloudNetDataApi* pThis = reinterpret_cast<CCloudNetDataApi*>(p_pParam);
		if (pThis != NULL)
		{
			pThis->HandleServerCallback(p_chMode, p_lSetCode, p_pResult);
		}
	}

	// IceRPCPush Binary 服务回调静态入口，只向上层暴露 CloudNetDataApi 公共结构。
	static void BinaryServerCallback(void* p_pParam, const ST_BINARY_REQUEST* p_pRequest)
	{
		CCloudNetDataApi* pThis = reinterpret_cast<CCloudNetDataApi*>(p_pParam);
		if (pThis != NULL)
		{
			pThis->HandleBinaryServerCallback(p_pRequest);
		}
	}

	// 延迟应答回调桥只转换公共结构，底层请求缓冲仍在回调返回后立即失效。
	static int BinaryServerCallbackEx(void* p_pParam,
		const ST_BINARY_REQUEST* p_pRequest)
	{
		CCloudNetDataApi* pThis = reinterpret_cast<CCloudNetDataApi*>(p_pParam);
		if (pThis == NULL)
		{
			return EN_BINARY_SERVER_CALLBACK_COMPLETED;
		}
		return pThis->HandleBinaryServerCallbackEx(p_pRequest);
	}

	// 控制请求分类由上层服务提供，CloudNetDataApi 和 IceRPCPush 均不硬编码功能号。
	static int BinaryPriorityClassifier(void* p_pParam, long long p_lFuncId)
	{
		CCloudNetDataApi* pThis = reinterpret_cast<CCloudNetDataApi*>(p_pParam);
		if (pThis == NULL)
		{
			return 0;
		}
		PFN_CLOUD_NET_BINARY_PRIORITY_CLASSIFIER pfnClassifier = NULL;
		void* pRequestParam = NULL;
		{
			std::lock_guard<std::mutex> clLock(pThis->m_clMutex);
			pfnClassifier = pThis->m_pfnBinaryPriorityClassifier;
			pRequestParam = pThis->m_pBinaryRequestParam;
		}
		if (pfnClassifier == NULL)
		{
			return 0;
		}
		try
		{
			return pfnClassifier(p_lFuncId, pRequestParam) != 0 ? 1 : 0;
		}
		catch (...)
		{
			return 0;
		}
	}

	// 处理底层 RPC/PUT 请求回调，调用上层业务回调并通过 IceRPCPush 回包。
	void HandleServerCallback(short p_chMode, long long p_lSetCode, ST_JSON_M_RESULT_TOP* p_pResult)
	{
		PFN_CLOUD_NET_REQUEST pfnRequest = NULL;
		void* pRequestParam = NULL;
		HANDLE hServer = NULL;
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			pfnRequest = m_pfnRequest;
			pRequestParam = m_pRequestParam;
			hServer = m_hServer;
			if (pfnRequest == NULL || p_pResult == NULL || hServer == NULL)
			{
				return;
			}
			++m_iActiveServiceCall;
		}
		CServiceCallGuard clServiceGuard(this);

		ST_CLOUD_NET_REQUEST stRequest = MakeRequest(p_chMode, p_lSetCode, p_pResult, pRequestParam);
		ST_CLOUD_NET_RESULT stResult;
		memset(&stResult, 0, sizeof(ST_CLOUD_NET_RESULT));
		stResult.lSynId = stRequest.lSynId;
		stResult.lFuncId = stRequest.lFuncId;
		try
		{
			pfnRequest(&stRequest, &stResult, pRequestParam);
		}
		catch (const std::exception& ex)
		{
			std::string strDetail = MakeCloudNetStdExceptionDetail("HandleServerCallback", ex);
			SetLastError(s_iCloudNetInternalError, strDetail.c_str(), "HandleServerCallback");
			stResult.lRetVal = s_iCloudNetInternalError;
			SafeCopyError(stResult.szErrInfo, sizeof(stResult.szErrInfo), strDetail.c_str());
		}
		catch (...)
		{
			std::string strDetail = MakeCloudNetUnknownExceptionDetail("HandleServerCallback");
			SetLastError(s_iCloudNetInternalError, strDetail.c_str(), "HandleServerCallback");
			stResult.lRetVal = s_iCloudNetInternalError;
			SafeCopyError(stResult.szErrInfo, sizeof(stResult.szErrInfo), strDetail.c_str());
		}
		FillJsonResult(*p_pResult, &stResult);
		try
		{
			JsonICEResponseData(hServer, p_pResult);
		}
		catch (const std::exception& ex)
		{
			SetLastError(s_iCloudNetInternalError, MakeCloudNetStdExceptionDetail("JsonICEResponseData", ex).c_str(), "JsonICEResponseData");
		}
		catch (...)
		{
			SetLastError(s_iCloudNetInternalError, MakeCloudNetUnknownExceptionDetail("JsonICEResponseData").c_str(), "JsonICEResponseData");
		}
	}

	// 执行上层 Binary 业务回调，并在回调返回前通过底层请求句柄完成应答。
	void HandleBinaryServerCallback(const ST_BINARY_REQUEST* p_pRequest)
	{
		PFN_CLOUD_NET_BINARY_REQUEST pfnRequest = NULL;
		void* pRequestParam = NULL;
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			pfnRequest = m_pfnBinaryRequest;
			pRequestParam = m_pBinaryRequestParam;
			if (pfnRequest == NULL || p_pRequest == NULL || p_pRequest->hResponse == NULL || m_hServer == NULL)
			{
				return;
			}
			++m_iActiveServiceCall;
		}
		CServiceCallGuard clServiceGuard(this);

		ST_CLOUD_NET_BINARY_REQUEST stRequest = MakeCloudBinaryRequest(*p_pRequest, pRequestParam);
		ST_CLOUD_NET_BINARY_RESULT stResult;
		stResult.lSynId = stRequest.lSynId;
		stResult.lFuncId = stRequest.lFuncId;
		try
		{
			pfnRequest(&stRequest, &stResult, pRequestParam);
		}
		catch (const std::exception& ex)
		{
			std::string strDetail = MakeCloudNetStdExceptionDetail("HandleBinaryServerCallback", ex);
			SetLastError(s_iCloudNetInternalError, strDetail.c_str(), "HandleBinaryServerCallback");
			stResult.lRetVal = s_iCloudNetInternalError;
			stResult.iErrorCode = s_iCloudNetInternalError;
			SafeCopyError(stResult.szErrInfo, sizeof(stResult.szErrInfo), strDetail.c_str());
		}
		catch (...)
		{
			std::string strDetail = MakeCloudNetUnknownExceptionDetail("HandleBinaryServerCallback");
			SetLastError(s_iCloudNetInternalError, strDetail.c_str(), "HandleBinaryServerCallback");
			stResult.lRetVal = s_iCloudNetInternalError;
			stResult.iErrorCode = s_iCloudNetInternalError;
			SafeCopyError(stResult.szErrInfo, sizeof(stResult.szErrInfo), strDetail.c_str());
		}

		ST_BINARY_RESULT stIceResult;
		FillIceBinaryResult(stIceResult, stResult);
		BinaryResponseData(p_pRequest->hResponse, &stIceResult);
		int iIceError = IceRPCPushGetLastErrorCode(NULL);
		if (iIceError != 0)
		{
			std::string strDetail = MakeIceRPCPushErrorDetail(NULL, "BINARY_RESPONSE_FAILED: BinaryResponseData rejected response");
			SetLastError(s_iCloudNetBinaryCallFailed, strDetail.c_str(), "HandleBinaryServerCallback");
		}
	}

	// Ex 回调在 DEFERRED 时只保留不透明应答句柄，业务必须自行深拷贝后续所需请求字段。
	int HandleBinaryServerCallbackEx(const ST_BINARY_REQUEST* p_pRequest)
	{
		PFN_CLOUD_NET_BINARY_REQUEST_EX pfnRequest = NULL;
		void* pRequestParam = NULL;
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			pfnRequest = m_pfnBinaryRequestEx;
			pRequestParam = m_pBinaryRequestParam;
			if (pfnRequest != NULL && p_pRequest != NULL &&
				p_pRequest->hResponse != NULL && m_hServer != NULL)
			{
				++m_iActiveServiceCall;
			}
			else
			{
				return EN_BINARY_SERVER_CALLBACK_COMPLETED;
			}
		}
		CServiceCallGuard clServiceGuard(this);

		ST_CLOUD_NET_BINARY_REQUEST stRequest =
			MakeCloudBinaryRequest(*p_pRequest, pRequestParam);
		ST_CLOUD_NET_BINARY_RESULT stResult;
		stResult.lSynId = stRequest.lSynId;
		stResult.lFuncId = stRequest.lFuncId;
		int iDisposition = EN_CLOUD_NET_BINARY_SERVER_CALLBACK_COMPLETED;
		try
		{
			iDisposition = pfnRequest(&stRequest, &stResult, pRequestParam);
		}
		catch (const std::exception& ex)
		{
			std::string strDetail = MakeCloudNetStdExceptionDetail(
				"HandleBinaryServerCallbackEx", ex);
			SetLastError(s_iCloudNetInternalError, strDetail.c_str(),
				"HandleBinaryServerCallbackEx");
			stResult.lRetVal = s_iCloudNetInternalError;
			stResult.iErrorCode = s_iCloudNetInternalError;
			SafeCopyError(stResult.szErrInfo, sizeof(stResult.szErrInfo),
				strDetail.c_str());
			iDisposition = EN_CLOUD_NET_BINARY_SERVER_CALLBACK_COMPLETED;
		}
		catch (...)
		{
			std::string strDetail = MakeCloudNetUnknownExceptionDetail(
				"HandleBinaryServerCallbackEx");
			SetLastError(s_iCloudNetInternalError, strDetail.c_str(),
				"HandleBinaryServerCallbackEx");
			stResult.lRetVal = s_iCloudNetInternalError;
			stResult.iErrorCode = s_iCloudNetInternalError;
			SafeCopyError(stResult.szErrInfo, sizeof(stResult.szErrInfo),
				strDetail.c_str());
			iDisposition = EN_CLOUD_NET_BINARY_SERVER_CALLBACK_COMPLETED;
		}

		if (iDisposition == EN_CLOUD_NET_BINARY_SERVER_CALLBACK_DEFERRED)
		{
			return EN_BINARY_SERVER_CALLBACK_DEFERRED;
		}
		if (iDisposition != EN_CLOUD_NET_BINARY_SERVER_CALLBACK_COMPLETED)
		{
			return iDisposition;
		}
		ST_BINARY_RESULT stIceResult;
		FillIceBinaryResult(stIceResult, stResult);
		return BinaryResponseDataEx(p_pRequest->hResponse, &stIceResult) != 0 ?
			EN_BINARY_SERVER_CALLBACK_COMPLETED :
			EN_BINARY_SERVER_CALLBACK_COMPLETED;
	}

private:
	mutable std::mutex m_clMutex;                              // 保护服务句柄、连接表、配置和后台线程状态。
	mutable std::mutex m_clErrorMutex;                         // 保护最后错误码和错误描述，避免多线程查询时读写竞争。
	std::condition_variable m_clServiceIdleCond;                // StopService 等待服务端回调全部结束的条件变量。
	std::condition_variable m_clNetworkCond;                    // 后台网络维护线程等待重连周期或停止信号的条件变量。
	std::atomic_bool m_bClosing;                                // 实例析构关闭标记，避免析构过程中继续进入新操作。
	std::atomic_bool m_bNetworkStop;                            // 后台网络维护线程停止标记。
	bool m_bNetworkStarted;                                     // 组网模式是否已经启动，用于控制维护线程执行逻辑。
	int m_iActiveServiceCall;                                   // 当前正在执行的服务端回调数量。
	HANDLE m_hServer;                                           // 本地 IceRPCPush 服务端句柄，由 StartService 创建。
	HANDLE m_hServerSem;                                        // 底层服务端信号量句柄，仅随服务端生命周期保存。
	PFN_CLOUD_NET_REQUEST m_pfnRequest;                         // 上层注册的服务端请求处理回调。
	void* m_pRequestParam;                                      // 服务端请求回调透传参数，生命周期由上层维护。
	PFN_CLOUD_NET_BINARY_REQUEST m_pfnBinaryRequest;            // 上层注册的 Binary 服务端请求处理回调。
	PFN_CLOUD_NET_BINARY_REQUEST_EX m_pfnBinaryRequestEx;       // 支持延迟应答的 Binary 服务端回调。
	PFN_CLOUD_NET_BINARY_PRIORITY_CLASSIFIER m_pfnBinaryPriorityClassifier; // 控制请求分类器。
	void* m_pBinaryRequestParam;                                // Binary 请求回调透传参数，生命周期由上层维护。
	PFN_CLOUD_NET_PUSH m_pfnNetworkPush;                        // 组网模式下统一使用的推送回调。
	void* m_pNetworkParam;                                      // 组网模式推送回调透传参数，生命周期由上层维护。
	ST_CLOUD_NET_CONFIG m_stConfig;                             // 当前实例加载成功的 CloudNetDataApi 组网配置。
	std::string m_strServerRuntimeConfigPath;                   // 为当前服务端生成的临时 IceRPCPush 运行时配置路径。
	std::thread m_clNetworkThread;                              // 后台重连和 RegisterPush 续约维护线程。
	std::map<std::string, ST_CONNECTION_CONTEXT_PTR> m_mapConnection; // 按连接名索引的远端服务连接表。
	std::map<std::string, std::chrono::steady_clock::time_point> m_mapRenewTime; // 每个订阅下一次允许续约的时间点。
	std::set<std::string> m_setDependencyPending;                // 正在等待完成的连接或订阅步骤键。
	std::set<std::string> m_setDependencyEverReady;              // 本进程内至少成功过一次的依赖步骤键。
	std::map<std::string, std::string> m_mapDependencyError;      // 每个依赖最近一次已记录的失败原因。
	std::map<std::string, std::chrono::steady_clock::time_point> m_mapDependencyLogTime; // 每个依赖最近一次等待日志时间。
	std::vector<ST_CONNECTION_CONTEXT_PTR> m_aRetiredConnection; // 已关闭但仍需保留一段生命周期的连接上下文。
	int m_iLastErrorCode;                                       // 当前实例最后一次错误码。
	std::string m_strLastError;                                 // 当前实例最后一次 ASCII 错误描述。
};

void SetCloudNetBoundaryError(CCloudNetDataApi* p_pApi, int p_iErrorCode, const std::string& p_strDetail, const char* p_szFunction)
{
	if (p_pApi != NULL)
	{
		p_pApi->SetBoundaryError(p_iErrorCode, p_strDetail, p_szFunction);
	}
	else
	{
		SetCloudNetBoundaryThreadError(p_iErrorCode, p_strDetail, p_szFunction);
	}
}

extern "C"
{
HCLOUD_NET_API Create()
{
	try
	{
		CCloudNetDataApi* pApi = new (std::nothrow) CCloudNetDataApi;
		if (pApi == NULL)
		{
			SetCloudNetBoundaryThreadError(s_iCloudNetMemoryError, "MEMORY_ERROR: CCloudNetDataApi allocation failed", __FUNCTION__);
			return NULL;
		}
		SetThreadLastError(s_iCloudNetOk, GetCloudNetErrorMsg(s_iCloudNetOk));
		return reinterpret_cast<HCLOUD_NET_API>(pApi);
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryThreadError(s_iCloudNetInternalError, MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryThreadError(s_iCloudNetInternalError, MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
	return NULL;
}

int LoadConfig(HCLOUD_NET_API p_hApi, const char* p_szXmlPath, const char* p_szPluginId)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	try
	{
		if (pApi == NULL)
		{
			SetCloudNetBoundaryError(NULL, s_iCloudNetInvalidParam, "INVALID_HANDLE: CloudNetDataApi handle is null", __FUNCTION__);
			return s_iCloudNetInvalidParam;
		}
		return pApi->LoadConfig(p_szXmlPath, p_szPluginId);
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
	return s_iCloudNetInternalError;
}

int ReloadConfig(HCLOUD_NET_API p_hApi)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	try
	{
		if (pApi == NULL)
		{
			SetCloudNetBoundaryError(NULL, s_iCloudNetInvalidParam, "INVALID_HANDLE: CloudNetDataApi handle is null", __FUNCTION__);
			return s_iCloudNetInvalidParam;
		}
		return pApi->ReloadConfig();
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
	return s_iCloudNetInternalError;
}

int StartNetwork(HCLOUD_NET_API p_hApi, PFN_CLOUD_NET_REQUEST p_pfnRequest, PFN_CLOUD_NET_PUSH p_pfnPush, void* p_pParam)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	try
	{
		if (pApi == NULL)
		{
			SetCloudNetBoundaryError(NULL, s_iCloudNetInvalidParam, "INVALID_HANDLE: CloudNetDataApi handle is null", __FUNCTION__);
			return s_iCloudNetInvalidParam;
		}
		return pApi->StartNetwork(p_pfnRequest, p_pfnPush, p_pParam);
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
	return s_iCloudNetInternalError;
}

int StartBinaryNetwork(HCLOUD_NET_API p_hApi, PFN_CLOUD_NET_BINARY_REQUEST p_pfnRequest, PFN_CLOUD_NET_PUSH p_pfnPush, void* p_pParam)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	try
	{
		if (pApi == NULL)
		{
			SetCloudNetBoundaryError(NULL, s_iCloudNetInvalidParam, "INVALID_HANDLE: CloudNetDataApi handle is null", __FUNCTION__);
			return s_iCloudNetInvalidParam;
		}
		return pApi->StartBinaryNetwork(p_pfnRequest, p_pfnPush, p_pParam);
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
	return s_iCloudNetInternalError;
}

int StartBinaryNetworkEx(HCLOUD_NET_API p_hApi,
	PFN_CLOUD_NET_BINARY_REQUEST_EX p_pfnRequest,
	PFN_CLOUD_NET_BINARY_PRIORITY_CLASSIFIER p_pfnPriorityClassifier,
	PFN_CLOUD_NET_PUSH p_pfnPush, void* p_pParam)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	try
	{
		if (pApi == NULL)
		{
			SetCloudNetBoundaryError(NULL, s_iCloudNetInvalidParam,
				"INVALID_HANDLE: CloudNetDataApi handle is null", __FUNCTION__);
			return s_iCloudNetInvalidParam;
		}
		return pApi->StartBinaryNetworkEx(p_pfnRequest,
			p_pfnPriorityClassifier, p_pfnPush, p_pParam);
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError,
			MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError,
			MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
	return s_iCloudNetInternalError;
}

int StopNetwork(HCLOUD_NET_API p_hApi)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	try
	{
		if (pApi == NULL)
		{
			SetCloudNetBoundaryError(NULL, s_iCloudNetInvalidParam, "INVALID_HANDLE: CloudNetDataApi handle is null", __FUNCTION__);
			return s_iCloudNetInvalidParam;
		}
		return pApi->StopNetwork();
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
	return s_iCloudNetInternalError;
}

long long Publish(HCLOUD_NET_API p_hApi, long long p_lReqNo, const char* p_pBuf, long p_lBufLen, int p_iAsync)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	try
	{
		if (pApi == NULL)
		{
			SetCloudNetBoundaryError(NULL, s_iCloudNetInvalidParam, "INVALID_HANDLE: CloudNetDataApi handle is null", __FUNCTION__);
			return s_iCloudNetInvalidParam;
		}
		return pApi->Publish(p_lReqNo, p_pBuf, p_lBufLen, p_iAsync);
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
	return s_iCloudNetInternalError;
}

void Destroy(HCLOUD_NET_API p_hApi)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	try
	{
		if (pApi == NULL)
		{
			SetCloudNetBoundaryError(NULL, s_iCloudNetInvalidParam, "INVALID_HANDLE: CloudNetDataApi handle is null", __FUNCTION__);
			return;
		}
		delete pApi;
		SetThreadLastError(s_iCloudNetOk, GetCloudNetErrorMsg(s_iCloudNetOk));
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryError(NULL, s_iCloudNetInternalError, MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryError(NULL, s_iCloudNetInternalError, MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
}

int StartService(HCLOUD_NET_API p_hApi, const char* p_szXmlPath, const char* p_szServiceName, int p_iSnappyCompress, PFN_CLOUD_NET_REQUEST p_pfnCallback, void* p_pParam)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	try
	{
		if (pApi == NULL)
		{
			SetCloudNetBoundaryError(NULL, s_iCloudNetInvalidParam, "INVALID_HANDLE: CloudNetDataApi handle is null", __FUNCTION__);
			return s_iCloudNetInvalidParam;
		}
		return pApi->StartService(p_szXmlPath, p_szServiceName, p_iSnappyCompress, p_pfnCallback, p_pParam);
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
	return s_iCloudNetInternalError;
}

int StartBinaryService(HCLOUD_NET_API p_hApi, const char* p_szXmlPath, const char* p_szServiceName, int p_iSnappyCompress, PFN_CLOUD_NET_BINARY_REQUEST p_pfnCallback, void* p_pParam)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	try
	{
		if (pApi == NULL)
		{
			SetCloudNetBoundaryError(NULL, s_iCloudNetInvalidParam, "INVALID_HANDLE: CloudNetDataApi handle is null", __FUNCTION__);
			return s_iCloudNetInvalidParam;
		}
		return pApi->StartBinaryService(p_szXmlPath, p_szServiceName, p_iSnappyCompress, p_pfnCallback, p_pParam);
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
	return s_iCloudNetInternalError;
}

int StartBinaryServiceEx(HCLOUD_NET_API p_hApi, const char* p_szXmlPath,
	const char* p_szServiceName, int p_iSnappyCompress,
	PFN_CLOUD_NET_BINARY_REQUEST_EX p_pfnCallback,
	PFN_CLOUD_NET_BINARY_PRIORITY_CLASSIFIER p_pfnPriorityClassifier,
	void* p_pParam)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	try
	{
		if (pApi == NULL)
		{
			SetCloudNetBoundaryError(NULL, s_iCloudNetInvalidParam,
				"INVALID_HANDLE: CloudNetDataApi handle is null", __FUNCTION__);
			return s_iCloudNetInvalidParam;
		}
		return pApi->StartBinaryServiceEx(p_szXmlPath, p_szServiceName,
			p_iSnappyCompress, p_pfnCallback, p_pfnPriorityClassifier,
			p_pParam);
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError,
			MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError,
			MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
	return s_iCloudNetInternalError;
}

int StopService(HCLOUD_NET_API p_hApi)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	try
	{
		if (pApi == NULL)
		{
			SetCloudNetBoundaryError(NULL, s_iCloudNetInvalidParam, "INVALID_HANDLE: CloudNetDataApi handle is null", __FUNCTION__);
			return s_iCloudNetInvalidParam;
		}
		return pApi->StopService();
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
	return s_iCloudNetInternalError;
}

int Connect(HCLOUD_NET_API p_hApi, const char* p_szXmlPath, const char* p_szProxyName, const char* p_szConnName, int p_iThreadPool)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	try
	{
		if (pApi == NULL)
		{
			SetCloudNetBoundaryError(NULL, s_iCloudNetInvalidParam, "INVALID_HANDLE: CloudNetDataApi handle is null", __FUNCTION__);
			return s_iCloudNetInvalidParam;
		}
		return pApi->Connect(p_szXmlPath, p_szProxyName, p_szConnName, p_iThreadPool);
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
	return s_iCloudNetInternalError;
}

int Disconnect(HCLOUD_NET_API p_hApi, const char* p_szConnName)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	try
	{
		if (pApi == NULL)
		{
			SetCloudNetBoundaryError(NULL, s_iCloudNetInvalidParam, "INVALID_HANDLE: CloudNetDataApi handle is null", __FUNCTION__);
			return s_iCloudNetInvalidParam;
		}
		return pApi->Disconnect(p_szConnName);
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
	return s_iCloudNetInternalError;
}

int RegisterPush(HCLOUD_NET_API p_hApi, const char* p_szConnName, const char* p_szPluginId, const char* p_szSubInfo, PFN_CLOUD_NET_PUSH p_pfnCallback, void* p_pParam)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	try
	{
		if (pApi == NULL)
		{
			SetCloudNetBoundaryError(NULL, s_iCloudNetInvalidParam, "INVALID_HANDLE: CloudNetDataApi handle is null", __FUNCTION__);
			return s_iCloudNetInvalidParam;
		}
		return pApi->RegisterPush(p_szConnName, p_szPluginId, p_szSubInfo, p_pfnCallback, p_pParam);
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
	return s_iCloudNetInternalError;
}

int UnregisterPush(HCLOUD_NET_API p_hApi, const char* p_szConnName, const char* p_szPluginId, const char* p_szSubInfo)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	try
	{
		if (pApi == NULL)
		{
			SetCloudNetBoundaryError(NULL, s_iCloudNetInvalidParam, "INVALID_HANDLE: CloudNetDataApi handle is null", __FUNCTION__);
			return s_iCloudNetInvalidParam;
		}
		return pApi->UnregisterPush(p_szConnName, p_szPluginId, p_szSubInfo);
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
	return s_iCloudNetInternalError;
}

ST_CLOUD_NET_RESULT* CallSync(HCLOUD_NET_API p_hApi, const char* p_szConnName, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lJsonLen)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	try
	{
		if (pApi == NULL)
		{
			SetCloudNetBoundaryError(NULL, s_iCloudNetInvalidParam, "INVALID_HANDLE: CloudNetDataApi handle is null", __FUNCTION__);
			return NULL;
		}
		return pApi->CallSync(p_szConnName, p_lSynId, p_lFuncId, p_lSetCode, p_szJsonReq, p_lJsonLen);
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
	return NULL;
}

long long CallAsync(HCLOUD_NET_API p_hApi, const char* p_szConnName, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lJsonLen, PFN_CLOUD_NET_ASYNC p_pfnCallback, void* p_pParam)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	try
	{
		if (pApi == NULL)
		{
			SetCloudNetBoundaryError(NULL, s_iCloudNetInvalidParam, "INVALID_HANDLE: CloudNetDataApi handle is null", __FUNCTION__);
			return s_iCloudNetInvalidParam;
		}
		return pApi->CallAsync(p_szConnName, p_lSynId, p_lFuncId, p_lSetCode, p_szJsonReq, p_lJsonLen, p_pfnCallback, p_pParam);
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
	return s_iCloudNetInternalError;
}

ST_CLOUD_NET_RESULT* PutSync(HCLOUD_NET_API p_hApi, const char* p_szConnName, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lJsonLen, const ST_CLOUD_NET_RESULT* p_pPutData)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	try
	{
		if (pApi == NULL)
		{
			SetCloudNetBoundaryError(NULL, s_iCloudNetInvalidParam, "INVALID_HANDLE: CloudNetDataApi handle is null", __FUNCTION__);
			return NULL;
		}
		return pApi->PutSync(p_szConnName, p_lSynId, p_lFuncId, p_lSetCode, p_szJsonReq, p_lJsonLen, p_pPutData);
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
	return NULL;
}

long long PutAsync(HCLOUD_NET_API p_hApi, const char* p_szConnName, long long p_lSynId, long long p_lFuncId, long long p_lSetCode, const char* p_szJsonReq, long p_lJsonLen, const ST_CLOUD_NET_RESULT* p_pPutData, PFN_CLOUD_NET_ASYNC p_pfnCallback, void* p_pParam)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	try
	{
		if (pApi == NULL)
		{
			SetCloudNetBoundaryError(NULL, s_iCloudNetInvalidParam, "INVALID_HANDLE: CloudNetDataApi handle is null", __FUNCTION__);
			return s_iCloudNetInvalidParam;
		}
		return pApi->PutAsync(p_szConnName, p_lSynId, p_lFuncId, p_lSetCode, p_szJsonReq, p_lJsonLen, p_pPutData, p_pfnCallback, p_pParam);
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
	return s_iCloudNetInternalError;
}

ST_CLOUD_NET_BINARY_RESULT* CallBinarySync(HCLOUD_NET_API p_hApi, const char* p_szConnName, const ST_CLOUD_NET_BINARY_CALL* p_pCall)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	try
	{
		if (pApi == NULL)
		{
			SetCloudNetBoundaryError(NULL, s_iCloudNetInvalidParam, "INVALID_HANDLE: CloudNetDataApi handle is null", __FUNCTION__);
			return NULL;
		}
		return pApi->CallBinarySync(p_szConnName, p_pCall);
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
	return NULL;
}

long long CallBinaryAsync(HCLOUD_NET_API p_hApi, const char* p_szConnName, const ST_CLOUD_NET_BINARY_CALL* p_pCall, PFN_CLOUD_NET_BINARY_ASYNC p_pfnCallback, void* p_pParam)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	try
	{
		if (pApi == NULL)
		{
			SetCloudNetBoundaryError(NULL, s_iCloudNetInvalidParam, "INVALID_HANDLE: CloudNetDataApi handle is null", __FUNCTION__);
			return s_iCloudNetInvalidParam;
		}
		return pApi->CallBinaryAsync(p_szConnName, p_pCall, p_pfnCallback, p_pParam);
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
	return s_iCloudNetInternalError;
}

long long CallBinaryAsyncEx(HCLOUD_NET_API p_hApi, const char* p_szConnName,
	const ST_CLOUD_NET_BINARY_CALL* p_pCall, int p_iTimeoutMs,
	PFN_CLOUD_NET_BINARY_ASYNC p_pfnCallback, void* p_pParam)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	try
	{
		if (pApi == NULL || p_iTimeoutMs < 0)
		{
			SetCloudNetBoundaryError(pApi, s_iCloudNetInvalidParam,
				"INVALID_PARAM: CloudNetDataApi handle is null or timeout is negative",
				__FUNCTION__);
			return s_iCloudNetInvalidParam;
		}
		return pApi->CallBinaryAsyncEx(p_szConnName, p_pCall,
			p_iTimeoutMs, p_pfnCallback, p_pParam);
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError,
			MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError,
			MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
	return s_iCloudNetInternalError;
}

ST_CLOUD_NET_BINARY_RESULT* PutBinarySync(HCLOUD_NET_API p_hApi, const char* p_szConnName, const ST_CLOUD_NET_BINARY_CALL* p_pCall)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	try
	{
		if (pApi == NULL)
		{
			SetCloudNetBoundaryError(NULL, s_iCloudNetInvalidParam, "INVALID_HANDLE: CloudNetDataApi handle is null", __FUNCTION__);
			return NULL;
		}
		return pApi->PutBinarySync(p_szConnName, p_pCall);
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
	return NULL;
}

long long PutBinaryAsync(HCLOUD_NET_API p_hApi, const char* p_szConnName, const ST_CLOUD_NET_BINARY_CALL* p_pCall, PFN_CLOUD_NET_BINARY_ASYNC p_pfnCallback, void* p_pParam)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	try
	{
		if (pApi == NULL)
		{
			SetCloudNetBoundaryError(NULL, s_iCloudNetInvalidParam, "INVALID_HANDLE: CloudNetDataApi handle is null", __FUNCTION__);
			return s_iCloudNetInvalidParam;
		}
		return pApi->PutBinaryAsync(p_szConnName, p_pCall, p_pfnCallback, p_pParam);
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError, MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
	return s_iCloudNetInternalError;
}

long long PutBinaryAsyncEx(HCLOUD_NET_API p_hApi, const char* p_szConnName,
	const ST_CLOUD_NET_BINARY_CALL* p_pCall, int p_iTimeoutMs,
	PFN_CLOUD_NET_BINARY_ASYNC p_pfnCallback, void* p_pParam)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	try
	{
		if (pApi == NULL || p_iTimeoutMs < 0)
		{
			SetCloudNetBoundaryError(pApi, s_iCloudNetInvalidParam,
				"INVALID_PARAM: CloudNetDataApi handle is null or timeout is negative",
				__FUNCTION__);
			return s_iCloudNetInvalidParam;
		}
		return pApi->PutBinaryAsyncEx(p_szConnName, p_pCall,
			p_iTimeoutMs, p_pfnCallback, p_pParam);
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError,
			MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError,
			MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
	return s_iCloudNetInternalError;
}

long long CallBinaryEncodedAsyncEx(HCLOUD_NET_API p_hApi,
	const char* p_szConnName, const ST_CLOUD_NET_BINARY_CALL* p_pCall,
	int p_iTimeoutMs, PFN_CLOUD_NET_BINARY_ENCODED_ASYNC p_pfnCallback,
	void* p_pParam)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	try
	{
		if (pApi == NULL || p_iTimeoutMs < 0 || p_pfnCallback == NULL)
		{
			SetCloudNetBoundaryError(pApi, s_iCloudNetInvalidParam,
				"INVALID_PARAM: handle, encoded callback or timeout is invalid",
				__FUNCTION__);
			return s_iCloudNetInvalidParam;
		}
		return pApi->CallBinaryEncodedAsyncEx(p_szConnName, p_pCall,
			p_iTimeoutMs, p_pfnCallback, p_pParam);
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError,
			MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError,
			MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
	return s_iCloudNetInternalError;
}

long long PutBinaryEncodedAsyncEx(HCLOUD_NET_API p_hApi,
	const char* p_szConnName, const ST_CLOUD_NET_BINARY_CALL* p_pCall,
	int p_iTimeoutMs, PFN_CLOUD_NET_BINARY_ENCODED_ASYNC p_pfnCallback,
	void* p_pParam)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	try
	{
		if (pApi == NULL || p_iTimeoutMs < 0 || p_pfnCallback == NULL)
		{
			SetCloudNetBoundaryError(pApi, s_iCloudNetInvalidParam,
				"INVALID_PARAM: handle, encoded callback or timeout is invalid",
				__FUNCTION__);
			return s_iCloudNetInvalidParam;
		}
		return pApi->PutBinaryEncodedAsyncEx(p_szConnName, p_pCall,
			p_iTimeoutMs, p_pfnCallback, p_pParam);
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError,
			MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryError(pApi, s_iCloudNetInternalError,
			MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
	return s_iCloudNetInternalError;
}

int CompleteBinaryResponse(HCLOUD_NET_BINARY_RESPONSE p_hResponse,
	const ST_CLOUD_NET_BINARY_RESULT* p_pResult)
{
	try
	{
		if (p_hResponse == NULL || p_pResult == NULL)
		{
			SetCloudNetBoundaryError(NULL, s_iCloudNetInvalidParam,
				"INVALID_PARAM: binary response handle or result is null", __FUNCTION__);
			return 0;
		}
		ST_BINARY_RESULT stIceResult;
		FillIceBinaryResult(stIceResult, *p_pResult);
		const int iRet = BinaryResponseDataEx(p_hResponse, &stIceResult);
		if (iRet == 0)
		{
			SetCloudNetBoundaryError(NULL, s_iCloudNetStateError,
				MakeIceRPCPushErrorDetail(NULL,
					"BINARY_RESPONSE_FAILED: response handle is completed or expired"),
				__FUNCTION__);
		}
		return iRet;
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryError(NULL, s_iCloudNetInternalError,
			MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryError(NULL, s_iCloudNetInternalError,
			MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
	return 0;
}

int CancelBinaryResponse(HCLOUD_NET_BINARY_RESPONSE p_hResponse,
	int p_iErrorCode, const char* p_szError)
{
	ST_CLOUD_NET_BINARY_RESULT stResult;
	stResult.lRetVal = p_iErrorCode;
	stResult.iErrorCode = p_iErrorCode;
	SafeCopyError(stResult.szErrInfo, sizeof(stResult.szErrInfo),
		p_szError != NULL ? p_szError : "BINARY_RESPONSE_CANCELLED: request was cancelled");
	return CompleteBinaryResponse(p_hResponse, &stResult);
}

void FreeResult(ST_CLOUD_NET_RESULT* p_pResult)
{
	try
	{
		ReleaseResult(p_pResult);
		SetThreadLastError(s_iCloudNetOk, GetCloudNetErrorMsg(s_iCloudNetOk));
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryError(NULL, s_iCloudNetInternalError, MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryError(NULL, s_iCloudNetInternalError, MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
}

void FreeBinaryResult(ST_CLOUD_NET_BINARY_RESULT* p_pResult)
{
	try
	{
		ReleaseCloudBinaryResult(p_pResult);
		SetThreadLastError(s_iCloudNetOk, GetCloudNetErrorMsg(s_iCloudNetOk));
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryError(NULL, s_iCloudNetInternalError, MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryError(NULL, s_iCloudNetInternalError, MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
}

int DecodeBinaryEncodedBuffer(
	const ST_CLOUD_NET_BINARY_ENCODED_BUFFER* p_pEncoded,
	ST_CLOUD_NET_BINARY_BUFFER* p_pDecoded, char* p_szError,
	int p_iErrorCapacity)
{
	if (p_pDecoded != NULL)
	{
		p_pDecoded->iLen = 0;
		p_pDecoded->pBuffer = NULL;
	}
	if (p_pEncoded == NULL || p_pDecoded == NULL ||
		p_iErrorCapacity < 0 ||
		(p_iErrorCapacity > 0 && p_szError == NULL))
	{
		const char* pError =
			"INVALID_PARAM: encoded buffer, output buffer or error capacity is invalid";
		if (p_szError != NULL && p_iErrorCapacity > 0)
		{
			strncpy_s(p_szError, static_cast<size_t>(p_iErrorCapacity),
				pError, _TRUNCATE);
		}
		SetThreadLastError(s_iCloudNetInvalidParam, pError);
		return 0;
	}

	ST_BINARY_ENCODED_BUFFER stIceEncoded;
	stIceEncoded.iVersion = p_pEncoded->iVersion;
	stIceEncoded.iCompression = p_pEncoded->iCompression;
	stIceEncoded.iRawSize = p_pEncoded->iRawSize;
	stIceEncoded.iWireSize = p_pEncoded->iWireSize;
	stIceEncoded.iMaxPayloadBytes = p_pEncoded->iMaxPayloadBytes;
	stIceEncoded.pBuffer = p_pEncoded->pBuffer;
	ST_BINARY_BUFFER stIceDecoded;
	const int iRet = BinaryDecodeEncodedBuffer(&stIceEncoded, &stIceDecoded,
		p_szError, p_iErrorCapacity);
	if (iRet == 0)
	{
		SetThreadLastError(s_iCloudNetBinaryProtocolError,
			p_szError != NULL && p_szError[0] != '\0' ? p_szError :
			IceRPCPushGetLastError(NULL));
		return 0;
	}
	p_pDecoded->iLen = stIceDecoded.lLen;
	p_pDecoded->pBuffer = stIceDecoded.pBuffer;
	SetThreadLastError(s_iCloudNetOk, GetCloudNetErrorMsg(s_iCloudNetOk));
	return 1;
}

void FreeBinaryBuffer(ST_CLOUD_NET_BINARY_BUFFER* p_pBuffer)
{
	if (p_pBuffer == NULL)
	{
		return;
	}
	ST_BINARY_BUFFER stIceBuffer;
	stIceBuffer.lLen = p_pBuffer->iLen;
	stIceBuffer.pBuffer = p_pBuffer->pBuffer;
	BinaryBufferFree(&stIceBuffer);
	p_pBuffer->iLen = 0;
	p_pBuffer->pBuffer = NULL;
	SetThreadLastError(s_iCloudNetOk, GetCloudNetErrorMsg(s_iCloudNetOk));
}

void FreeBinaryEncodedResult(
	ST_CLOUD_NET_BINARY_ENCODED_RESULT* p_pResult)
{
	ReleaseCloudBinaryEncodedResult(p_pResult);
	SetThreadLastError(s_iCloudNetOk, GetCloudNetErrorMsg(s_iCloudNetOk));
}

const char* GetLastErrorDetail(HCLOUD_NET_API p_hApi)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	try
	{
		if (pApi == NULL)
		{
			// 空句柄用于查询当前线程最后一次错误，不能覆盖 Create 失败等无句柄错误。
			return s_strThreadLastError.c_str();
		}
		return pApi->GetLastErrorText();
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryError(NULL, s_iCloudNetInternalError, MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryError(NULL, s_iCloudNetInternalError, MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
	return s_strThreadLastError.c_str();
}

int GetLastErrorCode(HCLOUD_NET_API p_hApi)
{
	CCloudNetDataApi* pApi = reinterpret_cast<CCloudNetDataApi*>(p_hApi);
	try
	{
		if (pApi == NULL)
		{
			// 空句柄用于查询当前线程最后一次错误，不能覆盖 Create 失败等无句柄错误。
			return s_iThreadLastErrorCode;
		}
		return pApi->GetLastErrorCodeValue();
	}
	catch (const std::exception& ex)
	{
		SetCloudNetBoundaryError(NULL, s_iCloudNetInternalError, MakeCloudNetStdExceptionDetail(__FUNCTION__, ex), __FUNCTION__);
	}
	catch (...)
	{
		SetCloudNetBoundaryError(NULL, s_iCloudNetInternalError, MakeCloudNetUnknownExceptionDetail(__FUNCTION__), __FUNCTION__);
	}
	return s_iCloudNetInternalError;
}

const char* GetErrorMsg(int p_iErrorCode)
{
	try
	{
		return GetCloudNetErrorMsg(p_iErrorCode);
	}
	catch (...)
	{
		return "INTERNAL_ERROR: failed to query error message";
	}
}

}

