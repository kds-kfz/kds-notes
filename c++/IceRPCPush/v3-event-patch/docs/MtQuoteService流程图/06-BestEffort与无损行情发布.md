# 1211 发布与容量策略

本文件名为历史固定入口。第一阶段只有 1211 Best Effort，不存在 Quote 无损 WAL 主链。

~~~
graph LR
    A["FIFO头Tick"] --> B["编码Quote Binary v1"]
    B --> C["封装ClientData BYTES"]
    C --> D["Publish 1211 UPDATED"]
    D -- 成功 --> E["published加1"]
    D -- 失败 --> F["publishFailed加1并限频报错"]
~~~

queueCapacity=0 只表示运行期间不因容量主动淘汰，并不表示跨进程恢复。正数容量满时淘汰最旧 Tick；1211 不提供 ACK、Replay 或历史补发。
