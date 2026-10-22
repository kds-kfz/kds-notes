#ifndef CREDCORE_JWT_RS256_VERIFIER_H
#define CREDCORE_JWT_RS256_VERIFIER_H

#include <openssl/types.h>

#include <string>

namespace credcore::detail
{
	// 保存单个 kid 对应的只读 RSA 公钥，供多个请求线程并发验签。
	// 结构体拥有 pPublicKey；只能在所有验签调用结束后销毁，且不承载 JWT 业务声明校验。
	struct ST_RS256_PUBLIC_KEY_VERIFIER final
	{
		// 输入已校验的 kid 和所有权待转移的 OpenSSL 公钥；构造函数不抛异常。
		ST_RS256_PUBLIC_KEY_VERIFIER(
			std::string p_strKeyId, EVP_PKEY* p_pPublicKey) noexcept;
		// 释放本结构体独占的 OpenSSL 公钥；调用方必须先停止并发验签。
		~ST_RS256_PUBLIC_KEY_VERIFIER();

		ST_RS256_PUBLIC_KEY_VERIFIER(
			const ST_RS256_PUBLIC_KEY_VERIFIER&) = delete;
		ST_RS256_PUBLIC_KEY_VERIFIER& operator=(
			const ST_RS256_PUBLIC_KEY_VERIFIER&) = delete;

		const std::string strKeyId; // 与公钥绑定的 JWT kid；构造前已完成长度和控制字符校验。
		EVP_PKEY* const pPublicKey; // 本结构体独占的只读 RSA 公钥；析构时由 EVP_PKEY_free 释放。
	};
}

#endif // CREDCORE_JWT_RS256_VERIFIER_H
