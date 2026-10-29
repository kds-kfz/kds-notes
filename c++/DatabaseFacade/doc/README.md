# DatabaseFacade

`DatabaseFacade` 是数据库类型中立的稳定 C ABI。首期 Provider 为 PostgreSQL，私有使用 pqxx 7.10.7 和 libpq 18.4。公共接口不暴露连接对象、STL、pqxx 类型或异常。

业务 SQL 和 Repository 仍由调用服务拥有。池、事务和结果句柄由本模块创建和销毁，调用方不得跨线程并发使用同一事务句柄。
