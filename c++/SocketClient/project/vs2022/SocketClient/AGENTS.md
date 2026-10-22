# SocketClient VS2022 工程规则

- 同时遵守 `F:\开发资料\MyCode\kds-notes\c++\AGENTS.md`。
- Release 与 ReleaseStatic 共用源码、C ABI、v143、C++17 和 `/MD`；SocketClient 与 HP-Socket 必须整体切换链接模式。
- 两种模式均使用 OpenSSL 3.5.6 动态库；TLS 必须校验证书链、有效期和 SAN/CN 主机名，失败关闭连接。
- 一个实例只允许一个在途请求；并发通过独立实例或受控连接池实现，回调不得在内部锁内执行。
- 每个连接状态、超时字段和缓冲区成员必须说明线程、所有权、单位和有效期。
- 修改后构建并运行 Release、ReleaseStatic 运行测试和 TLS 正反例。
