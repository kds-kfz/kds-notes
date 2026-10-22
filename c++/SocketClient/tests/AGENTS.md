# SocketClientRuntime 测试规则

- 同时遵守 `..\project\vs2022\SocketClient\AGENTS.md`。
- 动态与静态测试必须使用同一行为断言，分别验证 TCP、HTTP、WebSocket、断连、停机和多实例。
- TLS 至少覆盖正确域名、错误域名、错误 IP、错误 CA、过期证书和未配置 CA。
- 本地证书和服务只能使用测试数据；测试失败必须返回非零退出码并输出具体阶段。
