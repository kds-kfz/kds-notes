# MtEventService 工程规则

- 同时遵守 `F:\开发资料\MyCode\kds-notes\c++\AGENTS.md` 和 `IceRPCPush\project\vs2022\AGENTS.md`。
- 事件接收、队列、分发和回调必须分层；不得持锁调用 Ice、NATS 或宿主回调。
- 每个事件数据结构字段必须说明协议来源、单位、可空性、所有权和版本兼容要求。
- VS2022 Release 使用 v143、C++17、`/MD` 和 Level4，并覆盖重复、乱序、断连和停机路径。
