# DatabaseFacade 实施方案

## 目标

把 pqxx 7.10.7、libpq 18.4、连接池、事务、预编译和结果读取收口到单一数据库边界。V3 Repository 继续拥有业务 SQL、表结构和数据映射，原 `CPostgreSqlPool` 只保留源码兼容 Adapter。

## 接口与实现

- 公共 ABI 对数据库类型保持中立，只公开配置 POD、不透明 pool/transaction/result 句柄和字段复制接口。
- `IDatabaseProviderFactory` 与 PostgreSQL Provider 只在 DLL 内部存在；未实现 Provider 返回 `FACADE_NOT_SUPPORTED`。
- 每条物理连接同一时刻只允许一个执行流使用；并发由连接池多连接提供。
- 创建连接池时立即尝试建立最小连接，但数据库暂时不可达不使服务初始化失败；维护线程按健康检查周期补足最小连接，`IsPoolReady` 在首条连接成功后才返回就绪。
- 并发扩容必须把“已建立连接数 + 正在建立连接数”共同计入最大连接数，避免多个业务线程同时越过 `uiMaxConnections`。
- 归还连接记录空闲起点；维护线程只回收超过 `uiMinConnections` 且达到 `uiIdleShrinkMs` 的空闲连接。`uiIdleShrinkMs=0` 表示关闭空闲收缩。
- 维护线程在 DLL 内部拥有和回收，不在 `DllMain` 中启动；销毁池时先通知并等待线程退出，再释放物理连接。
- 事务必须显式 `Begin/Execute/Commit` 或 `Rollback`，`Destroy` 对未完成事务执行回滚。
- Result 与事务均由创建它们的 DatabaseFacade 释放，不跨边界传递 pqxx 或 STL 类型。

## 构建与验收

- Dynamic DLL 私有静态吸收 pqxx/libpq，仅动态依赖同进程唯一 OpenSSL 3.5.6。
- Static 兼容构建由最终 Facade 静态所有者吸收同一组依赖；不得与业务 EXE 重复链接。
- 覆盖参数化 SQL、NULL、结果字段、事务提交/回滚、超时、连接断开、并发池等待和 ABI 失败。
- 覆盖数据库启动晚于服务、运行期断连后恢复、并发扩容不越界、空闲收缩不低于最小连接数和停机无后台线程残留。
