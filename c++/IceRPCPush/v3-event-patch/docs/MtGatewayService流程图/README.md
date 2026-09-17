# MtGatewayService 流程图

## 1. 职责与状态

MtGatewayService 是六服务统一 HTTP/WebSocket 接入层，只负责认证、V2 兼容校验、JSON/Binary 转换、后端选择、超时治理、连接治理和推送扇出。禁止在网关实现 MT SDK、PostgreSQL、行情缓存、交易执行、查询业务、收益或 K线计算。

当前源码已经实现本文所列候选链路，包括八类 Topic、Quote/Profit/Kline 快照增量屏障、共享正文和固定 Sender 批量扇出；但 28 个 HTTP 黄金差分、真实 Topic 链路、双活和 5 万至 10 万连接尚未完成生产验证，因此状态为 **已实现，待真实联调/生产验证**。

~~~
graph LR
    C1["HTTP客户端"] --> H1["HTTP接入"]
    C2["WebSocket客户端"] --> W1["Web接入"]
    H1 --> A1["认证与兼容校验"]
    A1 --> B1["Binary路由"]
    B1 --> Q1["Quote Trade Query Event Derive"]
    Q1 -. 12xx通知 .-> W1
    W1 --> S1["会话与订阅索引"]
    S1 --> P1["分片推送队列"]
    P1 --> C2
~~~

## 2. 协议入口

| 范围 | 用途 |
| --- | --- |
| `10001` | Web 心跳、Token 主体绑定和应答 |
| `10002` | Quote、Symbol、Profit、Order、Position、Deal、User、Kline 订阅及业务推送 |
| `10003` | 查询当前连接订阅状态 |
| `10004` | 仅服务端发送交易源状态，入站请求拒绝 |
| `1122-1123` | Quote 快照与时间权威内部依赖，不属于对外 HTTP 路由 |
| `1132-1142` | 交易接口路由 |
| `1152-1166` | Query 和 ClientData 对外 HTTP 路由 |
| `1167` | Query 向 Derive 提供的内部权威状态快照，Gateway 不对外路由 |
| `1172-1175` | Reliable Event 内部管理和诊断调用，不属于对外 HTTP 路由 |
| `1182-1186` | Derive 内部能力；Gateway 使用 `1183/1184` 完成收益租约/快照，使用 `1185` 完成 Web Kline 初始快照 |
| `1211-1252` | 内部事件，转换成 Web `10002/10004` 后才能发给客户端 |

Quote v1 只携带原始 Tick，不再提供五个兼容扩展价格。MT4/MT5 的 OpenPrice 等业务字段必须由 Derive/Query 的确认状态在消费端合并；没有确认值时字段保留为 `0`。

## 3. 组件、线程与资源

| 组件 | 线程/并发模型 | 所有权与资源 |
| --- | --- | --- |
| `CMtGatewayHttpService` | SocketServer 回调、固定业务线程、超时线程、CloudNet 回调 | 拥有 HTTP Server、工作队列和在途表 |
| `CMtGatewayRequestAdapter` | 无状态并发转换 | 不访问网络、MT 或数据库 |
| `CMtGatewayServiceMonitor` | 监控线程和异步 `1101/1103` 回调 | 共享生命周期核心状态 |
| `CMtGatewayBackendPool` | 互斥保护轮询游标 | 不创建 CloudNet 连接 |
| `CMtGatewayWebService` | SocketServer 回调、空闲扫描线程 | 拥有 Web Server、会话管理器和推送器 |
| `CMtGatewayWebSessionManager` | 单互斥保护会话与反向索引 | 拥有连接代次、主体和订阅副本 |
| `CMtGatewayPushDispatcher` | 每连接固定 Sender 分片、批量目标任务 | 每分片独占三类队列、行情合并槽、PendingBytes、慢连接统计和共享不可变正文 |

网关不持久化业务数据。主日志、HTTP、Web、CloudNet 和 PushLog 相互隔离。

## 4. 子流程

- [启动、停止与资源管理](01-启动停止与资源管理.md)
- [HTTP 请求处理](02-HTTP请求处理.md)
- [接口校验与 Binary 路由](03-接口校验与Binary路由.md)
- [WebSocket 连接与鉴权](04-WebSocket连接与鉴权.md)
- [订阅、快照与实时推送](05-订阅快照与实时推送.md)
- [后端监控与故障切换](06-后端监控与故障切换.md)
- [双活扩容与故障恢复](07-双活扩容与故障恢复.md)

## 5. 源码与配置

- 源码：[`services/MtGatewayService`](../../services/MtGatewayService)
- 配置：[`conf/MtGatewayService_dev.xml`](../../conf/MtGatewayService_dev.xml)、[`conf/MtGatewayService_test.xml`](../../conf/MtGatewayService_test.xml)
- 协议：[`common/protocol`](../../common/protocol)
- 当前实现：[MtGatewayService 改造方案](../MtGatewayService改造方案.md)
- 第一阶段验收：[第一阶段平替 V2 收尾计划](../第一阶段平替V2收尾计划.md)
