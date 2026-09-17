#ifndef H_TRADING_TERMINAL_QUOTE_BINARY_PROTOCOL
#define H_TRADING_TERMINAL_QUOTE_BINARY_PROTOCOL

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Quote Tick v1 只承载 MT SDK 原始行情和接入连续性信息。
// 本结构不包含时间权威、业务扩展价格或集群所有权；消费者必须通过各自权威组件补充业务状态。
struct ST_QUOTE_BINARY_TICK
{
	std::uint16_t usVersion;          // Quote Tick 协议版本，当前固定为 1。
	std::uint16_t usPlatformVersion;  // MT 平台版本，只允许 4 或 5。
	std::int32_t iSourceNo;           // 同平台行情节点编号，必须大于 0。
	std::uint64_t ullSourceEpoch;     // 行情流代次，进程启动和原连接重连后必须变化。
	std::uint64_t ullIngressSequence; // 当前 Version+No 和 Epoch 内严格递增的入口序号。
	std::string strSymbol;            // UTF-8 品种代码，最大 64 字节。
	double dBid;                      // SDK 原始买价，必须为有限正数。
	double dAsk;                      // SDK 原始卖价，必须为有限正数。
	double dLast;                     // SDK 原始成交价，无成交价时允许为 0。
	std::uint64_t ullVolume;          // SDK 原始成交量，MT4 不提供时为 0。
	std::uint64_t ullVolumeExt;       // MT5 扩展精度成交量，MT4 固定为 0。
	std::uint64_t ullFlags;           // SDK Tick 标志，MT4 固定为 0。
	std::int64_t llServerTime;        // SDK 原始服务器秒时间，不执行 UTC 转换。
	std::int64_t llServerTimeMsc;     // SDK 原始服务器毫秒时间，MT4 使用秒值乘 1000。
	std::int64_t llIngressTimeMs;     // Quote 收到 Tick 的 Unix 毫秒时间。

	ST_QUOTE_BINARY_TICK();
};

static const std::uint16_t QUOTE_BINARY_PROTOCOL_VERSION = 1;
static const std::size_t QUOTE_BINARY_HEADER_SIZE = 108;
static const std::size_t QUOTE_BINARY_MAX_SYMBOL_BYTES = 64;

// 校验 v1 版本、来源、品种、价格、原始时间和序号；失败返回详细英文错误。
bool ValidateQuoteBinaryTick(const ST_QUOTE_BINARY_TICK& p_refTick,
	std::string& p_refError);
// 将 v1 Tick 编码为固定小端头和 UTF-8 Symbol；函数不保留调用方内存。
bool EncodeQuoteBinaryTick(const ST_QUOTE_BINARY_TICK& p_refTick,
	std::vector<unsigned char>& p_refBuffer, std::string& p_refError);
// 严格解码单一 v1 布局；旧开发态 v3-v6、截断和尾随字节均被拒绝。
bool DecodeQuoteBinaryTick(const unsigned char* p_pBuffer,
	std::size_t p_szBufferLen, ST_QUOTE_BINARY_TICK& p_refTick,
	std::string& p_refError);

#endif // H_TRADING_TERMINAL_QUOTE_BINARY_PROTOCOL
