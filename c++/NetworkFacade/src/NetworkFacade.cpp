#include "NetworkFacade.h"

#include "FoundationFacade.h"
#include "SocketClient.h"
#include "SocketServer.h"

#include <algorithm>
#include <cstring>

namespace
{
	void CopyText(char* p_pszDestination, size_t p_uiCapacity, const char* p_pszSource)
	{
		if (p_pszDestination == nullptr || p_uiCapacity == 0U || p_pszSource == nullptr)
		{
			return;
		}
		const size_t uiLength = (std::min)(std::strlen(p_pszSource), p_uiCapacity - 1U);
		std::memcpy(p_pszDestination, p_pszSource, uiLength);
		p_pszDestination[uiLength] = '\0';
	}

	bool IsValidCapabilitiesHeader(const ST_NETWORK_FACADE_CAPABILITIES* p_pCapabilities)
	{
		if (p_pCapabilities == nullptr ||
			p_pCapabilities->uiStructSize < sizeof(ST_NETWORK_FACADE_CAPABILITIES) ||
			p_pCapabilities->uiAbiVersion != FACADE_ABI_VERSION ||
			p_pCapabilities->uiFlags != 0U)
		{
			return false;
		}
		for (const uint64_t ullReserved : p_pCapabilities->aullReserved)
		{
			if (ullReserved != 0U)
			{
				return false;
			}
		}
		return true;
	}
}

int NetworkFacade_GetAbiInfo(ST_FACADE_ABI_INFO* p_pInfo)
{
	if (p_pInfo == nullptr || p_pInfo->uiStructSize < sizeof(ST_FACADE_ABI_INFO) ||
		p_pInfo->uiAbiVersion != FACADE_ABI_VERSION)
	{
		return FACADE_ABI_MISMATCH;
	}

	// 查询内部 Foundation ABI 会同时把该无状态静态组件纳入最终所有者，禁止形成单独运行 DLL。
	ST_FACADE_ABI_INFO stFoundationInfo = {};
	stFoundationInfo.uiStructSize = sizeof(stFoundationInfo);
	stFoundationInfo.uiAbiVersion = FACADE_ABI_VERSION;
	if (FoundationFacade_GetAbiInfo(&stFoundationInfo) != FACADE_SUCCESS)
	{
		return FACADE_INTERNAL_ERROR;
	}

	std::memset(p_pInfo, 0, sizeof(*p_pInfo));
	p_pInfo->uiStructSize = sizeof(*p_pInfo);
	p_pInfo->uiAbiVersion = FACADE_ABI_VERSION;
	CopyText(p_pInfo->szFacadeName, sizeof(p_pInfo->szFacadeName), "NetworkFacade");
	CopyText(p_pInfo->szImplementationVersion,
		sizeof(p_pInfo->szImplementationVersion), "1.0.1");
	CopyText(p_pInfo->szDependencyVersion,
		sizeof(p_pInfo->szDependencyVersion), "SocketServer-ABI7;SocketClient-ABI4;HP-Socket-6.0.9");
	CopyText(p_pInfo->szBuildId, sizeof(p_pInfo->szBuildId), __DATE__ " " __TIME__);
	return FACADE_SUCCESS;
}

int NetworkFacade_GetCapabilities(ST_NETWORK_FACADE_CAPABILITIES* p_pCapabilities)
{
	if (!IsValidCapabilitiesHeader(p_pCapabilities))
	{
		return FACADE_ABI_MISMATCH;
	}

	std::memset(p_pCapabilities, 0, sizeof(*p_pCapabilities));
	p_pCapabilities->uiStructSize = sizeof(*p_pCapabilities);
	p_pCapabilities->uiAbiVersion = FACADE_ABI_VERSION;
	p_pCapabilities->uiFlags = NETWORK_FACADE_CAPABILITY_TCP_SERVER |
		NETWORK_FACADE_CAPABILITY_HTTP_SERVER |
		NETWORK_FACADE_CAPABILITY_WEBSOCKET_SERVER |
		NETWORK_FACADE_CAPABILITY_TCP_CLIENT |
		NETWORK_FACADE_CAPABILITY_HTTP_CLIENT |
		NETWORK_FACADE_CAPABILITY_WEBSOCKET_CLIENT |
		NETWORK_FACADE_CAPABILITY_TLS_CLIENT;
	p_pCapabilities->uiSocketServerAbiVersion = GetSocketServerAbiVersion();
	p_pCapabilities->uiSocketClientAbiVersion = GetSocketClientAbiVersion();
	return FACADE_SUCCESS;
}
