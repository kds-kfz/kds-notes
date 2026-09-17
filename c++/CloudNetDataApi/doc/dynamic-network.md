# CloudNetDataApi 动态插件组网说明

## 配置文件

- 配置文件固定命名为 `IceRPCPush.xml`。
- `LoadConfig` 的配置路径为空时，默认读取进程当前目录下的 `IceRPCPush.xml`。
- 根节点为 `CloudNetDataApi`，内部包含 `IceRPCPush` 和 `CloudNet` 两个一级节点。
- 第一版默认按 6 个插件部署：`MtGatewayService`、`MtQuoteService`、`MtTradeService`、`MtQueryService`、`MtEventService`、`MtDeriveService`。
- 同一份配置可以给多个服务插件共用，例如 `LoadConfig(p_hApi, "IceRPCPush.xml", "MtGatewayService")` 会选择 `id="MtGatewayService"` 的 `Local` 节。
- 底层 IceRPCPush 旧接口需要 `Identity`，CloudNetDataApi 启动服务时会在内部临时 XML 中把 `Identity` 改成当前 `Local.service`，上层插件不需要接触这个细节。

## 6 插件角色

| 服务插件 | RPC 服务名 | 默认端口 | 职责 |
| --- | --- | ---: | --- |
| `MtGatewayService` | `MtGatewayServiceRPC` | 30001 | 合并 HTTP 入口、接口分发和 WebSocket 推送入口，不初始化 MT 连接。 |
| `MtQuoteService` | `MtQuoteServiceRPC` | 30002 | 行情连接服务，接收 tick 并发布到统一事件中心。 |
| `MtTradeService` | `MtTradeServiceRPC` | 30003 | 合并交易事件、下单交易和配置事件相关 MT 连接。 |
| `MtQueryService` | `MtQueryServiceRPC` | 30004 | 合并普通查询、历史订单、成交、K线和 bars 查询。 |
| `MtEventService` | `MtEventServiceRPC` | 30005 | 统一事件中心，内部区分行情 best-effort 事件和可靠事件预留通道。 |
| `MtDeriveService` | `MtDeriveServiceRPC` | 30006 | 合并收益计算和 K线生成，订阅 tick、持仓和账户事件。 |

## 12 到 6 合并映射

| 原服务 | 合并后服务 | 说明 |
| --- | --- | --- |
| `HttpGateway`、`WsGateway` | `MtGatewayService` | 入口统一，HTTP 分发和 WebSocket 推送共享网络连接管理。 |
| `MtQuoteService` | `MtQuoteService` | 行情连接独立保留，避免 tick 接收被其他业务阻塞。 |
| `MtTradeEventService`、`MtTradeCommandService`、`MtConfigService` | `MtTradeService` | 交易事件、交易指令和配置事件先放在同一交易域服务。 |
| `MtQueryService`、`MtHistoryService` | `MtQueryService` | 普通查询和历史查询先共享查询服务，后续按压力再拆。 |
| `IceMarketEventService`、`IceReliableEventService` | `MtEventService` | 第一版统一事件中心，内部按 topic 区分行情和可靠事件。 |
| `ProfitService`、`KlineService` | `MtDeriveService` | 派生计算统一承载收益和 K线生成。 |

## IceRPCPush 节

`IceRPCPush` 是底层通信配置，CloudNetDataApi 只把配置路径传给 IceRPCPush，不向上层暴露底层句柄。

`IceRPCPush/Ice` 里每个 `Property` 表示一条 Ice 属性：

| 配置 | 说明 |
| --- | --- |
| `Ice.Default.Locator` | IceGrid/Locator 地址，用于按服务名发现远端插件；当前默认值为 `MtIceGrid/Locator:default -h 127.0.0.1 -p 18000`。 |
| `Ice.ProgramName` | Ice 运行时程序名，主要用于日志和诊断。 |
| `Ice.MessageSizeMax` | Ice 单条消息上限，单位为 KiB；默认 `65536` 即 64 MiB。 |
| `Identity` | 底层默认对象标识；组网模式会被内部运行时配置改成当前 `Local.service`。 |
| `{Service}.Endpoints` | 某个服务端监听地址，例如 `MtGatewayServiceRPC.Endpoints`。 |
| `{Service}` | 某个服务的代理配置；使用 Locator 时通常直接写服务名。 |
| `{Service}Client.Endpoints` | 注册推送时双向回调客户端适配器地址；Ice 3.8 使用独立前缀，避免被服务端 Adapter 判定为未知属性。 |

`IceRPCPush/ICEPUSH` 是底层推送和日志配置：

| 配置 | 说明 |
| --- | --- |
| `AsyncWaitCompleted` | 异步 RPC/PUT 是否等待完成，`0` 立即返回，`1` 保持旧版等待行为。 |
| `BinaryProtocolVersion` | Binary 传输协议版本，当前固定为 `1`。 |
| `BinaryCompressionThresholdBytes` | 单个载荷达到该字节数后尝试 Snappy；只有压缩后更小时才发送压缩数据。 |
| `BinaryMaxPayloadBytes` | 单次请求或应答的主载荷与扩展载荷原始总长度上限，默认 50 MiB。 |
| `BinaryCallTimeoutMs` | Binary RPC/PUT 的调用超时，默认 15000 毫秒。 |
| `BinaryServerWorkerThreads` | Binary 服务端有界工作队列的消费线程数，默认 4。 |
| `BinaryServerQueueCapacity` | Binary 服务端待处理请求上限，默认 4096；队列满时立即返回明确错误。 |
| `BinaryMaxPendingAsync` | 单客户端 Binary 异步未完成请求上限，默认 10000。 |
| `LogLevel` | IceRPCPush 底层日志级别，可选 `debug/info/warn/error/off`；底层日志固定写入 EXE 同级 `PushLog`。 |
| `DebugLog` | 兼容旧版调试日志开关，`LogLevel` 缺失时生效。 |
| `RPCPushBindport` | 服务端 TCP 快速推送绑定端口，`0` 表示不开启。 |
| `RPCPushBindip` | 服务端 TCP 快速推送绑定地址。 |
| `RPCXPushBindport` | 服务端 UDP 点对点推送绑定端口，`0` 表示不开启。 |
| `RPCXPushBindip` | 服务端 UDP 点对点推送绑定地址。 |
| `RPCXThreadNum` | UDP 推送接收线程数。 |
| `RPCPushmax` | 服务端允许的 TCP 推送最大连接数。 |
| `RPCPushport` | 客户端 TCP 快速推送目标端口，`0` 表示不开启。 |
| `RPCPuship` | 客户端 TCP 快速推送目标 IP。 |
| `RPCXPushport` | 客户端 UDP 点对点推送目标端口，`0` 表示不开启。 |
| `RPCXPuship` | 客户端 UDP 点对点推送目标 IP。 |
| `RPCMPushport` | 组播推送端口，`0` 表示不开启。 |
| `RPCMPuship` | 组播推送地址。 |

## CloudNet 节

`CloudNet` 是业务网络管理层配置，只被 CloudNetDataApi 内部使用。

| 节点 | 属性 | 说明 |
| --- | --- | --- |
| `Log` | `path` | CloudNetDataApi 独立错误日志目录；相对路径按 EXE 目录解析，支持 `${PluginId}`。 |
| `Log` | `name` | CloudNetDataApi 日志文件名前缀，支持 `${PluginId}`，日期和 `.log` 由内部追加。 |
| `Log` | `level` | 日志级别，可选 `debug/info/warn/error/off`；当前非 `off` 时记录错误日志。 |
| `Local` | `id` | 当前服务插件唯一名，必须和 `LoadConfig` 的 `p_szPluginId` 一致。 |
| `Local` | `service` | 当前服务插件启动本地 Ice 服务时使用的 RPC 服务名。 |
| `Local` | `startService` | `1` 启动本地服务，`0` 只作为客户端。 |
| `Local` | `snappy` | 旧 JSON RPC/PUT 是否启用 Snappy；Binary 使用独立阈值和“压缩后更小才采用”的固定规则。 |
| `Runtime` | `reconnectIntervalMs` | 后台重连间隔，单位毫秒，非法值回退到 5000。 |
| `Runtime` | `renewIntervalSec` | `RegisterPush` 续约间隔，单位秒，默认 240，低于旧版 5 分钟超时。 |
| `Runtime` | `threadPool` | 每条 Service 客户端连接的 Ice Communicator 线程池大小；正数同时应用为 `Ice.ThreadPool.Client.Size/SizeMax`，`0` 不覆盖显式 Ice Property 或底层默认值。 |
| `Service` | `owner` | 可选，指定该连接属于哪个插件；为空表示所有插件生效，多个 owner 可用逗号、分号、竖线或空白分隔。 |
| `Service` | `id` | CloudNetDataApi 内部连接名，也是 JSON 和 Binary 调用接口的 `p_szConnName`。 |
| `Service` | `proxy` | Ice 属性名或 Locator 服务名，例如 `MtQueryServiceRPC`。 |
| `Service` | `autoConnect` | `1` 表示 `StartNetwork` 后自动连接并定时重连。 |
| `Subscribe` | `owner` | 可选，指定该订阅属于哪个插件；为空表示兼容旧配置，对所有插件生效。 |
| `Subscribe` | `target` | 要注册推送的远端服务，对应 `Service.id`。 |
| `Subscribe` | `pluginId` | 注册到远端的客户端标识，支持 `${PluginId}` 替换为当前插件 id。 |
| `Subscribe` | `subInfo` | 订阅条件字符串，按业务约定传给远端服务，例如 `market.tick.*`。 |
| `Subscribe` | `enable` | `1` 启用订阅，`0` 暂不启用。 |

## 连接和订阅关系

| 当前插件 | 自动连接 | 订阅 |
| --- | --- | --- |
| `MtGatewayService` | `MtQueryService`、`MtTradeService`、`MtQuoteService`、`MtEventService` | `market.tick.*`、订单/成交/持仓/收益等事件，用于 WebSocket 推送。 |
| `MtQuoteService` | `MtEventService` | 默认不订阅，负责发布 tick。 |
| `MtTradeService` | `MtEventService` | 默认不订阅，负责发布交易、账户和配置事件。 |
| `MtQueryService` | 无 | 默认只提供查询服务。 |
| `MtEventService` | 无 | 默认只提供事件注册、续约和发布转发入口。 |
| `MtDeriveService` | `MtEventService`、`MtQueryService` | `market.tick.*`、持仓和保证金相关事件。 |

## API 流程

1. 调用 `Create` 创建实例。
2. 调用 `LoadConfig` 加载 `IceRPCPush.xml` 和当前插件 id，例如 `MtGatewayService`。
3. 新六插件调用 `StartBinaryNetwork` 启动 Binary 本地服务、自动连接远端服务，并启动重连和续约线程；保留 JSON 兼容的插件才使用 `StartNetwork`。
4. `MtGatewayService` 把 HTTP/WebSocket JSON 转成共享二进制契约，再使用 `CallBinarySync`、`CallBinaryAsync`、`PutBinarySync`、`PutBinaryAsync` 按 `Service.id` 调用远端插件。
5. 行情或事件生产者调用 `Publish` 发布推送，已通过 `RegisterPush` 续约成功的插件会收到后续推送。
6. 退出时调用 `StopNetwork` 和 `Destroy`。

## Binary C API 约束

- `ST_CLOUD_NET_BINARY_CALL` 只包含固定宽度语义的整型字段和只读字节视图，不暴露 Ice、STL 或底层句柄。
- 同步调用返回的 `ST_CLOUD_NET_BINARY_RESULT` 必须由 `FreeBinaryResult` 释放。
- 异步结果只在 `PFN_CLOUD_NET_BINARY_ASYNC` 回调期间有效，跨线程保存时由业务自行深拷贝。
- `StartBinaryService` 和 `StartBinaryNetwork` 的业务回调只填写公共结果，CloudNetDataApi 在内部完成 `BinaryResponseData`。
- Binary 异步提交失败时返回负数且不触发回调；提交成功的请求由底层保证只回调一次。
- 载荷可以包含嵌入零字节；业务结构禁止指针、STL、`HANDLE`、平台 `bool` 和编译器相关整数类型。
- 超过 50 MiB 的查询结果必须使用分页或业务分块，第一阶段不提供自动分片。

## 推荐启动顺序

1. 启动 `MtEventService`，先提供事件注册和发布入口。
2. 启动 `MtQuoteService`、`MtTradeService`、`MtQueryService`。
3. 启动 `MtDeriveService`，向事件中心注册订阅。
4. 启动 `MtGatewayService`，建立对内部服务的连接并注册前端推送需要的订阅。

## 后续拆分条件

- `MtGatewayService`：当 WebSocket 推送压力影响 HTTP 接口响应时，再拆回 HTTP 入口和 WebSocket 入口。
- `MtTradeService`：当下单链路需要独立灰度、串行化或故障隔离时，再拆出交易指令服务。
- `MtQueryService`：当历史查询明显拖慢普通查询时，再拆出历史查询服务。
- `MtEventService`：当可靠事件 ACK、Replay、落库实现后，再拆为行情事件中心和可靠事件中心。
- `MtDeriveService`：当收益计算或 K线生成 CPU、内存压力明显时，再拆为独立派生服务。

## 当前能力边界

- 已具备服务发现、服务连接表、定时重连、推送注册续约、发布推送基础能力。
- 已具备 Binary RPC/PUT 的同步、真正异步、Snappy、自描述长度校验和服务端有界队列能力。
- 可靠事件中心的落库、ACK、Replay、消费者进度和保留期仍是后续业务服务实现内容。
- 现有 `RegisterPush/Publish` 是基础推送通道，不能替代完整 Topic、ACK、Replay、Snapshot、SourceEpoch 和 outbox 事件中心。
- HTTP、WebSocket、MT 连接、收益计算和 K线生成等插件源码工程后续单独规划。
- IceGrid 托管自动拉起需要真实插件 exe 路径后再完善应用模板。
