# MtEventService 流程图

## 1. 职责与状态

MtEventService 隔离三类传输职责：`1211` Best Effort 行情中继、`1251/1252` 派生通知中继、`1172-1175` JetStream 可靠事件。它不接 MT SDK、不访问 PostgreSQL、不缓存价格、不计算收益或 K线，也不直接生成 Web JSON。

第一阶段固定单 Event 实例：`1211/1251/1252` 只做本地 Ice 扇出，不进入 NATS；可靠事件使用单节点、`replicas=1` 的 JetStream。跨实例 Core NATS 广播代码只为第二阶段保留，dev/test 均禁用。真实 Quote/Event/消费者十分钟逐条对账仍待验收。

~~~
graph LR
    Q1["Quote 1211"] --> B1["Best Effort分片中继"]
    T1["Trade 1172"] --> J1["JetStream可靠存储"]
    D1["Derive 1251和1252"] --> V1["协议校验"]
    B1 --> E1["当前Event实例Ice订阅者"]
    V1 --> E1
    E1 --> G1["Gateway或Query"]
    J1 --> D2["Derive 1173和1174"]
~~~

## 2. 协议和资源

| 范围 | 用途 |
| --- | --- |
| `1211` | Quote Best Effort Tick 输入和输出 |
| `1172` | 可靠事件幂等追加，使用 `Nats-Msg-Id=EventId` |
| `1173` | durable pull 拉取并返回 AckToken |
| `1174` | 显式 ACK |
| `1175` | Stream 和 Consumer 状态 |
| `1176` | Event 定向心跳：进程代次、队列计数、逐来源水位和 JetStream Ready |
| `1221-1225/1241-1244` | Trade、配置和时间可靠事件 |
| `1251/1252` | Derive 收益与权威 M1 通知中继 |

`MT_DERIVE_M1_ARCHIVE` 由 Derive 直接写、Query 直接消费，不归 MtEventService 所有；`1252` 只承担在线 M1 通知。

## 3. 子流程

- [启动、停止与双链路隔离](01-启动停止与双链路隔离.md)
- [Best Effort 行情接收与扇出](02-BestEffort行情接收与扇出.md)
- [JetStream 可靠事件写入](03-JetStream可靠事件写入.md)
- [Durable 拉取、ACK 与重投](04-Durable拉取ACK与重投.md)
- [DLQ、积压、容量与恢复](05-DLQ积压容量与恢复.md)
- [第二阶段双活广播与实例级推送](06-双活广播与实例级推送.md)
- [NATS 故障与 Quorum 恢复](07-NATS故障与Quorum恢复.md)

## 4. 源码与配置

- 源码：[`services/MtEventService`](../../services/MtEventService)
- 配置：[`MtEventService_dev.xml`](../../conf/MtEventService_dev.xml)、[`MtEventService_test.xml`](../../conf/MtEventService_test.xml)
- 协议：[`ReliableEventBinaryProtocol.h`](../../common/protocol/ReliableEventBinaryProtocol.h)、[`EventHeartbeatBinaryProtocol.h`](../../common/protocol/EventHeartbeatBinaryProtocol.h)
- 启动：[服务启动操作手册](../服务启动操作手册.md)
- 集群：[横向扩展与自愈改造计划](../横向扩展与自愈改造计划.md)
