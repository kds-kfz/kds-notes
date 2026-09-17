#include "MtEventConfig.h"

#include "tinyxml.h"

#include <cstdlib>
#include <limits>

namespace
{
	// 读取范围受限的无符号整数属性，缺失属性使用构造函数默认值。
	bool ReadOptionalUInt(TiXmlElement* p_pElement,
		const char* p_szNodeName,
		const char* p_szAttribute, unsigned int p_uiMin,
		unsigned int p_uiMax, unsigned int& p_refValue,
		std::string& p_refError)
	{
		const char* pValue = p_pElement != nullptr ?
			p_pElement->Attribute(p_szAttribute) : nullptr;
		if (pValue == nullptr || pValue[0] == '\0')
		{
			return true;
		}
		char* pEnd = nullptr;
		const unsigned long ulValue = std::strtoul(
			pValue, &pEnd, 10);
		if (pEnd == pValue || pEnd == nullptr ||
			*pEnd != '\0' || ulValue < p_uiMin ||
			ulValue > p_uiMax)
		{
			p_refError =
				std::string("EVENT_CONFIG_INTEGER_INVALID: node=") +
				p_szNodeName + ", attribute=" +
				p_szAttribute + ", value=" + pValue;
			return false;
		}
		p_refValue = static_cast<unsigned int>(ulValue);
		return true;
	}

	// 读取范围受限的 64 位整数，单位由对应属性注释定义。
	bool ReadOptionalInt64(TiXmlElement* p_pElement,
		const char* p_szNodeName,
		const char* p_szAttribute,
		std::int64_t p_llMin,
		std::int64_t p_llMax,
		std::int64_t& p_refValue,
		std::string& p_refError)
	{
		const char* pValue = p_pElement != nullptr ?
			p_pElement->Attribute(p_szAttribute) :
			nullptr;
		if (pValue == nullptr || pValue[0] == '\0')
		{
			return true;
		}
		char* pEnd = nullptr;
		const long long llValue =
			std::strtoll(pValue, &pEnd, 10);
		if (pEnd == pValue || pEnd == nullptr ||
			*pEnd != '\0' || llValue < p_llMin ||
			llValue > p_llMax)
		{
			p_refError =
				std::string("EVENT_CONFIG_INTEGER_INVALID: node=") +
				p_szNodeName + ", attribute=" +
				p_szAttribute + ", value=" + pValue;
			return false;
		}
		p_refValue =
			static_cast<std::int64_t>(llValue);
		return true;
	}

	// 读取必填非空字符串；错误中保留配置位置但不输出敏感值。
	bool ReadRequiredString(TiXmlElement* p_pElement,
		const char* p_szNodeName,
		const char* p_szAttribute,
		std::string& p_refValue,
		std::string& p_refError)
	{
		const char* pValue = p_pElement != nullptr ?
			p_pElement->Attribute(p_szAttribute) :
			nullptr;
		if (pValue == nullptr || pValue[0] == '\0')
		{
			p_refError =
				std::string("EVENT_CONFIG_ATTRIBUTE_MISSING: node=") +
				p_szNodeName + ", attribute=" +
				p_szAttribute;
			return false;
		}
		p_refValue = pValue;
		return true;
	}

	// URL 可通过环境变量提供，避免生产凭据写入 XML。
	bool ReadUrl(TiXmlElement* p_pElement,
		std::string& p_refValue,
		std::string& p_refError)
	{
		const char* pEnvironment = p_pElement != nullptr ?
			p_pElement->Attribute("urlEnv") : nullptr;
		if (pEnvironment == nullptr ||
			pEnvironment[0] == '\0')
		{
			return ReadRequiredString(p_pElement,
				"MtEventService.ReliableEvent",
				"url", p_refValue, p_refError);
		}
		char* pValue = nullptr;
		std::size_t szValueLen = 0;
		const errno_t iRet = _dupenv_s(&pValue,
			&szValueLen, pEnvironment);
		if (iRet != 0 || pValue == nullptr ||
			szValueLen <= 1)
		{
			std::free(pValue);
			p_refError =
				std::string("EVENT_CONFIG_ENV_MISSING: environment=") +
				pEnvironment;
			return false;
		}
		p_refValue = pValue;
		std::free(pValue);
		return true;
	}
}

ST_MT_MARKET_EVENT_CONFIG::ST_MT_MARKET_EVENT_CONFIG()
	: uiWorkerThreads(4)
	, szQueueCapacity(0)
	, uiDropLogIntervalMs(10000)
{
}

ST_MT_EVENT_BROADCAST_CONFIG::ST_MT_EVENT_BROADCAST_CONFIG()
	: bEnable(false)
	, strSubject("mt.event.broadcast")
	, szMaxPayloadBytes(8U * 1024U * 1024U)
	, szDedupeCapacity(500000)
	, uiPendingMessages(200000)
	, uiPendingBytes(256U * 1024U * 1024U)
{
}

ST_MT_RELIABLE_EVENT_CONFIG::
	ST_MT_RELIABLE_EVENT_CONFIG()
	: bEnable(false)
	, strUrl("nats://127.0.0.1:4222")
	, strStream("MT_TRADING_EVENTS")
	, strSubjectPrefix("mt.trade")
	, llMaxAgeSeconds(604800)
	, llMaxBytes(10737418240LL)
	, llDuplicateWindowSeconds(86400)
	, uiReplicas(1)
	, uiFetchBatchMax(256)
	, uiFetchWaitMs(1000)
	, uiMaxDeliver(20)
{
}

ST_MT_EVENT_SERVICE_CONFIG::
	ST_MT_EVENT_SERVICE_CONFIG()
	: stMarketEvent()
	, stBroadcast()
	, stReliableEvent()
{
}

bool LoadMtEventServiceConfig(const std::string& p_refConfigPath,
	ST_MT_EVENT_SERVICE_CONFIG& p_refConfig,
	std::string& p_refError)
{
	p_refConfig = ST_MT_EVENT_SERVICE_CONFIG();
	p_refError.clear();
	TiXmlDocument clDocument(p_refConfigPath.c_str());
	if (!clDocument.LoadFile())
	{
		p_refError = "EVENT_CONFIG_XML_LOAD_FAILED: path=" +
			p_refConfigPath + ", detail=" + clDocument.ErrorDesc();
		return false;
	}
	TiXmlElement* pRoot = clDocument.RootElement();
	TiXmlElement* pEvent = pRoot != nullptr ?
		pRoot->FirstChildElement("MtEventService") : nullptr;
	TiXmlElement* pMarketEvent = pEvent != nullptr ?
		pEvent->FirstChildElement("MarketEvent") : nullptr;
	TiXmlElement* pBroadcast = pEvent != nullptr ?
		pEvent->FirstChildElement("Broadcast") : nullptr;
	TiXmlElement* pReliableEvent = pEvent != nullptr ?
		pEvent->FirstChildElement("ReliableEvent") : nullptr;
	if (pMarketEvent == nullptr || pBroadcast == nullptr ||
		pReliableEvent == nullptr)
	{
		p_refError =
			"EVENT_CONFIG_NODE_MISSING: MarketEvent, Broadcast and ReliableEvent are required";
		return false;
	}

	unsigned int uiQueueCapacity =
		static_cast<unsigned int>(
			p_refConfig.stMarketEvent.szQueueCapacity);
	if (!ReadOptionalUInt(pMarketEvent,
			"MtEventService.MarketEvent",
			"workerThreads", 1, 32,
			p_refConfig.stMarketEvent.uiWorkerThreads,
			p_refError) ||
		!ReadOptionalUInt(pMarketEvent,
			"MtEventService.MarketEvent",
			"queueCapacity", 0,
			10000000, uiQueueCapacity, p_refError) ||
		!ReadOptionalUInt(pMarketEvent,
			"MtEventService.MarketEvent",
			"dropLogIntervalMs", 1000,
			600000,
			p_refConfig.stMarketEvent.uiDropLogIntervalMs,
			p_refError))
	{
		return false;
	}
	p_refConfig.stMarketEvent.szQueueCapacity =
		uiQueueCapacity;
	if (p_refConfig.stMarketEvent.szQueueCapacity != 0 &&
		p_refConfig.stMarketEvent.szQueueCapacity < 128)
	{
		p_refError =
			"EVENT_CONFIG_QUEUE_INVALID: queueCapacity must be 0 or between 128 and 10000000";
		return false;
	}

	// 第二步：广播与 ReliableEvent 共用 NATS URL，但拥有独立 Subject、积压和去重边界。
	int iBroadcastEnable = 0;
	unsigned int uiBroadcastPayload = static_cast<unsigned int>(
		p_refConfig.stBroadcast.szMaxPayloadBytes);
	unsigned int uiDedupeCapacity = static_cast<unsigned int>(
		p_refConfig.stBroadcast.szDedupeCapacity);
	if (pBroadcast->QueryIntAttribute("enable", &iBroadcastEnable) !=
			TIXML_SUCCESS ||
		(iBroadcastEnable != 0 && iBroadcastEnable != 1) ||
		!ReadRequiredString(pBroadcast,
			"MtEventService.Broadcast", "subject",
			p_refConfig.stBroadcast.strSubject, p_refError) ||
		!ReadOptionalUInt(pBroadcast,
			"MtEventService.Broadcast", "maxPayloadBytes",
			1024, 52428800, uiBroadcastPayload, p_refError) ||
		!ReadOptionalUInt(pBroadcast,
			"MtEventService.Broadcast", "dedupeCapacity",
			1024, 5000000, uiDedupeCapacity, p_refError) ||
		!ReadOptionalUInt(pBroadcast,
			"MtEventService.Broadcast", "pendingMessages",
			1024, 5000000,
			p_refConfig.stBroadcast.uiPendingMessages, p_refError) ||
		!ReadOptionalUInt(pBroadcast,
			"MtEventService.Broadcast", "pendingBytes",
			1048576, 2147483647U,
			p_refConfig.stBroadcast.uiPendingBytes, p_refError))
	{
		p_refError = p_refError.empty() ?
			"EVENT_CONFIG_BROADCAST_INVALID: Broadcast attributes are invalid" :
			p_refError;
		return false;
	}
	p_refConfig.stBroadcast.bEnable = iBroadcastEnable == 1;
	p_refConfig.stBroadcast.szMaxPayloadBytes = uiBroadcastPayload;
	p_refConfig.stBroadcast.szDedupeCapacity = uiDedupeCapacity;
	if (p_refConfig.stBroadcast.strSubject.size() > 128)
	{
		p_refError =
			"EVENT_CONFIG_BROADCAST_SUBJECT_INVALID: subject exceeds 128 bytes";
		return false;
	}

	int iEnable = 0;
	if (pReliableEvent->QueryIntAttribute(
			"enable", &iEnable) != TIXML_SUCCESS ||
		(iEnable != 0 && iEnable != 1))
	{
		p_refError =
			"EVENT_CONFIG_BOOL_INVALID: node=MtEventService.ReliableEvent, attribute=enable";
		return false;
	}
	p_refConfig.stReliableEvent.bEnable =
		iEnable == 1;
	if (!p_refConfig.stReliableEvent.bEnable &&
		!p_refConfig.stBroadcast.bEnable)
	{
		return true;
	}
	unsigned int uiMaxDeliver =
		p_refConfig.stReliableEvent.uiMaxDeliver;
	if (!ReadUrl(pReliableEvent,
			p_refConfig.stReliableEvent.strUrl,
			p_refError) ||
		!ReadRequiredString(pReliableEvent,
			"MtEventService.ReliableEvent",
			"stream",
			p_refConfig.stReliableEvent.strStream,
			p_refError) ||
		!ReadRequiredString(pReliableEvent,
			"MtEventService.ReliableEvent",
			"subjectPrefix",
			p_refConfig.stReliableEvent.strSubjectPrefix,
			p_refError) ||
		!ReadOptionalInt64(pReliableEvent,
			"MtEventService.ReliableEvent",
			"maxAgeSeconds", 60, 315360000,
			p_refConfig.stReliableEvent.llMaxAgeSeconds,
			p_refError) ||
		!ReadOptionalInt64(pReliableEvent,
			"MtEventService.ReliableEvent",
			"maxBytes", 1048576,
			(std::numeric_limits<std::int64_t>::max)(),
			p_refConfig.stReliableEvent.llMaxBytes,
			p_refError) ||
		!ReadOptionalInt64(pReliableEvent,
			"MtEventService.ReliableEvent",
			"duplicateWindowSeconds", 60, 604800,
			p_refConfig.stReliableEvent.
				llDuplicateWindowSeconds,
			p_refError) ||
		!ReadOptionalUInt(pReliableEvent,
			"MtEventService.ReliableEvent",
			"replicas", 1, 5,
			p_refConfig.stReliableEvent.uiReplicas,
			p_refError) ||
		!ReadOptionalUInt(pReliableEvent,
			"MtEventService.ReliableEvent",
			"fetchBatchMax", 1, 10000,
			p_refConfig.stReliableEvent.uiFetchBatchMax,
			p_refError) ||
		!ReadOptionalUInt(pReliableEvent,
			"MtEventService.ReliableEvent",
			"fetchWaitMs", 1, 300000,
			p_refConfig.stReliableEvent.uiFetchWaitMs,
			p_refError) ||
		!ReadOptionalUInt(pReliableEvent,
			"MtEventService.ReliableEvent",
			"maxDeliver", 1, 1000,
			uiMaxDeliver, p_refError))
	{
		return false;
	}
	p_refConfig.stReliableEvent.uiMaxDeliver =
		uiMaxDeliver;
	if (p_refConfig.stReliableEvent.bEnable &&
		p_refConfig.stReliableEvent.uiReplicas != 1)
	{
		p_refError =
			"EVENT_CONFIG_REPLICAS_INVALID: first-phase ReliableEvent replicas must be 1";
		return false;
	}
	if (p_refConfig.stReliableEvent.strStream.size() >
			128 ||
		p_refConfig.stReliableEvent.strSubjectPrefix.size() >
			128)
	{
		p_refError =
			"EVENT_CONFIG_NAME_TOO_LONG: stream and subjectPrefix must not exceed 128 bytes";
		return false;
	}
	return true;
}
