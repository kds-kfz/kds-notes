#include "CredCore.h"
#include "aes.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <limits>
#include <string>
#include <vector>

namespace
{
	constexpr size_t kAesBlockSize = 16U;
	constexpr char kSecondsFeaturePrefix[] = "NSDK_SECONDS_V1|";

	const std::string& GetAuthKey()
	{
		static const std::string value = "tjzt!@#$%^&*()_+<>?;";
		return value;
	}

	const std::string& GetAuthIv()
	{
		static const std::string value = "gfdertfghjkuyrtg";
		return value;
	}

	const std::string& GetAuthSignature()
	{
		static const std::string value = "TJZT";
		return value;
	}

	const std::string& GetBase64Chars()
	{
		static const std::string value =
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz"
			"0123456789+/";
		return value;
	}

	bool IsBase64(unsigned char p_chValue)
	{
		return std::isalnum(p_chValue) != 0 ||
			p_chValue == '+' || p_chValue == '/';
	}

	std::tm GetSafeLocalTime(std::time_t p_time)
	{
		std::tm local = {};
#if defined(_WIN32) || defined(_WIN64)
		localtime_s(&local, &p_time);
#else
		localtime_r(&p_time, &local);
#endif
		return local;
	}

	int ToDate(const std::tm& p_refTime)
	{
		return (p_refTime.tm_year + 1900) * 10000 +
			(p_refTime.tm_mon + 1) * 100 + p_refTime.tm_mday;
	}

	int GetCurrentLocalDate()
	{
		return ToDate(GetSafeLocalTime(std::time(nullptr)));
	}

	int AddDays(int p_iDate, int p_iDays)
	{
		std::tm value = {};
		value.tm_year = p_iDate / 10000 - 1900;
		value.tm_mon = (p_iDate / 100) % 100 - 1;
		value.tm_mday = p_iDate % 100 + p_iDays;
		value.tm_hour = 12;
		const std::time_t normalized = std::mktime(&value);
		return normalized == static_cast<std::time_t>(-1) ?
			0 : ToDate(GetSafeLocalTime(normalized));
	}

	int ParseDate8(const char* p_szDate)
	{
		if (p_szDate == nullptr)
		{
			return 0;
		}
		char date[9] = { 0 };
		std::memcpy(date, p_szDate, 8U);
		return std::atoi(date);
	}

	int ValidateSecondsFeature(const std::string& p_refClientInfo,
		const std::string& p_refFeatureInfo)
	{
		const std::string prefix = kSecondsFeaturePrefix;
		if (p_refFeatureInfo.compare(0, prefix.size(), prefix) != 0)
		{
			return 1;
		}
		const std::string body = p_refFeatureInfo.substr(prefix.size());
		const size_t separator1 = body.find('|');
		const size_t separator2 = separator1 == std::string::npos ?
			std::string::npos : body.find('|', separator1 + 1U);
		const size_t separator3 = separator2 == std::string::npos ?
			std::string::npos : body.find('|', separator2 + 1U);
		const size_t separator4 = separator3 == std::string::npos ?
			std::string::npos : body.find('|', separator3 + 1U);
		if (separator1 == std::string::npos || separator2 == std::string::npos ||
			separator3 == std::string::npos || separator4 == std::string::npos ||
			body.find('|', separator4 + 1U) != std::string::npos)
		{
			return -3;
		}
		const std::string clientInfoBase64 = body.substr(0, separator1);
		const std::string authKey = body.substr(separator1 + 1U,
			separator2 - separator1 - 1U);
		const std::string signature = body.substr(separator2 + 1U,
			separator3 - separator2 - 1U);
		const std::string issuedValue = body.substr(separator3 + 1U,
			separator4 - separator3 - 1U);
		const std::string expiresValue = body.substr(separator4 + 1U);
		if (authKey != GetAuthKey() || signature != GetAuthSignature() ||
			credcore::Base64Decode(clientInfoBase64) != p_refClientInfo)
		{
			return -4;
		}
		char* issuedEnd = nullptr;
		char* expiresEnd = nullptr;
		const long long issuedAt = std::strtoll(
			issuedValue.c_str(), &issuedEnd, 10);
		const long long expiresAt = std::strtoll(
			expiresValue.c_str(), &expiresEnd, 10);
		if (issuedEnd == nullptr || *issuedEnd != '\0' ||
			expiresEnd == nullptr || *expiresEnd != '\0' ||
			issuedAt < 0 || expiresAt <= issuedAt)
		{
			return -3;
		}
		const std::int64_t now = static_cast<std::int64_t>(
			std::chrono::duration_cast<std::chrono::seconds>(
				std::chrono::system_clock::now().time_since_epoch()).count());
		return now < issuedAt || now >= expiresAt ? -7 : 0;
	}
}

std::string credcore::Base64Encode(
	const unsigned char* p_pBytes, unsigned int p_uiLength)
{
	if (p_pBytes == nullptr && p_uiLength != 0U)
	{
		return std::string();
	}
	std::string result;
	unsigned char chars3[3] = { 0 };
	unsigned char chars4[4] = { 0 };
	int index = 0;
	while (p_uiLength-- > 0U)
	{
		chars3[index++] = *(p_pBytes++);
		if (index != 3)
		{
			continue;
		}
		chars4[0] = static_cast<unsigned char>((chars3[0] & 0xfc) >> 2);
		chars4[1] = static_cast<unsigned char>(((chars3[0] & 0x03) << 4) +
			((chars3[1] & 0xf0) >> 4));
		chars4[2] = static_cast<unsigned char>(((chars3[1] & 0x0f) << 2) +
			((chars3[2] & 0xc0) >> 6));
		chars4[3] = static_cast<unsigned char>(chars3[2] & 0x3f);
		for (index = 0; index < 4; ++index)
		{
			result += GetBase64Chars()[chars4[index]];
		}
		index = 0;
	}
	if (index != 0)
	{
		for (int fill = index; fill < 3; ++fill)
		{
			chars3[fill] = 0;
		}
		chars4[0] = static_cast<unsigned char>((chars3[0] & 0xfc) >> 2);
		chars4[1] = static_cast<unsigned char>(((chars3[0] & 0x03) << 4) +
			((chars3[1] & 0xf0) >> 4));
		chars4[2] = static_cast<unsigned char>(((chars3[1] & 0x0f) << 2) +
			((chars3[2] & 0xc0) >> 6));
		chars4[3] = static_cast<unsigned char>(chars3[2] & 0x3f);
		for (int output = 0; output < index + 1; ++output)
		{
			result += GetBase64Chars()[chars4[output]];
		}
		while (index++ < 3)
		{
			result += '=';
		}
	}
	return result;
}

std::string credcore::Base64Decode(const std::string& p_refEncoded)
{
	std::string result;
	unsigned char chars3[3] = { 0 };
	unsigned char chars4[4] = { 0 };
	int index = 0;
	int inputPosition = 0;
	int remaining = static_cast<int>(p_refEncoded.size());
	while (remaining-- > 0 && p_refEncoded[inputPosition] != '=' &&
		IsBase64(static_cast<unsigned char>(p_refEncoded[inputPosition])))
	{
		chars4[index++] = static_cast<unsigned char>(p_refEncoded[inputPosition++]);
		if (index != 4)
		{
			continue;
		}
		for (index = 0; index < 4; ++index)
		{
			chars4[index] = static_cast<unsigned char>(
				GetBase64Chars().find(static_cast<char>(chars4[index])));
		}
		chars3[0] = static_cast<unsigned char>((chars4[0] << 2) +
			((chars4[1] & 0x30) >> 4));
		chars3[1] = static_cast<unsigned char>(((chars4[1] & 0x0f) << 4) +
			((chars4[2] & 0x3c) >> 2));
		chars3[2] = static_cast<unsigned char>(((chars4[2] & 0x03) << 6) + chars4[3]);
		result.append(reinterpret_cast<const char*>(chars3), 3U);
		index = 0;
	}
	if (index != 0)
	{
		const int encodedCount = index;
		for (int fill = index; fill < 4; ++fill)
		{
			chars4[fill] = 0;
		}
		for (index = 0; index < 4; ++index)
		{
			chars4[index] = static_cast<unsigned char>(
				GetBase64Chars().find(static_cast<char>(chars4[index])));
		}
		chars3[0] = static_cast<unsigned char>((chars4[0] << 2) +
			((chars4[1] & 0x30) >> 4));
		chars3[1] = static_cast<unsigned char>(((chars4[1] & 0x0f) << 4) +
			((chars4[2] & 0x3c) >> 2));
		chars3[2] = static_cast<unsigned char>(((chars4[2] & 0x03) << 6) + chars4[3]);
		result.append(reinterpret_cast<const char*>(chars3),
			static_cast<size_t>(encodedCount - 1));
	}
	return result;
}

std::string credcore::Aes(const std::string& p_refSource)
{
	const size_t inputLength = p_refSource.size();
	const size_t blockCount = inputLength / kAesBlockSize + 1U;
	std::vector<char> input(blockCount * kAesBlockSize, 0);
	std::copy(p_refSource.begin(), p_refSource.end(), input.begin());
	const unsigned char padding = static_cast<unsigned char>(
		kAesBlockSize - inputLength % kAesBlockSize);
	std::fill(input.begin() + static_cast<std::ptrdiff_t>(inputLength),
		input.end(), static_cast<char>(padding));
	std::vector<char> output(input.size(), 0);
	AES aes;
	aes.MakeKey(GetAuthKey().c_str(), GetAuthIv().c_str(),
		static_cast<int>(kAesBlockSize), static_cast<int>(kAesBlockSize));
	aes.Encrypt(input.data(), output.data(),
		static_cast<unsigned long>(input.size()), AES::CBC);
	return Base64Encode(reinterpret_cast<const unsigned char*>(output.data()),
		static_cast<unsigned int>(output.size()));
}

std::string credcore::Deaes(const std::string& p_refSource)
{
	const std::string encrypted = Base64Decode(p_refSource);
	if (encrypted.empty() || encrypted.size() % kAesBlockSize != 0U)
	{
		return std::string();
	}
	std::vector<char> input(encrypted.begin(), encrypted.end());
	std::vector<char> output(encrypted.size(), 0);
	AES aes;
	aes.MakeKey(GetAuthKey().c_str(), GetAuthIv().c_str(),
		static_cast<int>(kAesBlockSize), static_cast<int>(kAesBlockSize));
	aes.Decrypt(input.data(), output.data(),
		static_cast<unsigned long>(encrypted.size()), AES::CBC);
	const unsigned char padding = static_cast<unsigned char>(output.back());
	if (padding == 0U || padding > kAesBlockSize || padding > output.size())
	{
		return std::string();
	}
	for (size_t offset = 0; offset < padding; ++offset)
	{
		if (static_cast<unsigned char>(output[output.size() - 1U - offset]) != padding)
		{
			return std::string();
		}
	}
	return std::string(output.data(), output.size() - padding);
}

std::string credcore::MakeFeatrue(const std::string p_strClientInfo)
{
	std::string clientInfo = p_strClientInfo;
	clientInfo.append(GetAuthKey());
	clientInfo.append(GetAuthSignature());
	clientInfo.append(std::to_string(GetCurrentLocalDate()));
	return Aes(clientInfo);
}

int credcore::BuildFeatrueByPlainClientInfo(
	const std::string& p_refClientInfo, int p_iAuthDay,
	std::string& p_refFeature)
{
	p_refFeature.clear();
	if (p_refClientInfo.empty())
	{
		return -1;
	}
	const int currentDate = GetCurrentLocalDate();
	const int authDay = (std::max)(-30, (std::min)(30, p_iAuthDay));
	const int nextDate = AddDays(currentDate, authDay);
	if (nextDate == 0)
	{
		return -2;
	}
	std::string plain = p_refClientInfo;
	plain.append(GetAuthKey());
	plain.append(GetAuthSignature());
	plain.append(std::to_string(currentDate));
	plain.append(std::to_string(nextDate));
	p_refFeature = Aes(plain);
	return p_refFeature.empty() ? -2 : 0;
}

int credcore::BuildFeatrueByPlainClientInfoSeconds(
	const std::string& p_refClientInfo, std::string& p_refFeature,
	std::int64_t p_i64AuthSeconds)
{
	p_refFeature.clear();
	if (p_refClientInfo.empty() || p_i64AuthSeconds <= 0 ||
		p_refClientInfo.size() > static_cast<size_t>((std::numeric_limits<unsigned int>::max)()))
	{
		return -1;
	}
	const std::int64_t issuedAt = static_cast<std::int64_t>(
		std::chrono::duration_cast<std::chrono::seconds>(
			std::chrono::system_clock::now().time_since_epoch()).count());
	if (issuedAt < 0 || p_i64AuthSeconds >
		(std::numeric_limits<std::int64_t>::max)() - issuedAt)
	{
		return -1;
	}
	const std::string clientInfoBase64 = Base64Encode(
		reinterpret_cast<const unsigned char*>(p_refClientInfo.data()),
		static_cast<unsigned int>(p_refClientInfo.size()));
	std::string payload = kSecondsFeaturePrefix;
	payload.append(clientInfoBase64);
	payload.push_back('|');
	payload.append(GetAuthKey());
	payload.push_back('|');
	payload.append(GetAuthSignature());
	payload.push_back('|');
	payload.append(std::to_string(issuedAt));
	payload.push_back('|');
	payload.append(std::to_string(issuedAt + p_i64AuthSeconds));
	p_refFeature = Aes(payload);
	return p_refFeature.empty() ? -2 : 0;
}

int credcore::BuildFeatrue(std::string p_strClientInfo,
	unsigned int p_uiClientInfoLength, int p_iAuthDay,
	std::string& p_refFeature)
{
	if (p_strClientInfo.empty())
	{
		return -1;
	}
	const std::string plain = Deaes(p_strClientInfo);
	if (plain.empty())
	{
		return -2;
	}
	const size_t keyLength = GetAuthKey().size();
	const size_t signatureLength = GetAuthSignature().size();
	constexpr size_t dateLength = 8U;
	if (plain.size() != static_cast<size_t>(p_uiClientInfoLength) +
		keyLength + signatureLength + dateLength)
	{
		return -3;
	}
	if (plain.compare(p_uiClientInfoLength, keyLength, GetAuthKey()) != 0)
	{
		return -4;
	}
	const size_t signatureOffset = p_uiClientInfoLength + keyLength;
	if (plain.compare(signatureOffset, signatureLength, GetAuthSignature()) != 0)
	{
		return -5;
	}
	const int startDate = ParseDate8(plain.c_str() + signatureOffset + signatureLength);
	const int authDay = (std::max)(-30, (std::min)(30, p_iAuthDay));
	const int endDate = AddDays(startDate, authDay);
	if (endDate == 0)
	{
		return -2;
	}
	p_refFeature = Aes(plain + std::to_string(endDate));
	return 0;
}

int credcore::AuthFeatrueByPlainClientInfo(
	const std::string& p_refClientInfo,
	const std::string& p_refFeature,
	std::string& p_refFeatureInfo)
{
	if (p_refFeature.empty())
	{
		return -1;
	}
	p_refFeatureInfo = Deaes(p_refFeature);
	if (p_refFeatureInfo.empty())
	{
		return -2;
	}
	const int secondsResult = ValidateSecondsFeature(
		p_refClientInfo, p_refFeatureInfo);
	if (secondsResult != 1)
	{
		return secondsResult;
	}
	return AuthFeatrue(p_refClientInfo, p_refFeature, p_refFeatureInfo);
}

int credcore::AuthFeatrue(const std::string p_strClientInfo,
	std::string p_strFeature, std::string& p_refFeatureInfo)
{
	if (p_strFeature.empty())
	{
		return -1;
	}
	p_refFeatureInfo = Deaes(p_strFeature);
	if (p_refFeatureInfo.empty())
	{
		return -2;
	}
	const size_t clientInfoLength = p_strClientInfo.size();
	const size_t keyLength = GetAuthKey().size();
	const size_t signatureLength = GetAuthSignature().size();
	constexpr size_t dateLength = 16U;
	if (p_refFeatureInfo.size() != clientInfoLength + keyLength +
		signatureLength + dateLength)
	{
		return -3;
	}
	if (p_refFeatureInfo.compare(0, clientInfoLength, p_strClientInfo) != 0)
	{
		return -4;
	}
	if (p_refFeatureInfo.compare(clientInfoLength, keyLength, GetAuthKey()) != 0)
	{
		return -5;
	}
	const size_t signatureOffset = clientInfoLength + keyLength;
	if (p_refFeatureInfo.compare(signatureOffset, signatureLength,
		GetAuthSignature()) != 0)
	{
		return -6;
	}
	const char* dates = p_refFeatureInfo.c_str() + signatureOffset + signatureLength;
	const int startDate = ParseDate8(dates);
	const int endDate = ParseDate8(dates + 8);
	if (GetCurrentLocalDate() > endDate)
	{
		return -7;
	}
	if (startDate > endDate)
	{
		return -8;
	}
	return AddDays(startDate, 0) == endDate ? 0 : -9;
}
