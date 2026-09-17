# Best Effort 行情接收与扇出

## 1. 范围与状态

`1211` 回调线程只做通知元数据、ClientData 外层和 Quote Tick Binary v1 三层一致性校验及深拷贝；固定分片线程保持同一 `Version+No` 的输入顺序并发布。代码已实现，真实 Tick 峰值和 P99 待验证。

| 项目 | 内容 |
| --- | --- |
| 输入 | Quote `1211` 元数据、ClientData Binary 外层和 Quote Tick Binary v1 正文 |
| 输出 | 重新以 Event 身份发布的 `1211` 通知 |
| 线程/队列 | CloudNet 回调只深拷贝；同一来源固定分片线程按序发布 |
| 所有权 | Relay 拥有不可变 Payload 副本和可配置队列，不保存长期价格状态 |

## 2. 接收和分片

~~~
graph TD
    A["CloudNet收到Quote 1211"] --> B["校验外层通知元数据"]
    B --> C["解码ClientData和Quote Tick v1"]
    C --> D["核对Version No Symbol Sequence Timestamp一致"]
    D --> H["深拷贝不可变Payload"]
    H --> I["按Version加No稳定哈希到固定分片"]
    I --> L["检查Epoch和原始Sequence连续性"]
    L --> M{重复或乱序}
    M -- 是 --> N["拒绝且不入队"]
    M -- 否 --> O["缺口限频告警并推进已接受水位"]
    O --> J{正容量且分片队列满}
    J -- 是 --> K["淘汰最旧Tick并累计容量缺口"]
    J -- 否 --> P["当前Tick入队"]
    K --> P
    P --> Q["工作线程按序取出"]
~~~

`queueCapacity=0` 时 Event 不因容量主动淘汰；正数只允许 `128-10000000`，满载时淘汰分片中最旧 Tick并累计缺口。该策略不提供跨进程恢复，消费者必须根据 Epoch/Sequence 缺口调用各自历史补齐流程。

## 3. 扇出

~~~
graph TD
    A["分片工作线程交付有效Tick"] --> B["保留原ClientData正文"]
    B --> E["以Event身份本地Ice发布1211"]
    E --> H["Derive Query Gateway订阅者"]
    H --> I["Gateway和Query收到行情"]
~~~

## 4. 对应实现

- [`MtMarketEventRelay.cpp`](../../services/MtEventService/source/MtMarketEventRelay.cpp)
- [`MtEventBroadcastBus.cpp`](../../services/MtEventService/source/MtEventBroadcastBus.cpp)（第二阶段保留，第一阶段禁用）
- [`MtEventServiceApp.cpp`](../../services/MtEventService/source/MtEventServiceApp.cpp)
- 配置和设计索引：[MtEventService 流程图](README.md)

## 5. 2026-08-05 通知过滤与有界日志

~~~
graph TD
    A["IceRPCPush发布端按订阅过滤"] --> B["Event入口按配置FuncId白名单复核"]
    B --> C{是否允许且正文可解码}
    C -- 否 --> D["只累计拒绝计数"]
    D --> E["每分钟输出一次脱敏摘要"]
    C -- 是 --> F["进入1211固定分片业务队列"]
    G["日志生产者"] --> H{日志队列条数和64MiB字节上限}
    H -- 有容量 --> I["异步线程每100ms或被唤醒刷新"]
    H -- 满且DEBUG或INFO --> J["丢弃并按级别计数"]
    H -- 满且WARN或ERROR --> K["进入有界保留区"]
    L["1103 v2"] -.-> M["通知接收拒绝 日志条数字节和丢弃数"]
~~~

未知或误投的高频通知不再逐条写 `INFO`，日志故障也不能形成无界业务内存。源码和单测已完成，现场旧包 24.3 万条误投场景的同负载复测尚未执行。
