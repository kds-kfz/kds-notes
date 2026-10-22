# MtLoadEngine VS2022 工程规则

- 同时遵守 `..\..\..\AGENTS.md` 和 `F:\开发资料\MyCode\kds-notes\c++\AGENTS.md`。
- `MtLoadEngine`、测试和 Runner 必须共用公开 ABI；测试不得访问仅为绕过接口而暴露的内部符号。
- Release 与 ReleaseStatic 使用 v143、C++17、`/MD` 和 Level4，SocketClient/xlnt 的链接模式必须整体一致。
- 构建两种模式并覆盖并发隔离、暂停/继续、取消、停止和无残留线程。
