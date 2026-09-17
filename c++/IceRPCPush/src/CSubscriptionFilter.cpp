#include "CSubscriptionFilter.h"

#include <cctype>
#include <limits>

CSubscriptionFilter::SubscriptionMap CSubscriptionFilter::Parse(
	const std::string& p_refSubInfo)
{
	SubscriptionMap mapResult;
	std::size_t szBegin = 0;
	while (szBegin <= p_refSubInfo.size())
	{
		const std::size_t szEnd = p_refSubInfo.find('|', szBegin);
		const std::size_t szLength = szEnd == std::string::npos ?
			p_refSubInfo.size() - szBegin : szEnd - szBegin;
		std::string strToken = p_refSubInfo.substr(szBegin, szLength);
		bool bSubscribe = true;
		if (!strToken.empty() && strToken.front() == '~')
		{
			bSubscribe = false;
			strToken.erase(strToken.begin());
		}

		unsigned long ulId = 0;
		if (TryParseId(strToken, ulId))
		{
			mapResult[ulId] = bSubscribe ? 1 : 0;
		}

		if (szEnd == std::string::npos)
		{
			break;
		}
		szBegin = szEnd + 1;
	}
	return mapResult;
}

void CSubscriptionFilter::Replace(const std::string& p_refSubInfo,
	SubscriptionMap& p_refSubscriptions)
{
	p_refSubscriptions = Parse(p_refSubInfo);
}

bool CSubscriptionFilter::IsSubscribed(
	const SubscriptionMap& p_refSubscriptions,
	unsigned long p_ulRequestNo)
{
	const SubscriptionMap::const_iterator itRequest =
		p_refSubscriptions.find(p_ulRequestNo);
	if (itRequest != p_refSubscriptions.end())
	{
		return itRequest->second > 0;
	}
	const SubscriptionMap::const_iterator itAll =
		p_refSubscriptions.find(0);
	return itAll != p_refSubscriptions.end() && itAll->second > 0;
}

bool CSubscriptionFilter::TryParseId(const std::string& p_refText,
	unsigned long& p_refId)
{
	if (p_refText.empty())
	{
		return false;
	}
	unsigned long ulValue = 0;
	for (std::string::const_iterator it = p_refText.begin();
		it != p_refText.end(); ++it)
	{
		const unsigned char ucValue =
			static_cast<unsigned char>(*it);
		if (!std::isdigit(ucValue))
		{
			return false;
		}
		const unsigned long ulDigit =
			static_cast<unsigned long>(ucValue - '0');
		if (ulValue > (std::numeric_limits<unsigned long>::max)() / 10UL ||
			(ulValue == (std::numeric_limits<unsigned long>::max)() / 10UL &&
			ulDigit > (std::numeric_limits<unsigned long>::max)() % 10UL))
		{
			return false;
		}
		ulValue = ulValue * 10UL + ulDigit;
	}
	p_refId = ulValue;
	return true;
}
