#include "CredCore.h"

#include <iostream>
#include <string>

namespace
{
	bool Expect(bool p_bCondition, const char* p_szName)
	{
		if (p_bCondition)
		{
			return true;
		}
		std::cerr << p_szName << " failed" << std::endl;
		return false;
	}
}

int main()
{
	bool passed = true;
	const std::string clientInfo = "4087";
	const std::string plain = "CredCore-compatible-payload";
	const std::string base64 = credcore::Base64Encode(
		reinterpret_cast<const unsigned char*>(plain.data()),
		static_cast<unsigned int>(plain.size()));
	passed = Expect(base64 == "Q3JlZENvcmUtY29tcGF0aWJsZS1wYXlsb2Fk",
		"Base64Encode known vector") && passed;
	passed = Expect(credcore::Base64Decode(base64) == plain,
		"Base64Decode round trip") && passed;
	passed = Expect(credcore::Deaes(credcore::Aes(plain)) == plain,
		"AES round trip") && passed;

	std::string dailyFeature;
	std::string featureInfo;
	passed = Expect(credcore::BuildFeatrueByPlainClientInfo(
		clientInfo, 0, dailyFeature) == 0, "daily feature generation") && passed;
	passed = Expect(credcore::AuthFeatrueByPlainClientInfo(
		clientInfo, dailyFeature, featureInfo) == 0,
		"daily feature validation") && passed;
	passed = Expect(credcore::AuthFeatrueByPlainClientInfo(
		"4088", dailyFeature, featureInfo) == -4,
		"daily client mismatch") && passed;

	std::string secondsFeature;
	passed = Expect(credcore::BuildFeatrueByPlainClientInfoSeconds(
		clientInfo, secondsFeature, 120) == 0,
		"seconds feature generation") && passed;
	passed = Expect(credcore::AuthFeatrueByPlainClientInfo(
		clientInfo, secondsFeature, featureInfo) == 0,
		"seconds feature validation") && passed;

	const std::string encryptedClient = credcore::MakeFeatrue(clientInfo);
	std::string traditionalFeature;
	passed = Expect(credcore::BuildFeatrue(encryptedClient,
		static_cast<unsigned int>(clientInfo.size()), 0,
		traditionalFeature) == 0, "traditional feature generation") && passed;
	passed = Expect(credcore::AuthFeatrue(
		clientInfo, traditionalFeature, featureInfo) == 0,
		"traditional feature validation") && passed;

	if (!passed)
	{
		return 1;
	}
	std::cout << "CREDCORE_TESTS_PASSED" << std::endl;
	return 0;
}
