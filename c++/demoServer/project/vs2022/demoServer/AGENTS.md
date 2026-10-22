# demoServer 工程规则

- 同时遵守 `F:\开发资料\MyCode\kds-notes\c++\AGENTS.md`。
- 示例应用只调用 SocketServer 公共接口，不复制库内部协议、线程或资源管理实现。
- 所有监听端口、线程和连接必须在退出路径关闭；示例配置不得包含生产凭据。
- VS2022 Release 使用 v143、C++17、`/MD` 和 Level4，并验证启动、请求、断连和停机。
