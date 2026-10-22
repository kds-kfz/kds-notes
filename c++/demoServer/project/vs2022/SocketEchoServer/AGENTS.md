# SocketEchoServer 工程规则

- 同时遵守 `F:\开发资料\MyCode\kds-notes\c++\AGENTS.md`。
- Echo 服务只验证 SocketServer 的 TCP 生命周期和数据回送，不承载业务认证或持久化逻辑。
- 输入长度、连接状态和发送结果必须校验；停机不得遗留监听端口或工作线程。
- VS2022 Release 使用 v143、C++17、`/MD` 和 Level4。
