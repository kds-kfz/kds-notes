#include "CredCore.h"
#include "CredCoreC.h"

#include <algorithm>
#include <array>
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

	// 将测试字符串映射为同步 C ABI 视图；返回结构不拥有数据，调用期间原字符串保持不变。
	ST_FACADE_STRING_VIEW MakeFacadeStringView(const std::string& p_refValue)
	{
		ST_FACADE_STRING_VIEW stView = {};
		stView.uiStructSize = sizeof(stView);
		stView.uiAbiVersion = FACADE_ABI_VERSION;
		stView.pszData = p_refValue.data();
		stView.uiLength = p_refValue.size();
		return stView;
	}

	// 将固定测试数组映射为调用方输出缓冲区；DLL 只借用内存且不得保存指针。
	template <std::size_t t_szCapacity>
	ST_FACADE_MUTABLE_BUFFER MakeFacadeBuffer(
		std::array<std::uint8_t, t_szCapacity>& p_refBuffer)
	{
		ST_FACADE_MUTABLE_BUFFER stBuffer = {};
		stBuffer.uiStructSize = sizeof(stBuffer);
		stBuffer.uiAbiVersion = FACADE_ABI_VERSION;
		stBuffer.pData = p_refBuffer.data();
		stBuffer.uiCapacity = p_refBuffer.size();
		return stBuffer;
	}
}

// 依次验证历史编码/授权兼容性和 RS256 安全边界；全部通过返回 0，否则返回 1。
// 测试包含并发只读验签，不连接网络或业务数据库。
int main()
{
	bool bPassed = true;
	ST_FACADE_ABI_INFO stAbiInfo = {};
	stAbiInfo.uiStructSize = sizeof(stAbiInfo);
	stAbiInfo.uiAbiVersion = FACADE_ABI_VERSION;
	bPassed = Expect(CredCore_GetAbiInfo(&stAbiInfo) == FACADE_SUCCESS &&
		std::string(stAbiInfo.szFacadeName) == "CredCore" &&
		std::string(stAbiInfo.szImplementationVersion) == "1.0.0.3" &&
		std::string(stAbiInfo.szDependencyVersion) == "OpenSSL-3.5.6",
		"C ABI information") && bPassed;
	ST_FACADE_ABI_INFO stWrongAbiInfo = stAbiInfo;
	stWrongAbiInfo.uiAbiVersion = FACADE_ABI_VERSION + 1U;
	bPassed = Expect(CredCore_GetAbiInfo(&stWrongAbiInfo) == FACADE_ABI_MISMATCH,
		"C ABI version mismatch") && bPassed;
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
	const ST_FACADE_STRING_VIEW stClientInfoView = MakeFacadeStringView(strClientInfo);
	const ST_FACADE_STRING_VIEW stDailyFeatureView = MakeFacadeStringView(strDailyFeature);
	std::array<std::uint8_t, 256> aFeatureInfo = {};
	ST_FACADE_MUTABLE_BUFFER stFeatureInfoBuffer = MakeFacadeBuffer(aFeatureInfo);
	std::int32_t iOperationResult = -1;
	bPassed = Expect(CredCore_AuthFeatureByPlainClientInfo(&stClientInfoView,
		&stDailyFeatureView, &stFeatureInfoBuffer, &iOperationResult) == FACADE_SUCCESS &&
		iOperationResult == 0 && stFeatureInfoBuffer.uiLength > 0U,
		"C ABI daily feature validation") && bPassed;
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
	const ST_FACADE_STRING_VIEW stValidJwtView = MakeFacadeStringView(strValidJwt);
	std::array<std::uint8_t, 64> aFingerprint = {};
	ST_FACADE_MUTABLE_BUFFER stFingerprintBuffer = MakeFacadeBuffer(aFingerprint);
	iOperationResult = CREDCORE_C_RS256_INTERNAL_ERROR;
	bPassed = Expect(CredCore_GetCompactJwtSha256Fingerprint(&stValidJwtView,
		&stFingerprintBuffer, &iOperationResult) == FACADE_SUCCESS &&
		iOperationResult == CREDCORE_C_RS256_SUCCESS &&
		std::string(reinterpret_cast<const char*>(aFingerprint.data()),
			stFingerprintBuffer.uiLength) == strFingerprint,
		"C ABI JWT fingerprint") && bPassed;
	std::array<std::uint8_t, 8> aSmallFingerprint = {};
	aSmallFingerprint.fill(0xA5U);
	ST_FACADE_MUTABLE_BUFFER stSmallFingerprintBuffer =
		MakeFacadeBuffer(aSmallFingerprint);
	iOperationResult = CREDCORE_C_RS256_INTERNAL_ERROR;
	bPassed = Expect(CredCore_GetCompactJwtSha256Fingerprint(&stValidJwtView,
		&stSmallFingerprintBuffer, &iOperationResult) == FACADE_BUFFER_TOO_SMALL &&
		iOperationResult == CREDCORE_C_RS256_SUCCESS &&
		stSmallFingerprintBuffer.uiLength == 64U &&
		std::all_of(aSmallFingerprint.begin(), aSmallFingerprint.end(),
			[](std::uint8_t p_u8Value) { return p_u8Value == 0xA5U; }),
		"C ABI output buffer is atomic") && bPassed;

	std::string strParsedKeyId;
	bPassed = Expect(credcore::ReadRs256JwtKeyId(strValidJwt, strParsedKeyId) ==
		credcore::CREDCORE_RS256_SUCCESS && strParsedKeyId == strRsaKeyId,
		"RS256 header key id") && bPassed;

	credcore::CREDCORE_RS256_PUBLIC_KEY_HANDLE pVerifier = nullptr;
	bPassed = Expect(credcore::CreateRs256PublicKeyVerifier(strRsaKeyId,
		strRsaModulus, strRsaExponent, pVerifier) == credcore::CREDCORE_RS256_SUCCESS &&
		pVerifier != nullptr, "RS256 JWK public key") && bPassed;
	const ST_FACADE_STRING_VIEW stKeyIdView = MakeFacadeStringView(strRsaKeyId);
	const ST_FACADE_STRING_VIEW stModulusView = MakeFacadeStringView(strRsaModulus);
	const ST_FACADE_STRING_VIEW stExponentView = MakeFacadeStringView(strRsaExponent);
	CREDCORE_PUBLIC_KEY_HANDLE pCVerifier = nullptr;
	iOperationResult = CREDCORE_C_RS256_INTERNAL_ERROR;
	bPassed = Expect(CredCore_CreateRs256PublicKeyVerifier(&stKeyIdView,
		&stModulusView, &stExponentView, &pCVerifier, &iOperationResult) == FACADE_SUCCESS &&
		iOperationResult == CREDCORE_C_RS256_SUCCESS && pCVerifier != nullptr,
		"C ABI RS256 JWK public key") && bPassed;

	std::string strDecodedPayload;
	bPassed = Expect(pVerifier != nullptr &&
		credcore::VerifyRs256JwtSignature(pVerifier, strValidJwt, strDecodedPayload) ==
		credcore::CREDCORE_RS256_SUCCESS && strDecodedPayload == strExpectedPayload,
		"RS256 valid signature") && bPassed;
	std::vector<std::uint8_t> aPayload(strValidJwt.size());
	ST_FACADE_MUTABLE_BUFFER stPayloadBuffer = {};
	stPayloadBuffer.uiStructSize = sizeof(stPayloadBuffer);
	stPayloadBuffer.uiAbiVersion = FACADE_ABI_VERSION;
	stPayloadBuffer.pData = aPayload.data();
	stPayloadBuffer.uiCapacity = aPayload.size();
	iOperationResult = CREDCORE_C_RS256_INTERNAL_ERROR;
	bPassed = Expect(CredCore_VerifyRs256JwtSignature(pCVerifier, &stValidJwtView,
		&stPayloadBuffer, &iOperationResult) == FACADE_SUCCESS &&
		iOperationResult == CREDCORE_C_RS256_SUCCESS &&
		std::string(reinterpret_cast<const char*>(aPayload.data()),
			stPayloadBuffer.uiLength) == strExpectedPayload,
		"C ABI RS256 valid signature") && bPassed;

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
	CredCore_DestroyRs256PublicKeyVerifier(pCVerifier);
	CredCore_DestroyRs256PublicKeyVerifier(nullptr);

	if (!bPassed)
	{
		return 1;
	}
	std::cout << "CREDCORE_TESTS_PASSED" << std::endl;
	return 0;
}
