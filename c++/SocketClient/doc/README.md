# SocketClient

这是基于 HP-Socket 的独立客户端动态库，其 ABI 与 SocketServer 相互独立。
工厂、共享实例基类、`TcpSockClientObj`、`HttpSockClientObj`、
`WebSockClientObj`、各协议监听器及协议辅助逻辑分别编译；
`src/Base64.h/.cpp` 直接复用 `SocketServer/src` 的既有编码实现。
VS2022 工程引用同级 `SocketServer/vender/hpsocket` 的头文件和 x64vc14 导入库；
部署时需同时提供 `libSocketClient.dll` 与 `HPSocket.dll`。

每个命名工厂实例独占一条连接；名称在同一协议类型内唯一。TCP 交付未分帧的
字节流，业务层自行处理粘包和拆包；HTTP 交付已解析的响应事件；WebSocket
先校验 HTTP 升级响应，再交付完整帧。回调中的数据仅在本次回调期间有效，
业务层应立即复制并投递队列。不得在实例自身回调里调用 `Stop` 或
`Del*ClientInstance`；删除操作须与该实例其他公开调用串行化，并保证回调
上下文在 Stop 返回前仍有效。`Connect` 为异步连接，收到
`EN_SOCKET_CLIENT_CONNECTED` 后才可发送。连接关闭后先调用 `Stop`，
再调用 `Connect` 即可复用实例。重连间隔由业务层控制，库本身不自动重试。

通过 VS2022 `Release|x64` 构建 `project/vs2022/SocketClient/SocketClient.sln`。
DLL 和导入库输出到 `lib/x64vc14`，`tests` 包含多实例本地回环测试。
`demoClient` 和 `demoServer/src/SocketEchoServer.cpp` 提供双方动态库的
TCP/HTTP/WebSocket 真实联调，具体运行方式见 `demoClient/doc/README.md`。

Linux 使用 `make -C project` 构建动态库 `lib/Lnx64/libsocketclient.so`。
Makefile 参照现有 redhat 工程的 Linux 平台目录与对象文件/共享库构建方式；
`HP_INCLUDE`、`HP_LIB_DIR` 可指定 Linux HP-Socket 开发头文件与库目录。
源码已隔离 Windows/Linux 的 HP-Socket 头文件和加密接口，Linux 另需
OpenSSL Crypto 开发包。当前 Windows 联调已验证，但 Linux 依赖、编译与
实机联调尚未验证，不能把 Makefile 的存在当作 Linux 动态库已可交付。

调用方可从不同工作线程创建不同逻辑名称的 TCP/HTTP/Web 客户端；
同一实例的发送操作由实例锁串行化，底层发送按 HP-Socket 异步队列
处理。HTTP 响应按同一连接上的发送顺序交付，不附带业务请求 ID；
压测工具应自行标记请求并关联响应，控制积压量和每连接并发度。
