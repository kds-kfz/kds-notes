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
	constexpr size_t s_szAesBlockSize = 16U;
	constexpr char s_szSecondsFeaturePrefix[] = "NSDK_SECONDS_V1|";

	// 返回历史授权协议固定密钥的只读进程级对象；对象由运行时管理，可并发读取。
	const std::string& GetAuthKey()
	{
		static const std::string s_strValue = "tjzt!@#$%^&*()_+<>?;";
		return s_strValue;
	}

	// 返回历史授权协议固定 IV 的只读进程级对象；对象由运行时管理，可并发读取。
	const std::string& GetAuthIv()
	{
		static const std::string s_strValue = "gfdertfghjkuyrtg";
		return s_strValue;
	}

	// 返回历史授权协议固定标识的只读进程级对象；对象由运行时管理，可并发读取。
	const std::string& GetAuthSignature()
	{
		static const std::string s_strValue = "TJZT";
		return s_strValue;
	}

	// 返回标准 Base64 字符表的只读进程级对象；对象由运行时管理，可并发读取。
	const std::string& GetBase64Chars()
	{
		static const std::string s_strValue =
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz"
			"0123456789+/";
		return s_strValue;
	}

	// 判断输入字节是否属于标准 Base64 字符表；无副作用且可并发调用。
	bool IsBase64(unsigned char p_chValue)
	{
		return std::isalnum(p_chValue) != 0 ||
			p_chValue == '+' || p_chValue == '/';
	}

	// 将时间戳转换为调用线程的本地时间副本；转换失败时返回零初始化结构体。
	std::tm GetSafeLocalTime(std::time_t p_i64Time)
	{
		std::tm stLocalTime = {};
#if defined(_WIN32) || defined(_WIN64)
		localtime_s(&stLocalTime, &p_i64Time);
#else
		localtime_r(&p_i64Time, &stLocalTime);
#endif
		return stLocalTime;
	}

	// 将本地时间转换为 YYYYMMDD 整数；仅执行字段换算，无共享状态。
	int ToDate(const std::tm& p_refTime)
	{
		return (p_refTime.tm_year + 1900) * 10000 +
			(p_refTime.tm_mon + 1) * 100 + p_refTime.tm_mday;
	}

	// 返回当前本地自然日 YYYYMMDD；系统时间异常时沿用标准库转换结果。
	int GetCurrentLocalDate()
	{
		return ToDate(GetSafeLocalTime(std::time(nullptr)));
	}

	// 在 YYYYMMDD 日期上增加指定天数；标准库无法归一化时返回 0，可并发调用。
	int AddDays(int p_iDate, int p_iDays)
	{
		std::tm stValue = {};
		stValue.tm_year = p_iDate / 10000 - 1900;
		stValue.tm_mon = (p_iDate / 100) % 100 - 1;
		stValue.tm_mday = p_iDate % 100 + p_iDays;
		stValue.tm_hour = 12;
		const std::time_t i64Normalized = std::mktime(&stValue);
		return i64Normalized == static_cast<std::time_t>(-1) ?
			0 : ToDate(GetSafeLocalTime(i64Normalized));
	}

	// 从至少 8 字节的日期文本读取 YYYYMMDD；空指针返回 0，调用方负责长度契约。
	int ParseDate8(const char* p_szDate)
	{
		if (p_szDate == nullptr)
		{
			return 0;
		}
		char aDate[9] = { 0 };
		std::memcpy(aDate, p_szDate, 8U);
		return std::atoi(aDate);
	}

	// 校验 UTC 秒格式授权明文；1 表示不是该格式，0 表示通过，其他负值保持历史错误语义。
	// 函数只读输入和进程级常量，可并发调用。
	int ValidateSecondsFeature(const std::string& p_refClientInfo,
		const std::string& p_refFeatureInfo)
	{
		const std::string strPrefix = s_szSecondsFeaturePrefix;
		// 返回 1 是兼容分流哨兵，表示“不是秒级格式”，调用方据此继续尝试历史自然日格式。
		if (p_refFeatureInfo.compare(0, strPrefix.size(), strPrefix) != 0)
		{
			return 1;
		}
		const std::string strBody = p_refFeatureInfo.substr(strPrefix.size());
		const size_t szSeparator1 = strBody.find('|');
		const size_t szSeparator2 = szSeparator1 == std::string::npos ?
			std::string::npos : strBody.find('|', szSeparator1 + 1U);
		const size_t szSeparator3 = szSeparator2 == std::string::npos ?
			std::string::npos : strBody.find('|', szSeparator2 + 1U);
		const size_t szSeparator4 = szSeparator3 == std::string::npos ?
			std::string::npos : strBody.find('|', szSeparator3 + 1U);
		// 秒级明文固定为五段且只能有四个分隔符，防止多余字段被不同调用方作不同解释。
		if (szSeparator1 == std::string::npos || szSeparator2 == std::string::npos ||
			szSeparator3 == std::string::npos || szSeparator4 == std::string::npos ||
			strBody.find('|', szSeparator4 + 1U) != std::string::npos)
		{
			return -3;
		}
		const std::string strClientInfoBase64 = strBody.substr(0, szSeparator1);
		const std::string strAuthKey = strBody.substr(szSeparator1 + 1U,
			szSeparator2 - szSeparator1 - 1U);
		const std::string strSignature = strBody.substr(szSeparator2 + 1U,
			szSeparator3 - szSeparator2 - 1U);
		const std::string strIssuedValue = strBody.substr(szSeparator3 + 1U,
			szSeparator4 - szSeparator3 - 1U);
		const std::string strExpiresValue = strBody.substr(szSeparator4 + 1U);
		// 固定密钥、协议标识和客户端身份必须同时匹配，任一不一致都不能进入时间判断。
		if (strAuthKey != GetAuthKey() || strSignature != GetAuthSignature() ||
			credcore::Base64Decode(strClientInfoBase64) != p_refClientInfo)
		{
			return -4;
		}
		char* pIssuedEnd = nullptr;
		char* pExpiresEnd = nullptr;
		const long long lIssuedAt = std::strtoll(
			strIssuedValue.c_str(), &pIssuedEnd, 10);
		const long long lExpiresAt = std::strtoll(
			strExpiresValue.c_str(), &pExpiresEnd, 10);
		// strtoll 必须消费完整十进制文本；同时拒绝负时间和非递增区间，避免接受部分数字前缀。
		if (pIssuedEnd == nullptr || *pIssuedEnd != '\0' ||
			pExpiresEnd == nullptr || *pExpiresEnd != '\0' ||
			lIssuedAt < 0 || lExpiresAt <= lIssuedAt)
		{
			return -3;
		}
		const std::int64_t i64Now = static_cast<std::int64_t>(
			std::chrono::duration_cast<std::chrono::seconds>(
				std::chrono::system_clock::now().time_since_epoch()).count());
		// 有效期采用半开区间 [issuedAt, expiresAt)，到期秒本身已失效，避免边界重复放行。
		return i64Now < lIssuedAt || i64Now >= lExpiresAt ? -7 : 0;
	}
}

// 编码指定长度的字节序列；空指针仅在长度为 0 时合法，失败返回空字符串，可并发调用。
std::string credcore::Base64Encode(
	const unsigned char* p_pBytes, unsigned int p_uiLength)
{
	if (p_pBytes == nullptr && p_uiLength != 0U)
	{
		return std::string();
	}
	std::string strResult;
	unsigned char aChars3[3] = { 0 };
	unsigned char aChars4[4] = { 0 };
	int iIndex = 0;
	while (p_uiLength-- > 0U)
	{
		aChars3[iIndex++] = *(p_pBytes++);
		if (iIndex != 3)
		{
			continue;
		}
		// 第 1 个编码值取第 1 字节的高 6 位。
		aChars4[0] = static_cast<unsigned char>((aChars3[0] & 0xfc) >> 2);
		// 第 2 个编码值拼接第 1 字节低 2 位与第 2 字节高 4 位。
		aChars4[1] = static_cast<unsigned char>(((aChars3[0] & 0x03) << 4) +
			((aChars3[1] & 0xf0) >> 4));
		// 第 3 个编码值拼接第 2 字节低 4 位与第 3 字节高 2 位。
		aChars4[2] = static_cast<unsigned char>(((aChars3[1] & 0x0f) << 2) +
			((aChars3[2] & 0xc0) >> 6));
		// 第 4 个编码值取第 3 字节的低 6 位，至此 24 位拆成四组 6 位。
		aChars4[3] = static_cast<unsigned char>(aChars3[2] & 0x3f);
		// 四个 0..63 索引逐个映射为标准 Base64 字符。
		for (iIndex = 0; iIndex < 4; ++iIndex)
		{
			strResult += GetBase64Chars()[aChars4[iIndex]];
		}
		iIndex = 0;
	}
	// 不足三个输入字节时补零完成位运算，再按标准 Base64 规则追加 '=' 到四字符边界。
	if (iIndex != 0)
	{
		for (int iFill = iIndex; iFill < 3; ++iFill)
		{
			aChars3[iFill] = 0;
		}
		// 尾组沿用完整三字节的位拆分公式，补零位稍后由 '=' 标记为无效数据。
		aChars4[0] = static_cast<unsigned char>((aChars3[0] & 0xfc) >> 2);
		aChars4[1] = static_cast<unsigned char>(((aChars3[0] & 0x03) << 4) +
			((aChars3[1] & 0xf0) >> 4));
		aChars4[2] = static_cast<unsigned char>(((aChars3[1] & 0x0f) << 2) +
			((aChars3[2] & 0xc0) >> 6));
		aChars4[3] = static_cast<unsigned char>(aChars3[2] & 0x3f);
		// N 个尾字节产生 N+1 个有效编码字符，其余位置使用 '=' 补齐到四字符边界。
		for (int iOutput = 0; iOutput < iIndex + 1; ++iOutput)
		{
			strResult += GetBase64Chars()[aChars4[iOutput]];
		}
		while (iIndex++ < 3)
		{
			strResult += '=';
		}
	}
	return strResult;
}

// 解码标准 Base64 文本；非法字符会终止既有兼容解析，返回已解码前缀，可并发调用。
std::string credcore::Base64Decode(const std::string& p_refEncoded)
{
	std::string strResult;
	unsigned char aChars3[3] = { 0 };
	unsigned char aChars4[4] = { 0 };
	int iIndex = 0;
	int iInputPosition = 0;
	int iRemaining = static_cast<int>(p_refEncoded.size());
	// 历史接口遇到 '=' 或非法字符即停止并返回已解码前缀；这是既有兼容语义，不能改成严格报错。
	while (iRemaining-- > 0 && p_refEncoded[iInputPosition] != '=' &&
		IsBase64(static_cast<unsigned char>(p_refEncoded[iInputPosition])))
	{
		aChars4[iIndex++] = static_cast<unsigned char>(p_refEncoded[iInputPosition++]);
		if (iIndex != 4)
		{
			continue;
		}
		// 把四个 Base64 字符转换回各自的 6 位数值。
		for (iIndex = 0; iIndex < 4; ++iIndex)
		{
			aChars4[iIndex] = static_cast<unsigned char>(
				GetBase64Chars().find(static_cast<char>(aChars4[iIndex])));
		}
		// 第 1 个原字节由第 1 组全部 6 位和第 2 组高 2 位组成。
		aChars3[0] = static_cast<unsigned char>((aChars4[0] << 2) +
			((aChars4[1] & 0x30) >> 4));
		// 第 2 个原字节由第 2 组低 4 位和第 3 组高 4 位组成。
		aChars3[1] = static_cast<unsigned char>(((aChars4[1] & 0x0f) << 4) +
			((aChars4[2] & 0x3c) >> 2));
		// 第 3 个原字节由第 3 组低 2 位和第 4 组全部 6 位组成。
		aChars3[2] = static_cast<unsigned char>(((aChars4[2] & 0x03) << 6) + aChars4[3]);
		// 完整四字符组必然还原三个字节。
		strResult.append(reinterpret_cast<const char*>(aChars3), 3U);
		iIndex = 0;
	}
	// 末组每 N 个有效编码字符只能还原 N-1 个字节，未填满的槽位仅用于完成位运算。
	if (iIndex != 0)
	{
		const int iEncodedCount = iIndex;
		for (int iFill = iIndex; iFill < 4; ++iFill)
		{
			aChars4[iFill] = 0;
		}
		// 尾组空槽补零后仍按四组 6 位转换，实际输出字节数由有效字符数决定。
		for (iIndex = 0; iIndex < 4; ++iIndex)
		{
			aChars4[iIndex] = static_cast<unsigned char>(
				GetBase64Chars().find(static_cast<char>(aChars4[iIndex])));
		}
		// 尾组沿用与完整组相同的 6 位到 8 位重组公式。
		aChars3[0] = static_cast<unsigned char>((aChars4[0] << 2) +
			((aChars4[1] & 0x30) >> 4));
		aChars3[1] = static_cast<unsigned char>(((aChars4[1] & 0x0f) << 4) +
			((aChars4[2] & 0x3c) >> 2));
		aChars3[2] = static_cast<unsigned char>(((aChars4[2] & 0x03) << 6) + aChars4[3]);
		// N 个有效编码字符只能产生 N-1 个完整字节，补零产生的尾字节不得写入结果。
		strResult.append(reinterpret_cast<const char*>(aChars3),
			static_cast<size_t>(iEncodedCount - 1));
	}
	return strResult;
}

// 使用历史固定密钥和 IV 执行 AES-CBC/PKCS#7 加密并返回 Base64；失败可能抛出 AES 内核异常。
// 函数只读进程级常量，每次调用使用独立上下文，可并发调用。
std::string credcore::Aes(const std::string& p_refSource)
{
	const size_t szInputLength = p_refSource.size();
	// PKCS#7 即使明文恰好对齐也必须增加完整填充分组，因此分组数固定加一。
	const size_t szBlockCount = szInputLength / s_szAesBlockSize + 1U;
	std::vector<char> aInput(szBlockCount * s_szAesBlockSize, 0);
	std::copy(p_refSource.begin(), p_refSource.end(), aInput.begin());
	const unsigned char chPadding = static_cast<unsigned char>(
		s_szAesBlockSize - szInputLength % s_szAesBlockSize);
	std::fill(aInput.begin() + static_cast<std::ptrdiff_t>(szInputLength),
		aInput.end(), static_cast<char>(chPadding));
	std::vector<char> aOutput(aInput.size(), 0);
	CAes clAes;
	clAes.MakeKey(GetAuthKey().c_str(), GetAuthIv().c_str(),
		static_cast<int>(s_szAesBlockSize), static_cast<int>(s_szAesBlockSize));
	clAes.Encrypt(aInput.data(), aOutput.data(),
		static_cast<unsigned long>(aInput.size()), CAes::CBC);
	return Base64Encode(reinterpret_cast<const unsigned char*>(aOutput.data()),
		static_cast<unsigned int>(aOutput.size()));
}

// 解码 Base64 并执行 AES-CBC 解密；密文长度或 PKCS#7 填充非法时返回空字符串。
// 函数每次调用使用独立上下文，可并发调用。
std::string credcore::Deaes(const std::string& p_refSource)
{
	const std::string strEncrypted = Base64Decode(p_refSource);
	// CBC 只能处理完整的 16 字节分组；空密文或非整分组说明输入已截断或不是本协议密文。
	if (strEncrypted.empty() || strEncrypted.size() % s_szAesBlockSize != 0U)
	{
		return std::string();
	}
	std::vector<char> aInput(strEncrypted.begin(), strEncrypted.end());
	std::vector<char> aOutput(strEncrypted.size(), 0);
	CAes clAes;
	clAes.MakeKey(GetAuthKey().c_str(), GetAuthIv().c_str(),
		static_cast<int>(s_szAesBlockSize), static_cast<int>(s_szAesBlockSize));
	clAes.Decrypt(aInput.data(), aOutput.data(),
		static_cast<unsigned long>(strEncrypted.size()), CAes::CBC);
	const unsigned char chPadding = static_cast<unsigned char>(aOutput.back());
	// PKCS#7 填充值必须在 1..分组长度内，且不能超过本次明文缓冲区。
	if (chPadding == 0U || chPadding > s_szAesBlockSize || chPadding > aOutput.size())
	{
		return std::string();
	}
	// 每个尾字节都必须等于填充长度，只检查最后一个字节会把损坏密文误当成有效明文。
	for (size_t szOffset = 0; szOffset < chPadding; ++szOffset)
	{
		if (static_cast<unsigned char>(aOutput[aOutput.size() - 1U - szOffset]) != chPadding)
		{
			return std::string();
		}
	}
	return std::string(aOutput.data(), aOutput.size() - chPadding);
}

// 根据明文客户信息生成历史加密客户信息；返回值供 BuildFeatrue 使用，可并发调用。
std::string credcore::MakeFeatrue(const std::string p_strClientInfo)
{
	std::string strClientInfo = p_strClientInfo;
	strClientInfo.append(GetAuthKey());
	strClientInfo.append(GetAuthSignature());
	strClientInfo.append(std::to_string(GetCurrentLocalDate()));
	return Aes(strClientInfo);
}

// 根据明文客户信息生成自然日授权码；成功返回 0 并覆盖输出，失败返回负值并清空输出。
// 函数不保存调用状态，可并发调用。
int credcore::BuildFeatrueByPlainClientInfo(
	const std::string& p_refClientInfo, int p_iAuthDay,
	std::string& p_refFeature)
{
	p_refFeature.clear();
	if (p_refClientInfo.empty())
	{
		return -1;
	}
	const int iCurrentDate = GetCurrentLocalDate();
	// 历史协议把授权天数限制在 [-30, 30]，保持原有令牌字节和调用方预期。
	const int iAuthDay = (std::max)(-30, (std::min)(30, p_iAuthDay));
	const int iNextDate = AddDays(iCurrentDate, iAuthDay);
	if (iNextDate == 0)
	{
		return -2;
	}
	std::string strPlain = p_refClientInfo;
	strPlain.append(GetAuthKey());
	strPlain.append(GetAuthSignature());
	strPlain.append(std::to_string(iCurrentDate));
	strPlain.append(std::to_string(iNextDate));
	p_refFeature = Aes(strPlain);
	return p_refFeature.empty() ? -2 : 0;
}

// 根据明文客户信息生成 UTC 秒授权码；成功返回 0，参数或时间溢出返回 -1，加密失败返回 -2。
// 函数不保存调用状态，可并发调用。
int credcore::BuildFeatrueByPlainClientInfoSeconds(
	const std::string& p_refClientInfo, std::string& p_refFeature,
	std::int64_t p_i64AuthSeconds)
{
	p_refFeature.clear();
	// Base64Encode 的长度 ABI 是 unsigned int，转换前必须限制 size_t，避免大输入被截断。
	if (p_refClientInfo.empty() || p_i64AuthSeconds <= 0 ||
		p_refClientInfo.size() > static_cast<size_t>((std::numeric_limits<unsigned int>::max)()))
	{
		return -1;
	}
	const std::int64_t i64IssuedAt = static_cast<std::int64_t>(
		std::chrono::duration_cast<std::chrono::seconds>(
			std::chrono::system_clock::now().time_since_epoch()).count());
	// 先验证加法上界再计算 expiresAt，避免秒数相加产生有符号整数溢出。
	if (i64IssuedAt < 0 || p_i64AuthSeconds >
		(std::numeric_limits<std::int64_t>::max)() - i64IssuedAt)
	{
		return -1;
	}
	const std::string strClientInfoBase64 = Base64Encode(
		reinterpret_cast<const unsigned char*>(p_refClientInfo.data()),
		static_cast<unsigned int>(p_refClientInfo.size()));
	std::string strPayload = s_szSecondsFeaturePrefix;
	strPayload.append(strClientInfoBase64);
	strPayload.push_back('|');
	strPayload.append(GetAuthKey());
	strPayload.push_back('|');
	strPayload.append(GetAuthSignature());
	strPayload.push_back('|');
	strPayload.append(std::to_string(i64IssuedAt));
	strPayload.push_back('|');
	strPayload.append(std::to_string(i64IssuedAt + p_i64AuthSeconds));
	p_refFeature = Aes(strPayload);
	return p_refFeature.empty() ? -2 : 0;
}

// 解密客户端信息并生成历史自然日授权码；返回码保持原 nsdk 语义，输出仅成功时可用。
// 函数不保存调用状态，可并发调用。
int credcore::BuildFeatrue(std::string p_strClientInfo,
	unsigned int p_uiClientInfoLength, int p_iAuthDay,
	std::string& p_refFeature)
{
	if (p_strClientInfo.empty())
	{
		return -1;
	}
	const std::string strPlain = Deaes(p_strClientInfo);
	if (strPlain.empty())
	{
		return -2;
	}
	const size_t szKeyLength = GetAuthKey().size();
	const size_t szSignatureLength = GetAuthSignature().size();
	constexpr size_t szDateLength = 8U;
	// 历史输入明文布局固定为 ClientInfo + Key + Signature + YYYYMMDD，长度不符不能按偏移读取。
	if (strPlain.size() != static_cast<size_t>(p_uiClientInfoLength) +
		szKeyLength + szSignatureLength + szDateLength)
	{
		return -3;
	}
	if (strPlain.compare(p_uiClientInfoLength, szKeyLength, GetAuthKey()) != 0)
	{
		return -4;
	}
	const size_t szSignatureOffset = p_uiClientInfoLength + szKeyLength;
	if (strPlain.compare(szSignatureOffset, szSignatureLength, GetAuthSignature()) != 0)
	{
		return -5;
	}
	const int iStartDate = ParseDate8(
		strPlain.c_str() + szSignatureOffset + szSignatureLength);
	const int iAuthDay = (std::max)(-30, (std::min)(30, p_iAuthDay));
	const int iEndDate = AddDays(iStartDate, iAuthDay);
	if (iEndDate == 0)
	{
		return -2;
	}
	p_refFeature = Aes(strPlain + std::to_string(iEndDate));
	return 0;
}

// 校验明文客户信息对应的自然日或 UTC 秒授权码；成功返回 0 并输出解密明文。
// 失败返回历史负值，函数不保存调用状态，可并发调用。
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
	const int iSecondsResult = ValidateSecondsFeature(
		p_refClientInfo, p_refFeatureInfo);
	// 仅哨兵 1 表示“格式不匹配”并允许回退；秒级格式一旦命中，其成功或失败结果都是终态。
	if (iSecondsResult != 1)
	{
		return iSecondsResult;
	}
	return AuthFeatrue(p_refClientInfo, p_refFeature, p_refFeatureInfo);
}

// 校验传统自然日授权码；成功返回 0 并输出解密明文，失败返回原 nsdk 负值。
// 函数不保存调用状态，可并发调用。
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
	const size_t szClientInfoLength = p_strClientInfo.size();
	const size_t szKeyLength = GetAuthKey().size();
	const size_t szSignatureLength = GetAuthSignature().size();
	constexpr size_t szDateLength = 16U;
	// 传统明文尾部固定包含两个 YYYYMMDD，精确长度检查用于保证后续指针偏移不会跨界。
	if (p_refFeatureInfo.size() != szClientInfoLength + szKeyLength +
		szSignatureLength + szDateLength)
	{
		return -3;
	}
	if (p_refFeatureInfo.compare(0, szClientInfoLength, p_strClientInfo) != 0)
	{
		return -4;
	}
	if (p_refFeatureInfo.compare(szClientInfoLength, szKeyLength, GetAuthKey()) != 0)
	{
		return -5;
	}
	const size_t szSignatureOffset = szClientInfoLength + szKeyLength;
	if (p_refFeatureInfo.compare(szSignatureOffset, szSignatureLength,
		GetAuthSignature()) != 0)
	{
		return -6;
	}
	const char* szDates =
		p_refFeatureInfo.c_str() + szSignatureOffset + szSignatureLength;
	const int iStartDate = ParseDate8(szDates);
	const int iEndDate = ParseDate8(szDates + 8);
	// 保持历史返回码：只检查是否晚于结束日，开始/结束关系在下一步单独分类。
	if (GetCurrentLocalDate() > iEndDate)
	{
		return -7;
	}
	if (iStartDate > iEndDate)
	{
		return -8;
	}
	// 历史协议最终要求“开始日经日期归一化后等于结束日”；这是既有 -9 语义，不能改成区间判断。
	return AddDays(iStartDate, 0) == iEndDate ? 0 : -9;
}
