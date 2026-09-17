# TradingTerminal V3 整体架构流程图

## 1. 当前阶段

第一阶段目标是平替 V2 单体功能。Quote 固定单实例且每个逻辑来源只有一条物理行情连接；Event 使用单节点 JetStream。Quote 集群、同节点行情主备、NATS KV Owner、跨机接管和三副本属于第二阶段，不是当前生效配置。

当前 Quote 已收敛为原始 Tick 接入和 1211 发布，Event 已收敛为 Quote v1 校验、固定分片、本地 Ice 扇出和单副本可靠事件。Derive、Query、Gateway 的完整消费者改造尚未落地，因此整体仍是“可编译、可单测、不可生产平替”。

~~~
graph LR
    U["HTTP和WebSocket用户"] --> G["MtGatewayService"]
    MT["MT4和MT5"] --> Q["MtQuoteService单实例单源接入"]
    Q -->|"1211原始Tick"| E["MtEventService单实例"]
    E --> D["MtDeriveService后续消费"]
    D --> R["MtQueryService查询和历史"]
    G --> T["MtTradeService"]
    G --> R
    E --> G
    I["IceGrid Registry和Node"] -.-> G
    I -.-> Q
    I -.-> T
    I -.-> R
    I -.-> E
    I -.-> D
    N["单节点NATS JetStream"] -.-> E
    P["PostgreSQL"] --> R
~~~

## 2. 子流程

- [六服务逻辑架构](01-六服务逻辑架构.md)
- [查询、交易、行情、收益端到端链路](02-查询交易行情收益端到端链路.md)
- [IceGrid 与基础依赖部署拓扑](03-IceGrid与基础依赖部署拓扑.md)
- [横向扩展、租约与 Fencing](04-横向扩展与租约Fencing.md)
- [5 万用户目标部署](05-五万用户目标部署.md)
- [10 万用户目标部署](06-十万用户目标部署.md)
- [故障自愈与 N+1 切换](07-故障自愈与N加一切换.md)
- [启动、升级、停止与回滚](08-启动升级停止与回滚.md)
