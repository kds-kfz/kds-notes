# demoClient 工程规则

- 同时遵守 `F:\开发资料\MyCode\kds-notes\c++\AGENTS.md`。
- 本工程只演示公开客户端接口，不复制 SocketClient 内部实现，不使用生产地址、账号或密钥。
- 网络失败、超时和断连必须可见且可清理；示例退出前释放全部连接和线程。
- VS2022 Release 使用 v143、C++17、`/MD` 和 Level4；构建后运行最小连接/断连验证。
