#pragma once

#include <map>
#include <string>

// Single parser for push subscription text. The caller owns synchronization.
class CSubscriptionFilter
{
public:
	using SubscriptionMap = std::map<unsigned long, int>;

	// Parses numeric tokens strictly. Invalid text never degrades to id 0.
	static SubscriptionMap Parse(const std::string& p_refSubInfo);

	// Replaces the full set so repeat registration cannot retain stale ids.
	static void Replace(const std::string& p_refSubInfo,
		SubscriptionMap& p_refSubscriptions);

	// A specific include/exclude overrides the explicit id 0 subscribe-all token.
	static bool IsSubscribed(const SubscriptionMap& p_refSubscriptions,
		unsigned long p_ulRequestNo);

private:
	// Accepts a complete decimal DWORD only.
	static bool TryParseId(const std::string& p_refText,
		unsigned long& p_refId);
};
