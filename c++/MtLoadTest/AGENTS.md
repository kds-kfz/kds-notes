# AGENTS.md

本文件约束 `MtLoadTest` MFC 工具的后续开发。工程是界面宿主，不是压测引擎，也不得把业务协议重新堆回对话框类。

## 分层与依赖

- `*Dlg` 只负责控件状态、用户确认、任务编排和窗口消息，不直接解析 XML/JSON，不实现 HTTP/WebSocket 协议，不保存业务状态。
- `interface_test/InterfaceTestModel.*` 只负责配置模型、校验、变量和地址规则。
- `interface_test/JsonMessage.*` 只负责 JSON 构造、字段同步、格式化、脱敏和响应变量提取。
- `interface_test/InterfaceTransport.*` 只负责 HTTP、HTTPS、WebSocket 传输及其资源生命周期，不依赖 MFC 控件。
- `engine/LoadEngineClient.*` 是 `MtLoadEngine` 公共 C ABI 的唯一界面适配边界；动态配置加载 DLL，静态配置直接绑定符号，MFC 代码不得依赖引擎内部实现。
- 依赖方向固定为：MFC 界面 -> 应用模型/传输接口 -> Windows 或第三方基础库。底层模块禁止反向包含对话框头文件。
- TCP、Binary、Protobuf 等未完成功能通过接口和枚举预留，不在现有 HTTP/JSON 代码中加入协议特判。

## 配置与资源

- 接口测试配置固定放在 `MtLoadTest/conf/interface-test.xml`；压力测试配置也归 `MtLoadTest/conf` 管理。
- 禁止把工具配置放到 `MtLoadEngine/conf`。动态库不拥有、复制或修改 MFC 工具配置。
- XML 在工程中使用 `None` 项并归入“配置文件”筛选器，不编译为 Windows 资源；构建后复制到 `bin/x64vc14/conf`。
- 配置文件使用 UTF-8 无 BOM、CRLF。新增节点必须写中文说明并校验非法值、默认值、重复 ID 和交叉引用。
- 保存配置必须使用临时文件、UTF-8/XML 复检和原子替换；失败不得破坏原文件。
- 密码、Token 和其他敏感变量不得写入普通日志或请求确认摘要，运行时提取值不得回写 XML。

## 线程与生命周期

- 网络和长任务必须在后台线程执行，通过自定义窗口消息把深拷贝结果交给 UI 线程。
- 后台线程不得读写 MFC 控件；窗口消息处理函数不得执行阻塞网络请求。
- 关闭窗口时按“停止标记 -> 取消请求/关闭连接 -> 唤醒等待 -> join 线程 -> 释放句柄”的顺序退出。
- WebSocket 默认不自动重连。HTTP 写操作不得自动重试，避免重复副作用。
- 所有 SocketClient 实例、线程、事件和文件句柄必须有唯一所有者；取消与正常完成路径必须避免重复释放。
- HTTP、HTTPS、WebSocket 和 WSS 统一通过 `SocketClient` 适配 HPSocket；工程不得包含、链接或直接调用其他平台 HTTP API。
- `Release|x64` 使用动态自研链，`ReleaseStatic|x64` 使用静态自研链；`MtLoadEngine`、`SocketClient` 和 xlnt 不得在同一产物中混合动态/静态模式。
- 静态自研链继续统一使用动态 CRT（`/MD`）；静态库模式不得擅自改为 `/MT`，避免跨库分配释放不一致。

## 代码与注释

- 新增类、结构、关键函数、成员状态和复杂流程必须添加简体中文注释，解释职责、线程模型、所有权、失败路径和关键约束。
- 注释说明“为什么”，避免逐行复述代码；错误提示和协议报文保持服务端约定的语言与字段名。
- 类和函数保持单一职责。生产函数过长或重复逻辑超过 5 行时，优先提取有名称的辅助函数。
- 修改已有文件时保持原编码、换行和局部风格，不做无关格式化。
- 指针使用前判空，索引访问前检查范围，内置类型和成员必须初始化；禁止不安全 CRT 接口。

## 验证与文档

- 源码、资源、工程文件和配置必须同步更新，不允许只改实现。
- 链接方式改动至少执行相关测试、`Release|x64` 与 `ReleaseStatic|x64` 构建、EXE 导入表检查和 `git diff --check`。
- 网络改动需要 Mock 验证；涉及真实 HTTP/HTTPS 规则时再进行真实 V3 联调，并明确区分 Mock 通过、真实联调通过和生产验收。
- 新增、删除或移动源码时同步更新 `.vcxproj`、`.vcxproj.filters` 和 `docs/README.md`。
- 每批改动追加 `doc/版本说明.md`，记录问题、方案、影响、验证和未完成风险。

