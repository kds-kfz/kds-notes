# NetworkFacade 实施方案

## 目标

- 一个 DLL 唯一拥有 SocketServer、SocketClient 和 HP-Socket，消除同进程重复静态运行时。
- OpenSSL 固定动态链接 3.5.6，禁止将密码实现静态吸入 DLL。
- V3 当前 HTTP、WebSocket、TCP 和 JWKS HTTPS 行为保持不变。

## 迁移策略

1. `libNetworkFacade.dll` 先导出现有网络库的 C 工厂函数，V3 只更换导入库和运行 DLL。
2. 新稳定 C ABI 提供 ABI/实现/依赖版本和能力查询；结构使用大小、ABI、标志和预留字段前缀。
3. 新网络能力只能通过 opaque handle 的领域接口增加，不得继续扩大 C++ 虚表兼容面。
4. 兼容导出删除前必须完成 Gateway 与 IceRPCPush 的 Adapter 迁移和一整个发布周期的回归。

## 验收

- `.def` 是唯一导出白名单，旧网络工厂与新 Facade 查询函数均能解析。
- `dumpbin /DEPENDENTS` 不包含 `libSocketServer.dll`、`libSocketClient.dll` 或 `HPSocket.dll`。
- 依赖中只出现一组 OpenSSL 3.5.6；HTTP、WebSocket、HTTPS、断连和停机测试通过。
- V3 Gateway 只直接链接 `libNetworkFacade.lib`，不再链接三个旧网络导入/静态库。
