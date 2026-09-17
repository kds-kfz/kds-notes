#ifndef H_TRADING_TERMINAL_EVENT_HEARTBEAT_BINARY_PROTOCOL
#define H_TRADING_TERMINAL_EVENT_HEARTBEAT_BINARY_PROTOCOL

#include "TradingTerminalProtocol.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Event 心跳中的单来源水位，不携带 Symbol、价格或连接凭据。
struct ST_EVENT_HEARTBEAT_SOURCE_STATUS
{
	std::uint16_t usPlatformVersion; // MT 平台版本，只允许 4 或 5。
	std::int32_t iSourceNo;          // 行情节点编号，必须大于 0。
	std::uint64_t ullSourceEpoch;    // Event 最近接受的 Quote 来源代次。
	std::uint64_t ullLastSequence;   // 当前代次最近接受的 Quote 入口序号。
	std::uint64_t ullGapCount;       // 当前进程观察到的来源序号缺口累计数。

	ST_EVENT_HEARTBEAT_SOURCE_STATUS();
};

// 1176 定向心跳请求。调用方身份使用六服务现有 EN_PLUGIN_ID，不建立第二套编号。
struct ST_EVENT_HEARTBEAT_REQUEST
{
	std::uint16_t usVersion;          // 心跳协议版本，当前固定为 1。
	EN_PLUGIN_ID enSenderPluginId;    // 发起心跳的插件编号。
	EN_PLUGIN_ID enTargetPluginId;    // 目标固定为 MtEventService。
	std::uint64_t ullRequestSequence; // 发起方进程内递增请求序号。
	std::uint64_t ullSenderProcessEpoch; // 发起方进程代次。
	std::int64_t llSentAtMs;          // 发起时间，Unix 毫秒。

	ST_EVENT_HEARTBEAT_REQUEST();
};

// 1176 定向心跳应答，返回传输状态和逐来源水位，不暴露业务正文或 NATS 地址。
struct ST_EVENT_HEARTBEAT_RESPONSE
{
	std::uint16_t usVersion;             // 心跳协议版本，当前固定为 1。
	EN_PLUGIN_ID enRequesterPluginId;    // 原请求方插件编号。
	EN_PLUGIN_ID enResponderPluginId;    // 应答方固定为 MtEventService。
	bool bJetStreamReady;                // ReliableEvent 是否已连接并取得 Stream。
	std::uint64_t ullRequestSequence;    // 原样回显请求序号。
	std::uint64_t ullResponderProcessEpoch; // 当前 Event 进程代次。
	std::int64_t llRespondedAtMs;        // 应答生成时间，Unix 毫秒。
	std::uint64_t ullQueueCapacity;      // 配置总容量，0 表示不主动容量淘汰。
	std::uint64_t ullQueueDepth;         // 当前全部分片待发布数量。
	std::uint64_t ullReceivedCount;      // 进入 Event 1211 校验的累计数量。
	std::uint64_t ullValidationRejectedCount; // 协议或元数据校验拒绝数量。
	std::uint64_t ullDuplicateCount;     // 同 Epoch 重复序号数量。
	std::uint64_t ullOutOfOrderCount;    // 同 Epoch 回退序号数量。
	std::uint64_t ullEpochResetCount;    // 来源 Epoch 变化数量。
	std::uint64_t ullSequenceGapCount;   // 来源 Sequence 跳跃数量。
	std::uint64_t ullCapacityDroppedCount; // 正容量模式淘汰最旧 Tick 数量。
	std::uint64_t ullFanoutCount;        // 成功本地 Ice 扇出数量。
	std::uint64_t ullPublishFailedCount; // 本地 Ice 扇出失败数量。
	std::vector<ST_EVENT_HEARTBEAT_SOURCE_STATUS> aSource; // 稳定排序的来源水位。

	ST_EVENT_HEARTBEAT_RESPONSE();
};

// 编码 1176 请求；失败返回详细英文错误且清空输出。
bool EncodeEventHeartbeatRequest(const ST_EVENT_HEARTBEAT_REQUEST& p_refRequest,
	std::vector<unsigned char>& p_refBuffer, std::string& p_refError);
// 解码 1176 请求并拒绝错误插件身份、版本、长度和尾随字节。
bool DecodeEventHeartbeatRequest(const unsigned char* p_pBuffer,
	std::size_t p_szBufferLen, ST_EVENT_HEARTBEAT_REQUEST& p_refRequest,
	std::string& p_refError);
// 编码 1176 应答；来源数量和总长度均执行显式边界检查。
bool EncodeEventHeartbeatResponse(const ST_EVENT_HEARTBEAT_RESPONSE& p_refResponse,
	std::vector<unsigned char>& p_refBuffer, std::string& p_refError);
// 解码 1176 应答并校验全部计数、身份和来源水位。
bool DecodeEventHeartbeatResponse(const unsigned char* p_pBuffer,
	std::size_t p_szBufferLen, ST_EVENT_HEARTBEAT_RESPONSE& p_refResponse,
	std::string& p_refError);

#endif // H_TRADING_TERMINAL_EVENT_HEARTBEAT_BINARY_PROTOCOL
