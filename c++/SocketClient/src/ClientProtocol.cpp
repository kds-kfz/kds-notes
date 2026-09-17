#include "ClientProtocol.h"
#include "Base64.h"
#if defined(_WIN32)
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
#else
#include <openssl/rand.h>
#include <openssl/sha.h>
#endif
#include <cctype>
#include <climits>
#include <cstring>
#include <vector>

namespace ClientProtocol
{
// 调用从 SocketServer 原样复制的编码函数，不重复实现编码算法。
std::string EncodeBase64(const unsigned char* p_byData, size_t p_uiLength)
{
    if (!p_byData || p_uiLength > static_cast<size_t>(UINT_MAX)) return {};
    std::vector<char> buffer(((p_uiLength + 2) / 3) * 4 + 1);
    if (base64_encode(p_byData, static_cast<unsigned int>(p_uiLength), buffer.data()) != 0)
        return {};
    return std::string(buffer.data());
}

// CNG 仅负责 SHA-1 运算；二进制编码仍复用 SocketServer 的 Base64。
bool WebAccept(const std::string& p_strKey, std::string& p_refOut)
{
    const std::string input = p_strKey + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
#if defined(_WIN32)
    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    DWORD objectSize = 0, cbResult = 0;
    unsigned char digest[20] = {};
    bool ok = BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA1_ALGORITHM, nullptr, 0));
    if (ok) ok = BCRYPT_SUCCESS(BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH,
        reinterpret_cast<PUCHAR>(&objectSize), sizeof(objectSize), &cbResult, 0));
    std::vector<unsigned char> object(ok ? objectSize : 0);
    if (ok) ok = BCRYPT_SUCCESS(BCryptCreateHash(alg, &hash, object.data(), objectSize,
        nullptr, 0, 0));
    if (ok) ok = BCRYPT_SUCCESS(BCryptHashData(hash,
        reinterpret_cast<PUCHAR>(const_cast<char*>(input.data())),
        static_cast<ULONG>(input.size()), 0));
    if (ok) ok = BCRYPT_SUCCESS(BCryptFinishHash(hash, digest, sizeof(digest), 0));
    if (hash) BCryptDestroyHash(hash);
    if (alg) BCryptCloseAlgorithmProvider(alg, 0);
#else
    unsigned char digest[SHA_DIGEST_LENGTH] = {};
    const bool ok = SHA1(reinterpret_cast<const unsigned char*>(input.data()),
        input.size(), digest) != nullptr;
#endif
    if (ok) p_refOut = EncodeBase64(digest, sizeof(digest));
    return ok && !p_refOut.empty();
}

// Windows 使用 CNG，Linux 使用系统 OpenSSL 的密码学随机数源。
bool RandomBytes(unsigned char* p_byOutput, size_t p_uiLength)
{
    if (!p_byOutput || p_uiLength == 0 || p_uiLength > static_cast<size_t>(INT_MAX))
        return false;
#if defined(_WIN32)
    return BCRYPT_SUCCESS(BCryptGenRandom(nullptr, p_byOutput,
        static_cast<ULONG>(p_uiLength), BCRYPT_USE_SYSTEM_PREFERRED_RNG));
#else
    return RAND_bytes(p_byOutput, static_cast<int>(p_uiLength)) == 1;
#endif
}

// 仅对协议头名使用 ASCII 字符折叠，不改变业务内容的编码。
bool HeaderEquals(const char* p_szLeft, const char* p_szRight)
{
    if (!p_szLeft || !p_szRight) return false;
    while (*p_szLeft && *p_szRight)
    {
        if (std::tolower(static_cast<unsigned char>(*p_szLeft++)) !=
            std::tolower(static_cast<unsigned char>(*p_szRight++))) return false;
    }
    return *p_szLeft == *p_szRight;
}

// 名称规则与 SocketServer 命名工厂保持一致。
bool ValidName(const char* p_szName)
{
    if (!p_szName) return false;
    size_t length = 0;
    while (length < 64 && p_szName[length]) ++length;
    if (!length || length >= 64) return false;
    for (size_t i = 0; i < length; ++i)
    {
        const unsigned char c = static_cast<unsigned char>(p_szName[i]);
        if (!std::isalnum(c) && c != '.' && c != '_' && c != '-') return false;
    }
    return true;
}

// 将未使用的事件字段统一初始化为零。
ST_SOCKET_CLIENT_EVENT Event(EN_SOCKET_CLIENT_EVENT p_enType)
{
    ST_SOCKET_CLIENT_EVENT event = {};
    event.enEvent = p_enType;
    return event;
}
}
