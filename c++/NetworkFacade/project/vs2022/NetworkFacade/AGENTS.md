# AGENTS.md

本工程遵循 `c++/AGENTS.md` 的通用规则，并补充以下 NetworkFacade 约束：修改源码、公共头、工程或测试前必须先读 `doc/README.md` 与 `doc/版本说明.md`，并先在 `doc` 保存方案。版本说明只能在末尾追加。

- 公共稳定 ABI 只能使用固定宽度整数、指针加长度、POD 结构和不透明句柄，禁止新增 STL、异常、虚函数和第三方类型。
- Facade 本身不得继承抽象工厂；Provider 工厂与 HP-Socket、SocketServer、SocketClient 具体类型留在 DLL 内部。
- 迁移期兼容导出只用于保持现有 V3 行为，新增业务不得继续使用兼容 C++ 虚表 ABI。
- 每个函数必须说明输入、输出、失败路径和线程安全；每个结构字段和类成员必须说明用途、单位、范围、所有权及有效期。
- 使用制表符缩进，大括号独立成行，新增注释使用简体中文且不得乱码。
- VS2022 使用 v143、C++17、动态 CRT；动态库导出只能来自 `.def` 白名单。
- SocketServer、SocketClient 和 HP-Socket 必须由本 DLL 唯一静态吸收，OpenSSL 只允许动态链接 3.5.6。
