# MtEventService 第一阶段收尾与 Quote 内部协议 v1 方案

## 1. 问题与目标

当前 Event 已能订阅 Quote `1211` 并提供 `1172-1175` JetStream 可靠事件，但 Quote Tick 仍沿用未发布的 v3-v6 多版本报文，行情队列固定有界且满载拒绝当前 Tick，test 仍启用第二阶段 Cluster 和 Core NATS 广播。第一阶段需要把链路收敛为单 Quote、单 Event、单节点 JetStream，并形成可观测、可测试的在线中继。

目标链路固定为：

```text
Quote --1211--> Event --1211--> Derive / Query / Gateway
Derive --1251/1252--> Event --> Query / Gateway
Trade/Query --1172--> Event JetStream --1173/1174--> 消费者
```

## 2. 协议方案

- Quote Tick Binary 从未发布开发版本重置为 v1，只接受 `ProtocolVersion=1`。
- v1 固定编码平台与节点、SourceEpoch、Sequence、Symbol、Bid/Ask/Last、Volume/VolumeExt、Flags、原始服务器秒/毫秒时间和 Quote 接收时间。
- 删除 v3-v6 分支、旧头长、线上 Owner/Fencing、时间权威和扩展价格字段；旧开发报文不迁移。
- Event 校验通知来源、投递模式、动作、通知元数据、ClientData 外层字段和 Quote v1 正文的一致性，正文深拷贝后保持字节不变。
- 新增 `1176 EVENT_HEARTBEAT`，请求和应答使用现有 `EN_PLUGIN_ID`，返回进程代次、队列容量/深度、计数、逐来源 Epoch/Sequence 水位和 JetStream Ready。

## 3. Event 运行方案

- 同一 `Version+No` 固定映射到同一工作线程，按 `SourceEpoch+Sequence` 拒绝重复和乱序。
- Epoch 变化记录流重建；同 Epoch 的 Sequence 跳跃记录缺口。
- `queueCapacity=0` 不因容量主动淘汰；正数只允许 `128-10000000`，队列满时淘汰该分片最旧 Tick并接收当前 Tick。
- 工作线程只调用 Event 本地 Ice Publish，不修改行情正文，不进入 NATS，不缓存价格、不计算 M1、不做时间转换。
- `1251/1252` 只接受 Derive 来源并严格解码，第一阶段直接由当前 Event 实例在线转发。
- `1172-1175` 独立使用单节点 JetStream，Stream 副本固定为 1。Core NATS Broadcast 和 Event Cluster 代码保留，但 dev/test 生效配置均关闭，作为第二阶段注释示例。

## 4. 风险与失败边界

- `queueCapacity=0` 可能在消费者或 Ice 发布长期阻塞时增加内存，必须通过 `1103` 和 `1176` 监控深度、时延和失败数。
- `1211/1251/1252` 不提供跨进程恢复；网络失败、进程崩溃和机器重启由消费者根据 Epoch/Sequence 缺口和历史接口恢复。
- 正容量淘汰会制造明确缺口，不能宣传“每个 Tick 不丢失”。容量零仅表示进程存活期间不因配置上限主动丢弃。
- JetStream Ready 只代表 NATS 和 Stream 可用，不代表所有消费者无积压。

## 5. 验收标准

- Quote v1 编解码、非法版本、截断、UTF-8、价格、时间和元数据不一致测试通过，旧 v3-v6 被拒绝。
- Event 有效中继、错误来源/模式/动作、重复、乱序、Epoch、缺口、固定分片、容量零、淘汰最旧和心跳测试通过。
- dev/test 均禁用 Cluster/Broadcast，ReliableEvent 启用且 `replicas=1`。
- Quote、Event、协议测试和六服务 `Release|x64 /W4 /WX` 为零警告零错误，协议测试输出 `ALL_TESTS_PASSED`。
- 真实 NATS、Quote、Event 和测试消费者连续 10 分钟联调完成前，状态保持“源码完成、单测通过、真实联调未通过、生产验收未通过”。
