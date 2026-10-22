#include "CredCore.h"

#include <atomic>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace
{
	// 记录单项断言结果；失败时输出英文用例名，函数不保存共享状态。
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

// 依次验证历史编码/授权兼容性和 RS256 安全边界；全部通过返回 0，否则返回 1。
// 测试包含并发只读验签，不连接网络或业务数据库。
int main()
{
	bool bPassed = true;
	const std::string strClientInfo = "4087";
	const std::string strPlain = "CredCore-compatible-payload";
	const std::string strBase64 = credcore::Base64Encode(
		reinterpret_cast<const unsigned char*>(strPlain.data()),
		static_cast<unsigned int>(strPlain.size()));
	bPassed = Expect(strBase64 == "Q3JlZENvcmUtY29tcGF0aWJsZS1wYXlsb2Fk",
		"Base64Encode known vector") && bPassed;
	bPassed = Expect(credcore::Base64Decode(strBase64) == strPlain,
		"Base64Decode round trip") && bPassed;
	bPassed = Expect(credcore::Aes(strPlain) ==
		"qfOkyTcTc6ybAhgcvXx44yMDvMGq9lBErDNRIrle6XU=",
		"AES fixed compatibility vector") && bPassed;
	bPassed = Expect(credcore::Deaes(credcore::Aes(strPlain)) == strPlain,
		"AES round trip") && bPassed;

	std::string strDailyFeature;
	std::string strFeatureInfo;
	bPassed = Expect(credcore::BuildFeatrueByPlainClientInfo(
		strClientInfo, 0, strDailyFeature) == 0, "daily feature generation") && bPassed;
	bPassed = Expect(credcore::AuthFeatrueByPlainClientInfo(
		strClientInfo, strDailyFeature, strFeatureInfo) == 0,
		"daily feature validation") && bPassed;
	bPassed = Expect(credcore::AuthFeatrueByPlainClientInfo(
		"4088", strDailyFeature, strFeatureInfo) == -4,
		"daily client mismatch") && bPassed;

	std::string strSecondsFeature;
	bPassed = Expect(credcore::BuildFeatrueByPlainClientInfoSeconds(
		strClientInfo, strSecondsFeature, 120) == 0,
		"seconds feature generation") && bPassed;
	bPassed = Expect(credcore::AuthFeatrueByPlainClientInfo(
		strClientInfo, strSecondsFeature, strFeatureInfo) == 0,
		"seconds feature validation") && bPassed;

	const std::string strEncryptedClient = credcore::MakeFeatrue(strClientInfo);
	std::string strTraditionalFeature;
	bPassed = Expect(credcore::BuildFeatrue(strEncryptedClient,
		static_cast<unsigned int>(strClientInfo.size()), 0,
		strTraditionalFeature) == 0, "traditional feature generation") && bPassed;
	bPassed = Expect(credcore::AuthFeatrue(
		strClientInfo, strTraditionalFeature, strFeatureInfo) == 0,
		"traditional feature validation") && bPassed;

	const std::string strRsaKeyId = "credcore-test-key";
	const std::string strRsaModulus =
		"xldipdso2O8N30_gJu3IEzqUK0V7AojKFr4wY78uZeqyjQKwPtqW9WDDgoXUoPgi"
		"E7nlnPBwklGur7odCI4dL8EV6YvUQot2Y_cS5HoKVYoCij3SlIYXhT4fEncp-mC9X"
		"or3NMwIQB6-ZoWe1bL_CwkhW69k9a-hqOgGtH2mjs3imyDi9y7zxUU_SGsBAu8lMu"
		"npFXXap-4tkn-ni9hGYiwqBoqqPSk33Z6ICihTHyoSijFn71DS8R8FUt3DlBkc4Oh"
		"g6KEgnHC2JTKARLzqdgDVAYzvDOvbaUcL6Djl8LQ1SnUeHapPjlL7JQ3SqjcBDhNT"
		"o8Z7RBbnAT0bDvOM4Q";
	const std::string strRsaExponent = "AQAB";
	const std::string strValidJwt =
		"eyJhbGciOiJSUzI1NiIsImtpZCI6ImNyZWRjb3JlLXRlc3Qta2V5IiwidHlwIjoiSldUIn0."
		"eyJzdWIiOiJjcmVkY29yZS10ZXN0IiwibG9naW4iOjE5NzQyMX0."
		"CGXhDiE4gvRbZuBEikEbj3iCxQPZr61FtXBkW_1Euk0tpdbmiGq8GrX-q_w4uyGgBCYBQfOjgP3"
		"EvoKLYpHRGjuym6v0qedzNBrvKd3h1HhB7WoqmrV8QvkrTuVdtzxeX-urgyY64nwojR6v8bANK"
		"XiuO4vLdIzGPGIS55VEQ4929U8ku0PIKsAoM64DQfHTWfYEzN1VD52Llw2SMLvnA1gSDFzkSEB"
		"_3PSgMKFvgrPlMzI62di_3JwkOkm_W-PAVPitlIC-BGUG127abuaoi4DpwxBywl5EhFmsgv4fNp"
		"-E824zxM6HvXMTxJf12sQ6KeKsHXR3QxFP5dAY7iKpLw";
	const std::string strExpectedPayload = "{\"sub\":\"credcore-test\",\"login\":197421}";

	std::string strFingerprint;
	bPassed = Expect(credcore::GetCompactJwtSha256Fingerprint(
		strValidJwt, strFingerprint) == credcore::CREDCORE_RS256_SUCCESS &&
		strFingerprint == "a44d056a84dfa35c88624cbf1b24a6c796ab29a2d0dbb1413531faccfbbdebaa",
		"JWT SHA-256 strFingerprint") && bPassed;

	std::string strParsedKeyId;
	bPassed = Expect(credcore::ReadRs256JwtKeyId(strValidJwt, strParsedKeyId) ==
		credcore::CREDCORE_RS256_SUCCESS && strParsedKeyId == strRsaKeyId,
		"RS256 header key id") && bPassed;

	credcore::CREDCORE_RS256_PUBLIC_KEY_HANDLE pVerifier = nullptr;
	bPassed = Expect(credcore::CreateRs256PublicKeyVerifier(strRsaKeyId,
		strRsaModulus, strRsaExponent, pVerifier) == credcore::CREDCORE_RS256_SUCCESS &&
		pVerifier != nullptr, "RS256 JWK public key") && bPassed;

	std::string strDecodedPayload;
	bPassed = Expect(pVerifier != nullptr &&
		credcore::VerifyRs256JwtSignature(pVerifier, strValidJwt, strDecodedPayload) ==
		credcore::CREDCORE_RS256_SUCCESS && strDecodedPayload == strExpectedPayload,
		"RS256 valid signature") && bPassed;

	std::string strTamperedJwt = strValidJwt;
	const std::size_t szSecondDot = strTamperedJwt.find('.', strTamperedJwt.find('.') + 1);
	if (szSecondDot != std::string::npos && szSecondDot > 0)
	{
		strTamperedJwt[szSecondDot - 1] = '4';
	}
	bPassed = Expect(credcore::VerifyRs256JwtSignature(
		pVerifier, strTamperedJwt, strDecodedPayload) ==
		credcore::CREDCORE_RS256_SIGNATURE_MISMATCH && strDecodedPayload.empty(),
		"RS256 tampered payload") && bPassed;

	const std::string strWrongKeyJwt =
		"eyJhbGciOiJSUzI1NiIsImtpZCI6Indyb25nLWtleSJ9" +
		strValidJwt.substr(strValidJwt.find('.'));
	bPassed = Expect(credcore::VerifyRs256JwtSignature(
		pVerifier, strWrongKeyJwt, strDecodedPayload) ==
		credcore::CREDCORE_RS256_KEY_ID_MISMATCH,
		"RS256 key id mismatch") && bPassed;

	const std::string strUnsupportedAlgorithmJwt =
		"eyJhbGciOiJIUzI1NiIsImtpZCI6ImNyZWRjb3JlLXRlc3Qta2V5In0" +
		strValidJwt.substr(strValidJwt.find('.'));
	bPassed = Expect(credcore::ReadRs256JwtKeyId(
		strUnsupportedAlgorithmJwt, strParsedKeyId) ==
		credcore::CREDCORE_RS256_UNSUPPORTED_ALGORITHM,
		"JWT algorithm allowlist") && bPassed;
	const std::string strCriticalHeaderJwt =
		"eyJhbGciOiJSUzI1NiIsImtpZCI6ImNyZWRjb3JlLXRlc3Qta2V5IiwiY3JpdCI6WyJmb28iXX0" +
		strValidJwt.substr(strValidJwt.find('.'));
	bPassed = Expect(credcore::ReadRs256JwtKeyId(
		strCriticalHeaderJwt, strParsedKeyId) ==
		credcore::CREDCORE_RS256_UNSUPPORTED_ALGORITHM,
		"JWT unsupported critical header") && bPassed;
	const std::string strCommentedHeaderJwt =
		"eyJhbGciOiJSUzI1NiIsImtpZCI6ImNyZWRjb3JlLXRlc3Qta2V5Ii8qY29tbWVudCovfQ" +
		strValidJwt.substr(strValidJwt.find('.'));
	bPassed = Expect(credcore::ReadRs256JwtKeyId(
		strCommentedHeaderJwt, strParsedKeyId) ==
		credcore::CREDCORE_RS256_INVALID_HEADER_JSON,
		"JWT reject non-standard JSON comments") && bPassed;

	std::string strMalformedJwt = strValidJwt;
	strMalformedJwt.back() = '=';
	bPassed = Expect(credcore::VerifyRs256JwtSignature(
		pVerifier, strMalformedJwt, strDecodedPayload) ==
		credcore::CREDCORE_RS256_INVALID_BASE64URL,
		"JWT strict Base64URL") && bPassed;
	bPassed = Expect(credcore::ReadRs256JwtKeyId(
		"A.A.A", strParsedKeyId) == credcore::CREDCORE_RS256_INVALID_BASE64URL,
		"JWT Base64URL invalid length remainder") && bPassed;
	bPassed = Expect(credcore::ReadRs256JwtKeyId(
		"AB.A.A", strParsedKeyId) == credcore::CREDCORE_RS256_INVALID_BASE64URL,
		"JWT Base64URL non-zero trailing bits") && bPassed;

	credcore::CREDCORE_RS256_PUBLIC_KEY_HANDLE pInvalidVerifier = pVerifier;
	bPassed = Expect(credcore::CreateRs256PublicKeyVerifier(strRsaKeyId,
		"AQAB", strRsaExponent, pInvalidVerifier) ==
		credcore::CREDCORE_RS256_INVALID_RSA_KEY_SIZE && pInvalidVerifier == nullptr,
		"JWK RSA key size") && bPassed;
	bPassed = Expect(credcore::CreateRs256PublicKeyVerifier(strRsaKeyId,
		"AAE", strRsaExponent, pInvalidVerifier) ==
		credcore::CREDCORE_RS256_INVALID_JWK && pInvalidVerifier == nullptr,
		"JWK leading zero") && bPassed;
	bPassed = Expect(credcore::CreateRs256PublicKeyVerifier(strRsaKeyId,
		"", strRsaExponent, pInvalidVerifier) ==
		credcore::CREDCORE_RS256_INVALID_ARGUMENT && pInvalidVerifier == nullptr,
		"JWK empty modulus argument") && bPassed;

	std::atomic<bool> bConcurrentPassed(true);
	std::vector<std::thread> aWorkers;
	for (int iWorker = 0; iWorker < 8; ++iWorker)
	{
		aWorkers.emplace_back([&]()
		{
			for (int iAttempt = 0; iAttempt < 100; ++iAttempt)
			{
				std::string strConcurrentPayload;
				if (credcore::VerifyRs256JwtSignature(pVerifier, strValidJwt,
					strConcurrentPayload) != credcore::CREDCORE_RS256_SUCCESS ||
					strConcurrentPayload != strExpectedPayload)
				{
					bConcurrentPassed.store(false);
					return;
				}
			}
		});
	}
	for (std::thread& clWorker : aWorkers)
	{
		clWorker.join();
	}
	bPassed = Expect(bConcurrentPassed.load(),
		"RS256 concurrent verification") && bPassed;

	credcore::DestroyRs256PublicKeyVerifier(pVerifier);
	credcore::DestroyRs256PublicKeyVerifier(nullptr);

	if (!bPassed)
	{
		return 1;
	}
	std::cout << "CREDCORE_TESTS_PASSED" << std::endl;
	return 0;
}
