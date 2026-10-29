# FoundationFacade 实施方案

## 目标

建立可由领域 Facade 私有吸收的无状态基础组件，首期只接管 V3 使用的日期、休眠、文件、目录和路径分隔符能力。组件不整体依赖 nsdk、Breakpad、日志或线程运行时。

## 接口与实现

- 公共接口使用 UTF-8 指针加长度、固定宽度整数和不透明句柄，不暴露 STL 或 nsdk 类型。
- 组件内部由 `IPlatformProviderFactory` 创建 Windows 或 Linux Provider，入口本身不继承工厂。
- V3 生产只使用 ReleaseStatic 生成的 `FoundationStatic.lib`；Release DLL 仅保留兼容构建，不进入生产包。
- 文件、目录、日期和休眠均使用 C++17 标准库及必要的平台 API，不链接完整 nsdk 或 Breakpad。
- 失败关闭，不在 `DllMain` 执行线程、锁或文件操作。

## 验收

- ReleaseStatic 可独立构建，依赖表中不得出现 nsdk、Breakpad、日志或线程库。
- 覆盖 ABI 不匹配、空参数、文件/目录存在与不存在、递归建目录、日期格式和休眠。
- 业务工程不直接链接本组件，由 Network/CloudNet 等最终所有者私有吸收。
