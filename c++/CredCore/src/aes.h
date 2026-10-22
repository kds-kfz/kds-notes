#ifndef CREDCORE_AES_H
#define CREDCORE_AES_H

#include <cstddef>

// 历史 AES/Rijndael 兼容内核，仅由 CredCore.cpp 创建短生命周期实例。
// 每个实例独占轮密钥、链块和工作缓冲区；实例不可跨线程共享，也不负责 Token 业务。
class CAes final
{
public:
	// 指定分组密码工作模式；当前 CredCore 授权协议固定使用 CBC。
	enum EN_AES_MODE : int
	{
		ECB = 0, // 电子密码本模式；各分组独立处理。
		CBC = 1, // 密码块链接模式；CredCore 历史授权协议固定使用该模式。
		CFB = 2 // 密文反馈模式；仅为历史内核兼容保留。
	};

private:
	enum EN_AES_DEFAULTS : int
	{
		DEFAULT_BLOCK_SIZE = 16 // 历史协议默认使用 16 字节分组和密钥。
	};
	enum EN_AES_LIMITS : int
	{
		MAX_BLOCK_SIZE = 32, // Rijndael 支持的最大分组字节数。
		MAX_ROUNDS = 14, // 256 位密钥对应的最大轮数。
		MAX_KC = 8, // 密钥扩展缓冲区的最大 32 位列数。
		MAX_BC = 8 // 分组状态的最大 32 位列数。
	};
public:
	// 初始化空 AES 上下文；使用前必须先调用 MakeKey，同一实例不可并发访问。
	CAes();
	// 清理实例状态；对象不拥有外部输入输出缓冲区。
	~CAes();
private:
	bool m_bKeyInit; // 标识轮密钥是否已生成；由 MakeKey 写入，在实例生命周期内有效。
	int m_aEncryptionRoundKeys[MAX_ROUNDS + 1][MAX_BC]; // 加密轮密钥表；实例独占，由 MakeKey 生成。
	int m_aDecryptionRoundKeys[MAX_ROUNDS + 1][MAX_BC]; // 解密轮密钥表；实例独占，由 MakeKey 生成。
	int m_iKeyLength; // 当前密钥字节数；由 MakeKey 校验并设置。
	int m_iBlockSize; // 当前分组字节数；由 MakeKey 校验并设置。
	int m_iRounds; // 当前密钥和分组组合对应的轮数；由 MakeKey 计算。
	char m_aInitialChainBlock[MAX_BLOCK_SIZE]; // 初始链块副本；每次 Encrypt/Decrypt 用它重置状态。
	char m_aChainBlock[MAX_BLOCK_SIZE]; // 当前 CBC/CFB 链块；仅在单次加解密过程中有效。
	int m_aTemporaryKey[MAX_KC]; // 密钥扩展临时缓冲区；实例独占，不包含外部所有权。
	int m_aState[MAX_BC]; // 分组变换工作状态；实例独占，仅在单次调用中有效。
	int m_aTemporaryState[MAX_BC]; // 分组变换临时状态；实例独占，仅在单次调用中有效。
private:
	// 将输入链块异或到可写缓冲区；两个指针必须至少覆盖 m_iBlockSize 字节。
	void Xor(char* p_pBuffer, char const* p_pChain);
	// 使用通用 Rijndael 路径加密一个分组；实例不可并发访问。
	void DefEncryptBlock(char const* p_pInput, char* p_pResult);
	// 使用通用 Rijndael 路径解密一个分组；实例不可并发访问。
	void DefDecryptBlock(char const* p_pInput, char* p_pResult);
	// 加密一个分组；16 字节分组使用兼容快速路径，实例不可并发访问。
	void EncryptBlock(char const* p_pInput, char* p_pResult);
	// 解密一个分组；16 字节分组使用兼容快速路径，实例不可并发访问。
	void DecryptBlock(char const* p_pInput, char* p_pResult);
public:
	// 输入密钥、初始链块和长度并生成轮密钥；指针或长度非法时保持未初始化状态并返回。
	// 函数复制输入内容，调用结束后不保留外部指针；同一实例不可并发调用。
	void MakeKey(char const* p_pKey, char const* p_pChain, int p_iKeyLength =
		DEFAULT_BLOCK_SIZE, int p_iBlockSize = DEFAULT_BLOCK_SIZE);
	// 按指定模式加密 p_szLength 字节；缓冲区必须有效且长度满足分组要求，同一实例不可并发。
	void Encrypt(char const* p_pInput, char* p_pResult, size_t p_szLength, int p_iMode = ECB);
	// 按指定模式解密 p_szLength 字节；缓冲区必须有效且长度满足分组要求，同一实例不可并发。
	void Decrypt(char const* p_pInput, char* p_pResult, size_t p_szLength, int p_iMode = ECB);
};

#endif // CREDCORE_AES_H
