# AGENTS.md

本工程遵循 `c++/AGENTS.md` 的通用规则，并补充以下数据库 Facade 约束：修改前先读 `doc/README.md` 和 `doc/版本说明.md`，在 `doc` 保存方案；版本说明只在末尾追加。

- 公共 ABI 禁止暴露 STL、pqxx、libpq、异常、虚函数或回调闭包，只允许 POD 和不透明句柄。
- Facade 本身不继承工厂；数据库 Provider 工厂和 PostgreSQL 实现只在 DLL 内部。
- 每个连接同一时刻只能由一个执行流使用；事务句柄不得跨线程并发调用。
- SQL 必须支持参数化执行，密码不得写日志；失败不得自动改用其他数据库或拼接 SQL。
- 所有结构字段、句柄所有权、超时单位、范围、线程要求和释放顺序必须有简体中文说明。
- DLL/静态库使用 C++17、动态 CRT；`.def` 是 DLL 唯一导出白名单。
- pqxx、libpq 只允许本 Facade 成为最终所有者，OpenSSL 必须保持动态且进程唯一。
