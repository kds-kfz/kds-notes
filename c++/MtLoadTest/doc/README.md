# MtLoadTest 文档导航

`MtLoadTest` 是统一的 MFC 测试与数据工具。应用主窗口提供“压力测试、接口测试、数据查看”三个入口；压力测试通过同一 C ABI 调用动态或静态 `MtLoadEngine`，接口测试使用独立的 `conf/interface-test.xml` 和 SocketClient/HPSocket，数据查看仍为预留入口。

## 职责边界

- 工程根目录 `AGENTS.md` 定义本工具的长期分层、配置、线程、注释和验证规则；它由 V3 规范提炼而来，已删除七服务、IceGrid、V2 兼容和插件等无关约束。
- `src` 保存 MFC 主程序、各功能对话框、日志组件、预编译头和资源脚本；`common`、`engine`、`tests`、`res` 继续按职责独立维护。
- 主窗口只负责功能导航，不加载 DLL、不发请求、不解析数据文件。
- 压力测试窗口负责 XML 计划选择与重新加载、HTTP/HTTPS 运行环境、工作负载和用户选择、运行控制、请求/事务指标表格、阈值结果和报告格式选择；全部参数只在 XML 中维护，用户下拉仅显示无密码摘要。
- 接口测试已实现 HTTP+JSON、HTTPS+JSON、WebSocket/WSS+JSON；TCP、Binary、Protobuf 保留配置、枚举、界面入口和适配器接口，并明确显示“待开发”。
- 接口测试配置 `interface-test.xml` 和压力测试配置 `v3-http.xml` 都只属于 `MtLoadTest/conf`，不得放入或复用 `MtLoadEngine/conf`。HTTP/HTTPS/Web 接口测试请求不通过压测 DLL 转发。
- 数据查看当前状态为 `RESERVED`，入口和窗口生命周期已固定。
- 通过 `MtLoadEngine` V4 公共 C ABI 获取状态、请求指标、事务指标和阈值的不可变快照。
- 本地日志只记录操作状态、DLL 脱敏错误样本和终态聚合结果，不记录密码、完整 Token 或报文正文。
- 压力测试窗口不直接调用引擎内部的 jsoncpp、TinyXML 或 xlnt；接口测试模块独立使用 SocketClient/HPSocket、TinyXML 和 nlohmann/json。
- 接口测试写操作不自动重试；HTTPS/WSS 通过 SocketClient/HPSocket 加载配置指定的 PEM CA 并校验证书链，证书路径支持中文；官方 HP-Socket 6.0.9 不校验连接主机名是否匹配证书，不能作为严格站点身份认证；客户端不部署服务器私钥。
- 接口测试通过 `Server.maxResponseBytes` 限制解压后的单响应正文，默认 8 MiB；自动化回归覆盖真实配置读取/原子保存、重复 HTTP 请求、响应超限、取消和句柄释放。
- 当前与压测库同进程运行，不实现 Worker 或命名管道。
- 工程提供 x64 `Release` 动态版和 `ReleaseStatic` 静态版；通过 `vender/MtLoadEngine` 使用公共头及对应模式的库，不引用压测库内部源码。
- `interface-test.xml` 与 `v3-http.xml` 在 Visual Studio 中属于“配置文件”筛选器，项目项类型为 `None`；它们不是 `.rc` 资源，也不会嵌入 EXE。

## 文档索引

- [MtLoadTest 界面设计方案](MtLoadTest界面设计方案.md)
- [源码结构与模块说明](源码结构与模块说明.md)
- [界面接口与数据定义](界面接口与数据定义.md)
- [实现步骤](实现步骤.md)
- [测试与验收说明](测试与验收说明.md)
- [静态链接链路改造方案](静态链接链路改造方案.md)
- [指标绘图压测场景实施方案](指标绘图压测场景实施方案.md)
- [版本说明](../doc/版本说明.md)

源码落地后，文档中的控件、消息、定时器、类、函数和文件路径必须与实际工程同步更新。

解决方案路径：`project/vs2022/MtLoadTest/MtLoadTest.sln`。动态版输出到 `bin/x64vc14/share`，静态版输出到 `bin/x64vc14/static`。
