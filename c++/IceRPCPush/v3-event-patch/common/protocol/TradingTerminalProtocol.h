#ifndef H_TRADING_TERMINAL_PROTOCOL
#define H_TRADING_TERMINAL_PROTOCOL

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// 六个插件的固定身份，用于通知来源校验和日志定位。
enum EN_PLUGIN_ID : std::uint16_t
{
	EN_PLUGIN_ID_INVALID = 0,
	EN_PLUGIN_ID_MT_GATEWAY_SERVICE = 1,
	EN_PLUGIN_ID_MT_QUOTE_SERVICE = 2,
	EN_PLUGIN_ID_MT_TRADE_SERVICE = 3,
	EN_PLUGIN_ID_MT_QUERY_SERVICE = 4,
	EN_PLUGIN_ID_MT_EVENT_SERVICE = 5,
	EN_PLUGIN_ID_MT_DERIVE_SERVICE = 6
};

// 插件间 Binary RPC/PUT 功能号；同步和异步调用共享同一个功能号。
enum EN_PLUGIN_FUNC_ID : std::int64_t
{
	EN_PLUGIN_FUNC_COMMON_HEALTH = 1101,          // 查询服务健康状态。
	EN_PLUGIN_FUNC_COMMON_VERSION = 1102,         // 查询程序和内部协议版本。
	EN_PLUGIN_FUNC_CLUSTER_STATE = 1103,          // 查询实例、分片所有权和 fencing 状态。
	EN_PLUGIN_FUNC_MT_GATEWAY_SERVICE_DEMO_RETIRED = 1111,   // 已废弃的网关 Demo 编号，禁止重新分配。
	EN_PLUGIN_FUNC_QUOTE_DEMO_RETIRED = 1121,     // 已废弃的行情 Demo 编号，禁止重新分配。
	EN_PLUGIN_FUNC_QUOTE_SNAPSHOT = 1122,         // 历史行情快照编号；第一阶段 Quote 不处理，待消费者迁移后评估退休。
	EN_PLUGIN_FUNC_QUOTE_TIME_SNAPSHOT = 1123,    // 历史时间快照编号；第一阶段 Quote 不处理，时间权威迁移到 Query。
	EN_PLUGIN_FUNC_QUOTE_HEARTBEAT = 1124,        // Derive 定向查询 Quote 进程代次、逐来源连接和序号水位。
	EN_PLUGIN_FUNC_TRADE_DEMO_RETIRED = 1131,     // 已废弃的交易 Demo 编号，禁止重新分配。
	EN_PLUGIN_FUNC_TRADE_PLACE_ORDER = 1132,      // 普通下单。
	EN_PLUGIN_FUNC_TRADE_PLACE_ORDER_DETAIL = 1133, // 带交易预检明细的下单。
	EN_PLUGIN_FUNC_TRADE_UPDATE_ORDER = 1134,     // 修改挂单。
	EN_PLUGIN_FUNC_TRADE_CANCEL_ORDER = 1135,     // 撤销挂单。
	EN_PLUGIN_FUNC_TRADE_CREATE_ACCOUNT = 1136,   // 创建交易账户。
	EN_PLUGIN_FUNC_TRADE_CLOSE_POSITION = 1137,   // 平仓。
	EN_PLUGIN_FUNC_TRADE_BATCH_CLOSE_POSITION = 1138, // 批量平仓。
	EN_PLUGIN_FUNC_TRADE_MODIFY_POSITION = 1139,  // 修改持仓止盈止损。
	EN_PLUGIN_FUNC_TRADE_MODIFY_BALANCE = 1141,   // 修改账户余额。
	EN_PLUGIN_FUNC_TRADE_VERIFY_PASSWORD = 1142,  // 校验交易账户密码。
	EN_PLUGIN_FUNC_TRADE_HOLIDAYS = 1143,         // 查询 MT Holiday 市场日历快照。
	EN_PLUGIN_FUNC_TRADE_SYMBOL_SUSPENSIONS = 1144, // 查询品种停牌和交易限制快照。
	EN_PLUGIN_FUNC_QUERY_DEMO_RETIRED = 1151,     // 已废弃的查询 Demo 编号，禁止重新分配。
	EN_PLUGIN_FUNC_QUERY_SERVER_INFO = 1152,      // 查询服务器信息。
	EN_PLUGIN_FUNC_QUERY_SYMBOLS = 1153,          // 查询品种列表。
	EN_PLUGIN_FUNC_QUERY_SYMBOL_RATES = 1154,     // 查询品种费率。
	EN_PLUGIN_FUNC_QUERY_VOLUME_RANK = 1155,      // 查询品种成交量排名。
	EN_PLUGIN_FUNC_QUERY_QUOTES = 1156,           // 查询行情快照。
	EN_PLUGIN_FUNC_QUERY_BARS = 1157,             // 查询 K 线。
	EN_PLUGIN_FUNC_QUERY_ACCOUNTS = 1158,         // 查询账户。
	EN_PLUGIN_FUNC_QUERY_ACCOUNT_SYMBOLS = 1159,  // 查询账户可见品种。
	EN_PLUGIN_FUNC_QUERY_ACCOUNT_SYMBOL_TRADING = 1161, // 查询账户品种可交易性。
	EN_PLUGIN_FUNC_QUERY_ORDERS = 1162,           // 查询订单。
	EN_PLUGIN_FUNC_QUERY_POSITIONS = 1163,        // 查询持仓。
	EN_PLUGIN_FUNC_QUERY_DEALS = 1164,            // 查询成交。
	EN_PLUGIN_FUNC_QUERY_WATCHLIST = 1165,        // WatchList；RouteCode 区分 symbols/sections。
	EN_PLUGIN_FUNC_QUERY_CHART = 1166,            // Chart；RouteCode 区分四类子资源。
	EN_PLUGIN_FUNC_QUERY_DERIVE_STATE_SNAPSHOT = 1167, // 派生服务冷启动使用的分页权威状态快照。
	EN_PLUGIN_FUNC_QUERY_M1_BACKFILL = 1168,       // Derive 双日槽启动、缺口和归档前的纯 MT M1 回填。
	EN_PLUGIN_FUNC_QUERY_MT5_PROFIT_GROUP_QUOTE = 1169, // Derive 使用普通查询池校准 MT5 账户组有效报价；不对 Gateway 暴露。
	EN_PLUGIN_FUNC_MT_EVENT_SERVICE_DEMO_RETIRED = 1171,     // 已废弃的事件 Demo 编号，禁止重新分配。
	EN_PLUGIN_FUNC_RELIABLE_EVENT_APPEND = 1172,             // 幂等追加可靠事件。
	EN_PLUGIN_FUNC_RELIABLE_EVENT_FETCH = 1173,              // 按流序号拉取或重放可靠事件。
	EN_PLUGIN_FUNC_RELIABLE_EVENT_ACK = 1174,                // 提交消费者确认位置。
	EN_PLUGIN_FUNC_RELIABLE_EVENT_STATUS = 1175,             // 查询可靠事件中心状态。
	EN_PLUGIN_FUNC_EVENT_HEARTBEAT = 1176,                   // 查询 Event 进程、行情队列、水位和 JetStream Ready 状态。
	EN_PLUGIN_FUNC_MT_DERIVE_SERVICE_DEMO_RETIRED = 1181,    // 已废弃的派生服务 Demo 编号，禁止重新分配。
	EN_PLUGIN_FUNC_DERIVE_TICK_APPEND = 1182,                // Derive 现有 Tick 批量兼容入口；第一阶段 Quote 不调用。
	EN_PLUGIN_FUNC_DERIVE_PROFIT_DEMAND = 1183,              // Gateway 替换、续约或删除收益计算需求租约。
	EN_PLUGIN_FUNC_DERIVE_PROFIT_SNAPSHOT = 1184,            // Gateway 查询持仓及账户收益初始快照。
	EN_PLUGIN_FUNC_DERIVE_M1_SNAPSHOT = 1185,                // Query 拉取当前 M1 快照或指定缺口。
	EN_PLUGIN_FUNC_DERIVE_STATUS = 1186,                     // 查询 Derive 节点、队列、水位和计算状态。
	EN_PLUGIN_FUNC_DERIVE_M1_ARCHIVE_FETCH = 1187,           // Query 拉取已落地 spool 的确定性 M1 归档批次。
	EN_PLUGIN_FUNC_DERIVE_M1_ARCHIVE_ACK = 1188,             // Query 在全部周期和 coverage 落盘后确认归档批次。
	EN_PLUGIN_FUNC_DIAGNOSTIC_ECHO = 1191         // 回显 Binary 载荷，用于链路诊断。
};

// 插件间主动通知号；该值通过 CloudNetDataApi::Publish 的 ReqNo 传递。
enum EN_PLUGIN_NOTIFY_ID : std::int64_t
{
	EN_PLUGIN_NOTIFY_SERVICE_STATE = 1201,        // 服务启动、就绪、停止或异常状态变化。
	EN_PLUGIN_NOTIFY_MARKET_TICK = 1211,          // MT 行情 Tick 通知。
	EN_PLUGIN_NOTIFY_ORDER_CHANGED = 1221,        // 订单状态变化通知。
	EN_PLUGIN_NOTIFY_DEAL_CHANGED = 1222,         // 成交变化通知。
	EN_PLUGIN_NOTIFY_POSITION_CHANGED = 1223,     // 持仓变化通知。
	EN_PLUGIN_NOTIFY_MARGIN_CHANGED = 1224,       // 保证金变化通知。
	EN_PLUGIN_NOTIFY_TRADE_SOURCE_STATE = 1225,   // 交易数据源状态变化通知。
	EN_PLUGIN_NOTIFY_SYMBOL_CHANGED = 1241,       // 品种配置变化通知。
	EN_PLUGIN_NOTIFY_USER_CHANGED = 1242,         // 用户配置变化通知。
	EN_PLUGIN_NOTIFY_GROUP_CHANGED = 1243,        // 分组配置变化通知。
	EN_PLUGIN_NOTIFY_SERVER_TIME_CHANGED = 1244,  // 服务器时区、UTC 偏移或冬夏令状态可靠变化通知。
	EN_PLUGIN_NOTIFY_HOLIDAY_CHANGED = 1245,      // Holiday 权威快照变化通知。
	EN_PLUGIN_NOTIFY_SYMBOL_SUSPENSION_CHANGED = 1246, // 品种停牌权威快照变化通知。
	EN_PLUGIN_NOTIFY_PROFIT_CHANGED = 1251,       // 收益变化通知。
	EN_PLUGIN_NOTIFY_KLINE_CHANGED = 1252,        // K线变化通知。
	EN_PLUGIN_NOTIFY_DIAGNOSTIC_TEST = 1291       // 诊断推送通知。
};

// 通知投递语义；可靠通知的持久化和重放由 MtEventService 业务层实现。
enum EN_PLUGIN_NOTIFY_MODE : std::uint16_t
{
	EN_PLUGIN_NOTIFY_MODE_INVALID = 0,
	EN_PLUGIN_NOTIFY_MODE_BEST_EFFORT = 1,
	EN_PLUGIN_NOTIFY_MODE_RELIABLE = 2,
	EN_PLUGIN_NOTIFY_MODE_SNAPSHOT = 3,
	EN_PLUGIN_NOTIFY_MODE_CONTROL = 4
};

// 通知对应的数据动作，消费者根据动作决定覆盖、合并或删除本地状态。
enum EN_PLUGIN_NOTIFY_ACTION : std::uint16_t
{
	EN_PLUGIN_NOTIFY_ACTION_INVALID = 0,
	EN_PLUGIN_NOTIFY_ACTION_CREATED = 1,
	EN_PLUGIN_NOTIFY_ACTION_UPDATED = 2,
	EN_PLUGIN_NOTIFY_ACTION_DELETED = 3,
	EN_PLUGIN_NOTIFY_ACTION_STATE_CHANGED = 4,
	EN_PLUGIN_NOTIFY_ACTION_RESET = 5
};

// 集群实例或业务分片当前角色。只有 OWNER 可以产生状态型副作用。
enum EN_CLUSTER_ROLE : std::uint16_t
{
	EN_CLUSTER_ROLE_DISABLED = 0,       // 当前环境关闭集群，保持单实例兼容行为。
	EN_CLUSTER_ROLE_STANDBY = 1,        // 实例健康但未持有目标分片租约。
	EN_CLUSTER_ROLE_OWNER = 2,          // 当前实例持有有效租约并允许产生副作用。
	EN_CLUSTER_ROLE_RECOVERING = 3,     // 已取得租约但业务状态尚未完成恢复。
	EN_CLUSTER_ROLE_DEGRADED = 4        // 无法确认租约或依赖失效，必须停止副作用。
};

// 状态型写入携带的 fencing token。线上协议必须显式编码字符串和固定宽度整数。
struct ST_CLUSTER_FENCE
{
	std::uint16_t usVersion;            // fencing 协议版本，当前固定为 1。
	std::string strOwnerInstanceId;     // 产生当前写入的 Owner 实例 ID。
	std::uint64_t ullLeaseGeneration;   // NATS KV revision 派生的严格递增租约代次。

	ST_CLUSTER_FENCE();
};

// 单个 Version+No 分片的租约快照，不保存 NATS 凭据或业务状态正文。
struct ST_CLUSTER_LEASE
{
	std::uint16_t usPlatformVersion;    // MT 平台版本，只允许 4 或 5。
	std::int32_t iSourceNo;             // 同平台节点编号，必须大于 0。
	EN_CLUSTER_ROLE enRole;             // 当前实例对该分片的角色。
	std::string strOwnerInstanceId;     // 当前 KV 中记录的 Owner 实例。
	std::uint64_t ullLeaseGeneration;   // 当前租约 KV revision，旧代次写入必须拒绝。
	std::int64_t llExpiresAtMs;          // 当前租约到期的 Unix 毫秒时间。
	std::int64_t llLastRenewedMs;        // 本实例最近成功续约的 Unix 毫秒时间。
	bool bDataReady;                     // Owner 是否已经完成业务状态恢复。

	ST_CLUSTER_LEASE();
};

// 1103 可选无符号运行指标。名称是稳定 ASCII 键，值不承载地址、账号或密码。
struct ST_CLUSTER_RUNTIME_METRIC
{
	std::string strName;                 // 指标名，最长 128 字节。
	std::uint64_t ullValue;              // 当前快照值或累计计数。

	ST_CLUSTER_RUNTIME_METRIC();
};

// 当前服务实例状态。1103 返回值和 KV 心跳均使用该结构的显式编码。
struct ST_CLUSTER_INSTANCE
{
	std::uint16_t usVersion;            // 集群状态协议版本，当前固定为 2。
	EN_PLUGIN_ID enPluginId;            // 当前六服务插件身份。
	EN_CLUSTER_ROLE enRole;             // 无分片服务的实例角色或全部分片聚合角色。
	std::string strServiceName;         // 当前服务名称。
	std::string strInstanceId;          // 当前部署实例唯一 ID。
	std::string strNodeId;              // 当前 IceGrid Node 名称。
	std::string strAdapterId;           // 当前实例 AdapterId，供诊断和实例级路由。
	bool bProcessReady;                 // 网络和派生组件是否已经完成启动。
	std::int64_t llUpdatedAtMs;         // 最近状态更新时间。
	std::string strDetail;              // 不含凭据的详细英文状态。

	ST_CLUSTER_INSTANCE();
};

// 1103 应答包含当前实例和其登记的全部分片租约，调用方只持有值副本。
struct ST_CLUSTER_STATE_RESPONSE
{
	std::uint16_t usVersion;            // 集群状态协议版本，当前固定为 2，解码兼容 v1。
	ST_CLUSTER_INSTANCE stInstance;     // 当前进程实例状态。
	std::vector<ST_CLUSTER_LEASE> aLease;// 按 Version+No 排序的分片状态。
	std::vector<ST_CLUSTER_RUNTIME_METRIC> aMetric; // v2 可选有界运行指标，v1 解码为空。

	ST_CLUSTER_STATE_RESPONSE();
};

// 1167 派生状态快照实体位；调用方可组合多个位，未知位必须拒绝。
enum EN_DERIVE_STATE_ENTITY_MASK : std::uint32_t
{
	EN_DERIVE_STATE_ENTITY_ACCOUNT = 0x00000001U,  // 账户资金和币种状态。
	EN_DERIVE_STATE_ENTITY_POSITION = 0x00000002U, // 当前持仓状态。
	EN_DERIVE_STATE_ENTITY_SYMBOL = 0x00000004U,   // 品种合约和币种属性。
	EN_DERIVE_STATE_ENTITY_GROUP = 0x00000008U,    // 用户组和默认杠杆属性。
	EN_DERIVE_STATE_ENTITY_RATE = 0x00000010U,     // 最新汇率边。
	EN_DERIVE_STATE_ENTITY_ALL = 0x0000001FU       // 第一阶段全部收益依赖实体。
};

// MT 服务器时间同步状态。消费者只有在 READY 且未超过 ValidUntilUtcMs 时才允许执行时间相关业务。
enum EN_MT_TIME_SYNC_STATE : std::uint16_t
{
	EN_MT_TIME_SYNC_UNKNOWN = 0,
	EN_MT_TIME_SYNC_SYNCING = 1,
	EN_MT_TIME_SYNC_READY = 2,
	EN_MT_TIME_SYNC_STALE = 3,
	EN_MT_TIME_SYNC_MISMATCH = 4
};

// 时间状态变化原因，用于诊断连接恢复、SDK 通知、周期校准和配置热更新。
enum EN_MT_TIME_CHANGE_REASON : std::uint16_t
{
	EN_MT_TIME_CHANGE_INITIAL = 1,
	EN_MT_TIME_CHANGE_PERIODIC = 2,
	EN_MT_TIME_CHANGE_SDK_UPDATE = 3,
	EN_MT_TIME_CHANGE_SDK_SYNC = 4,
	EN_MT_TIME_CHANGE_CONFIG_RELOAD = 5,
	EN_MT_TIME_CHANGE_RECONNECT = 6
};

// 1123 查询条件。PlatformVersion=0 且 SourceNo=0 表示查询全部节点，其余情况必须同时指定。
struct ST_MT_TIME_SNAPSHOT_REQUEST
{
	std::uint16_t usVersion;          // 时间协议版本，当前固定为 1。
	std::uint16_t usPlatformVersion;  // MT 平台版本，0 表示全部，否则只允许 4 或 5。
	std::int32_t iSourceNo;           // 节点编号，0 表示全部，否则必须大于 0。

	ST_MT_TIME_SNAPSHOT_REQUEST();
};

// 单个 Version+No 的时间权威状态。线上数据使用显式小端编码，禁止发送结构体原始内存。
struct ST_MT_TIME_STATE
{
	std::uint16_t usVersion;            // 时间协议版本，当前固定为 1。
	std::uint16_t usPlatformVersion;    // MT 平台版本，只允许 4 或 5。
	std::int32_t iSourceNo;             // 同平台节点编号，必须大于 0。
	std::string strTimeZoneId;          // Windows 动态时区 ID，用于历史和未来日期规则。
	std::int32_t iStandardOffsetSeconds;// MT 配置的标准 UTC 偏移秒数。
	std::int32_t iCurrentOffsetSeconds; // 当前实测 UTC 偏移秒数，包含当前冬夏令修正。
	bool bDaylight;                     // 当前是否处于夏令时状态。
	EN_MT_TIME_SYNC_STATE enSyncState;  // 当前采样是否已经确认并可供业务使用。
	EN_MT_TIME_CHANGE_REASON enChangeReason; // 本代状态的产生原因。
	std::uint64_t ullAuthorityEpoch;    // Quote 时间权威生命周期，重启或配置热更新后变化。
	std::uint64_t ullGeneration;        // 同一 Epoch 内单调递增的时间代次。
	std::int64_t llEffectiveUtcMs;      // 当前偏移开始生效的 UTC 毫秒时间。
	std::int64_t llSampledUtcMs;        // 最近一次成功采样的 UTC 毫秒时间。
	std::int64_t llValidUntilUtcMs;     // 超过该 UTC 毫秒时间后状态必须按 STALE 处理。

	ST_MT_TIME_STATE();
};

// 1123 应答，按 Version+No 排序保存匹配的时间状态。
struct ST_MT_TIME_SNAPSHOT_RESPONSE
{
	std::uint16_t usVersion;            // 时间协议版本，当前固定为 1。
	std::vector<ST_MT_TIME_STATE> aState;// 匹配的节点状态，查询全部时可包含多个元素。

	ST_MT_TIME_SNAPSHOT_RESPONSE();
};

// 通知正文前的逻辑元数据；线上数据由显式编解码函数生成，不直接发送结构体内存。
struct ST_PLUGIN_NOTIFY_META
{
	std::uint16_t usVersion;          // 通知元数据版本，当前固定为 1。
	std::uint16_t usHeaderSize;       // 编码后元数据长度，当前固定为 32 字节。
	std::uint16_t usSourcePlugin;     // 通知来源插件，对应 EN_PLUGIN_ID。
	std::uint16_t usNotifyMode;       // 投递模式，对应 EN_PLUGIN_NOTIFY_MODE。
	std::uint16_t usNotifyAction;     // 数据动作，对应 EN_PLUGIN_NOTIFY_ACTION。
	std::uint16_t usReserved;         // 保留字段，发送前必须设置为 0。
	std::uint32_t uiPayloadLen;       // 元数据后业务正文长度，单位为字节。
	std::uint64_t ullSequence;        // 来源插件内递增序号，Best Effort 通知允许为 0。
	std::int64_t llTimestampMs;       // 通知产生时间，Unix 毫秒时间戳。

	ST_PLUGIN_NOTIFY_META();
};

static const std::uint16_t TRADING_TERMINAL_PROTOCOL_VERSION = 1;
static const std::size_t TRADING_TERMINAL_NOTIFY_META_SIZE = 32;
static const std::uint16_t MT_TIME_PROTOCOL_VERSION = 1;
static const std::uint16_t CLUSTER_PROTOCOL_VERSION_V1 = 1;
static const std::uint16_t CLUSTER_PROTOCOL_VERSION = 2;
static const std::size_t CLUSTER_MAX_TEXT_BYTES = 128;
static const std::size_t CLUSTER_MAX_LEASE_COUNT = 1024;
static const std::size_t CLUSTER_MAX_METRIC_COUNT = 256;
static const std::size_t MT_TIME_MAX_TIME_ZONE_ID_BYTES = 128;

// 11xx 功能号分段边界，业务实现只能引用这些常量，禁止直接书写协议数字。
static const std::int64_t PLUGIN_FUNC_COMMON_BEGIN = 1100;
static const std::int64_t PLUGIN_FUNC_COMMON_END = 1109;
static const std::int64_t PLUGIN_FUNC_MT_GATEWAY_SERVICE_BEGIN = 1110;
static const std::int64_t PLUGIN_FUNC_MT_GATEWAY_SERVICE_END = 1119;
static const std::int64_t PLUGIN_FUNC_QUOTE_BEGIN = 1120;
static const std::int64_t PLUGIN_FUNC_QUOTE_END = 1129;
static const std::int64_t PLUGIN_FUNC_TRADE_BEGIN = 1130;
static const std::int64_t PLUGIN_FUNC_TRADE_END = 1149;
static const std::int64_t PLUGIN_FUNC_QUERY_BEGIN = 1150;
static const std::int64_t PLUGIN_FUNC_QUERY_END = 1169;
static const std::int64_t PLUGIN_FUNC_MT_EVENT_SERVICE_BEGIN = 1170;
static const std::int64_t PLUGIN_FUNC_MT_EVENT_SERVICE_END = 1179;
static const std::int64_t PLUGIN_FUNC_MT_DERIVE_SERVICE_BEGIN = 1180;
static const std::int64_t PLUGIN_FUNC_MT_DERIVE_SERVICE_END = 1189;
static const std::int64_t PLUGIN_FUNC_DIAGNOSTIC_BEGIN = 1190;
static const std::int64_t PLUGIN_FUNC_DIAGNOSTIC_END = 1199;

// 12xx 通知号总边界，细分范围和已发布编号由 EN_PLUGIN_NOTIFY_ID 登记。
static const std::int64_t PLUGIN_NOTIFY_BEGIN = 1200;
static const std::int64_t PLUGIN_NOTIFY_END = 1299;

// CloudNetDataApi 当前会把 IceRPCPush 的客户端生命周期控制通知透传到服务回调。
// 这些编号仅用于传输层维护订阅关系，不能进入 12xx 业务通知解码和分发流程。
static const std::int64_t TRANSPORT_NOTIFY_CLIENT_ADDED = 0x1FFFFFFFFFFFFFF0LL;
static const std::int64_t TRANSPORT_NOTIFY_CLIENT_SUBSCRIBED = 0x1FFFFFFFFFFFFFF1LL;
static const std::int64_t TRANSPORT_NOTIFY_CLIENT_UNSUBSCRIBED = 0x1FFFFFFFFFFFFFF2LL;
static const std::int64_t TRANSPORT_NOTIFY_CLIENT_DELETED = 0x1FFFFFFFFFFFFFFFLL;

// 1165 的 WatchList 子路由编码，只在 FuncId=1165 时有效。
enum EN_PLUGIN_WATCHLIST_ROUTE_CODE : std::int64_t
{
	EN_PLUGIN_WATCHLIST_ROUTE_SYMBOLS = 1,
	EN_PLUGIN_WATCHLIST_ROUTE_SECTIONS = 2
};

// 1166 的 Chart 子路由编码，只在 FuncId=1166 时有效。
enum EN_PLUGIN_CHART_ROUTE_CODE : std::int64_t
{
	EN_PLUGIN_CHART_ROUTE_DRAWINGS = 1,
	EN_PLUGIN_CHART_ROUTE_DRAWINGS_SYNC = 2,
	EN_PLUGIN_CHART_ROUTE_INDICATORS = 3,
	EN_PLUGIN_CHART_ROUTE_CONFIG = 4
};

// 判断功能号是否属于 11xx 内部调用范围，并排除各段未分配的 x0 起点。
bool IsPluginFuncId(std::int64_t p_llFuncId);
// 判断功能号是否为 MtTradeService 已发布且仍在使用的正式接口，排除退役号和预留号。
bool IsActiveMtTradeFuncId(std::int64_t p_llFuncId);
// 判断功能号是否为 MtDeriveService 已发布且仍在使用的正式接口。
bool IsActiveMtDeriveFuncId(std::int64_t p_llFuncId);
// 判断通知号是否属于 12xx 内部通知范围，并排除各段未分配的 x0 起点。
bool IsPluginNotifyId(std::int64_t p_llNotifyId);
// 判断通知号是否为 CloudNet/IceRPCPush 的客户端生命周期控制事件；业务层必须静默消化且不得解码正文。
bool IsTransportControlNotifyId(std::int64_t p_llNotifyId);
// 判断指定插件是否有权处理当前功能号。
bool IsPluginFuncAllowed(EN_PLUGIN_ID p_enPluginId, std::int64_t p_llFuncId);
// 根据功能号范围返回目标插件，公共和诊断功能号返回 INVALID。
EN_PLUGIN_ID GetPluginIdByFuncId(std::int64_t p_llFuncId);
// 返回插件名称，未知值返回 InvalidPlugin。
const char* GetPluginName(EN_PLUGIN_ID p_enPluginId);
// 返回已登记功能号名称，预留号返回 PLUGIN_FUNC_RESERVED。
const char* GetPluginFuncName(std::int64_t p_llFuncId);
// 返回已登记通知号名称，预留号返回 PLUGIN_NOTIFY_RESERVED。
const char* GetPluginNotifyName(std::int64_t p_llNotifyId);
// 校验一个时间状态的节点、偏移、代次、时间戳和 UTF-8 时区 ID。
bool ValidateMtTimeState(const ST_MT_TIME_STATE& p_refState,
	std::string& p_refError);
// 编码/解码单个 1244 时间状态正文，整数统一使用小端字节序。
bool EncodeMtTimeState(const ST_MT_TIME_STATE& p_refState,
	std::vector<unsigned char>& p_refBuffer, std::string& p_refError);
bool DecodeMtTimeState(const unsigned char* p_pBuffer,
	std::size_t p_szBufferLen, ST_MT_TIME_STATE& p_refState,
	std::string& p_refError);
// 编码/解码 1123 查询条件；空 Payload 不再作为隐式查询全部，调用方必须显式发送请求。
bool EncodeMtTimeSnapshotRequest(const ST_MT_TIME_SNAPSHOT_REQUEST& p_refRequest,
	std::vector<unsigned char>& p_refBuffer, std::string& p_refError);
bool DecodeMtTimeSnapshotRequest(const unsigned char* p_pBuffer,
	std::size_t p_szBufferLen, ST_MT_TIME_SNAPSHOT_REQUEST& p_refRequest,
	std::string& p_refError);
// 编码/解码 1123 应答；每个元素带独立长度，损坏元素不会造成越界读取。
bool EncodeMtTimeSnapshotResponse(const ST_MT_TIME_SNAPSHOT_RESPONSE& p_refResponse,
	std::vector<unsigned char>& p_refBuffer, std::string& p_refError);
bool DecodeMtTimeSnapshotResponse(const unsigned char* p_pBuffer,
	std::size_t p_szBufferLen, ST_MT_TIME_SNAPSHOT_RESPONSE& p_refResponse,
	std::string& p_refError);
// 显式编码通知元数据并追加正文，线上整数统一使用小端字节序。
bool EncodePluginNotify(const ST_PLUGIN_NOTIFY_META& p_refMeta, const unsigned char* p_pPayload,
	std::size_t p_szPayloadLen, std::vector<unsigned char>& p_refBuffer, std::string& p_refError);
// 解码通知元数据并返回正文视图，返回指针仅在输入缓冲区有效期内可用。
bool DecodePluginNotify(const unsigned char* p_pBuffer, std::size_t p_szBufferLen,
	ST_PLUGIN_NOTIFY_META& p_refMeta, const unsigned char*& p_refPayload, std::size_t& p_refPayloadLen,
	std::string& p_refError);
static_assert(EN_PLUGIN_FUNC_COMMON_HEALTH >= PLUGIN_FUNC_COMMON_BEGIN &&
	EN_PLUGIN_FUNC_DIAGNOSTIC_ECHO <= PLUGIN_FUNC_DIAGNOSTIC_END,
	"Internal function identifiers must remain in the 11xx range.");
static_assert(EN_PLUGIN_NOTIFY_SERVICE_STATE >= PLUGIN_NOTIFY_BEGIN &&
	EN_PLUGIN_NOTIFY_DIAGNOSTIC_TEST <= PLUGIN_NOTIFY_END,
	"Internal notification identifiers must remain in the 12xx range.");

#endif // H_TRADING_TERMINAL_PROTOCOL
