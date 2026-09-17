# MtQuoteService 流程图

## 1. 第一阶段职责

MtQuoteService 只负责 MT4/MT5 原始行情接入：单实例、每个 Version+No 单物理连接、原连接重连、最小校验、深拷贝、进程内 FIFO、SourceEpoch/Sequence 和 1211 发布。

Quote 不负责 WAL、1122/1123/1182、服务器时间偏移、UTC 转换、快照、TickStat 业务扩展、OpenPrice、M1 或归档。旧消费者尚未全部迁移，当前状态为可编译、可单测、不可生产平替。

~~~
graph LR
    M["MT4或MT5"] --> P["唯一QUOTE_PUMPING连接"]
    P --> C["最小校验和深拷贝"]
    C --> Q["Version加No独立FIFO"]
    Q --> S["SourceEpoch和Sequence"]
    S --> B["Quote Tick Binary v1"]
    B --> E["1211 Best Effort"]
    H["1124定向心跳"] --> D["连接 队列 水位和失败诊断"]
~~~

## 2. 第一阶段配置

缺少生效 Cluster 时按单实例运行，不连接 NATS KV。所有 Source 固定 sourceFailoverEnable="0"、queueCapacity="0" 和一条 priority="1" 的 QUOTE_PUMPING/quote 连接。集群和主备示例只存在于 XML 注释，属于第二阶段。

## 3. 子流程

- [启动、停止与来源隔离](01-启动停止与节点隔离.md)
- [第一阶段单实例与第二阶段预留](02-Owner租约与主备切换.md)
- [MT4/MT5 行情接入](03-MT4MT5行情接入.md)
- [最小校验、Epoch、Sequence 与排队](04-Tick校验修正与排序.md)
- [重启、重连与消费者补数契约](05-快照WAL与冷启动恢复.md)
- [1211 发布与容量策略](06-BestEffort与无损行情发布.md)
- [1124 定向心跳](07-行情快照RPC.md)
- [Quote 与时间/派生业务边界](08-时间权威与冬夏令同步.md)

## 4. 源码

- services\MtQuoteService\include\MtQuoteConfig.h
- services\MtQuoteService\include\MtQuoteIngressQueue.h
- services\MtQuoteService\include\MtQuoteSource.h
- services\MtQuoteService\include\MtQuoteManager.h
- services\MtQuoteService\include\MtQuoteServiceApp.h
- services\MtQuoteService\source\MtQuoteConfig.cpp
- services\MtQuoteService\source\MtQuoteIngressQueue.cpp
- services\MtQuoteService\source\MtQuoteSource.cpp
- services\MtQuoteService\source\MtQuoteManager.cpp
- services\MtQuoteService\source\MtQuoteServiceApp.cpp
- common\protocol\QuoteBinaryProtocol.*
- common\protocol\QuoteHeartbeatBinaryProtocol.*
