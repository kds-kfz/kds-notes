# MtLoadTest VS2022 工程规则

- 同时遵守 `..\..\..\AGENTS.md` 和 `F:\开发资料\MyCode\kds-notes\c++\AGENTS.md`。
- MFC 对话框只做界面编排，网络、JSON、配置和压测调度保留在已有分层中。
- Release 与 ReleaseStatic 使用 v143、C++17、`/MD`；自研依赖的链接模式必须整体一致。
- UI 更新必须回到 UI 线程；关闭窗口必须取消请求、唤醒等待、join 线程后再释放资源。
