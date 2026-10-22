# SocketServer VS2022 工程规则

- 同时遵守 `F:\开发资料\MyCode\kds-notes\c++\AGENTS.md`。
- Release 与 ReleaseStatic 共用源码、C ABI、v143、C++17 和 `/MD`；SocketServer 与 HP-Socket 必须整体切换链接模式。
- 监听实例、连接、发送队列、应答线程和停机状态必须有唯一所有者及明确并发契约。
- 不得在库锁内调用用户回调；停机按停止接单、唤醒等待、等待线程、释放连接的顺序执行。
- 每个协议结构字段必须说明单位、最大长度、所有权、有效期和兼容版本。
- 修改后构建 Release/ReleaseStatic，并运行 HTTP 查询、多实例和稳定性测试。
