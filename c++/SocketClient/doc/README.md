# SocketClient

这是基于 HP-Socket 的独立客户端库，其 ABI 与 SocketServer 相互独立，同时支持动态库和静态库构建。
工厂、共享实例基类、`TcpSockClientObj`、`HttpSockClientObj`、
`WebSockClientObj`、各协议监听器及协议辅助逻辑分别编译；
`src/Base64.h/.cpp` 直接复用 `SocketServer/src` 的既有编码实现。
VS2022 工程只引用自身 `SocketClient/vender/hpsocket` 的头文件和库。`Release|x64`
链接动态导入库，部署时同时提供 `libSocketClient.dll` 与 `HPSocket.dll`；
`ReleaseStatic|x64` 定义 `HPSOCKET_STATIC_LIB`，链接 `lib/x64vc14/static/HPSocket.lib`，
最终程序不再部署 `HPSocket.dll`。动态、静态产物分别保存在独立目录，两种模式同时保留。

每个命名工厂实例独占一条连接；名称在同一协议类型内唯一。TCP 交付未分帧的
字节流，业务层自行处理粘包和拆包；HTTP 交付已解析的响应事件；WebSocket
先校验 HTTP 升级响应，再交付完整帧。回调中的数据仅在本次回调期间有效，
业务层应立即复制并投递队列。不得在实例自身回调里调用 `Stop` 或
`Del*ClientInstance`；删除操作须与该实例其他公开调用串行化，并保证回调
上下文在 Stop 返回前仍有效。
HTTP 请求可以通过普通请求头配置 `Accept-Encoding: gzip`、`zlib`/`deflate`，
也可以不发送该请求头。库以响应的 `Content-Encoding` 为准自动解压 gzip 和
zlib/deflate；`HTTP_BODY` 交付解压后的业务正文，`HTTP_COMPLETE.ullWireBodyBytes`
保留解压前的线上正文大小。未知编码或损坏压缩流通过 `HTTP_ERROR` 明确失败，
不会把压缩字节误交给 JSON 解析器。动态配置把 zlib 源码随 SocketClient 使用相同
`/MD` 选项编译；静态配置复用 HP-Socket 静态归档内的 zlib，不再重复合并同名对象。
两种调用链都不增加 zlib DLL。
同一 HTTP/HTTPS 实例同一时刻只允许一个在途请求；前一个响应完成、失败或连接关闭后才允许
发送下一次请求。公开事件没有请求 ID，因此库明确拒绝流水线重叠请求，避免响应归属错误。

`Connect` 为异步连接。明文连接在 TCP 建连后发送 `EN_SOCKET_CLIENT_CONNECTED`；HTTPS/WSS
仅在 TLS 握手和证书链校验成功后发送该事件。安全客户端必须在 `Connect` 前调用
`ConfigureSocketClientTls`：启用校验时传入 PEM 格式 CA 文件，关闭校验仅用于明确的诊断场景。
为支持中文配置路径，库先以 `std::filesystem` 读取证书，再通过 HP-Socket 内存接口加载；站点私钥
不得传入客户端。官方 HP-Socket 6.0.9 预编译库会设置 SNI 并验证 CA 链，但不校验连接主机名是否
匹配证书，因此该能力不能描述为严格站点身份认证。需要严格主机名认证时，应在受控代理终止 TLS，
或使用补齐主机名校验后自行构建并完成安全回归的 HP-Socket。连接关闭后先调用 `Stop`，
再调用 `Connect` 即可复用实例。重连间隔由业务层控制，库本身不自动重试。

通过 VS2022 `Release|x64` 或 `ReleaseStatic|x64` 构建 `project/vs2022/SocketClient/SocketClient.sln`。
动态产物输出到 `lib/x64vc14/share`，SocketClient 静态库输出到 `lib/x64vc14/static`；
HP-Socket 动态文件位于 `vender/hpsocket/lib/x64vc14`，使用 `/MD` 编译的静态库位于其
`static` 子目录。`tests` 包含多实例本地回环测试。
静态调用方必须定义 `SOCKETCLIENT_STATIC` 和 `HPSOCKET_STATIC_LIB`，链接
`libSocketClient_static.lib`、静态 `HPSocket.lib`、`bcrypt.lib`、`crypt32.lib` 和 `ws2_32.lib`。
静态调用方不得再复制 `HPSocket.dll`；动态调用方仍必须保留该 DLL。
动态与静态消费程序均已在 Windows x64 下通过真实 TCP、HTTP、WebSocket 回环运行测试；
当前 `libSocketClient.dll` 为 93,696 字节，静态归档约 8.6 MiB。发布清单应在最终构建后生成
SHA-256，不在源码说明中固化每次重编都会变化的调试签名哈希。
回归测试还覆盖 Keep-Alive 顺序请求、重叠请求拒绝、TLS 工厂/配置以及 32 轮创建、连接、收发、
销毁后的句柄回落。
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
压测工具应为每个并发请求使用独立实例，或在同一实例上严格串行发送。
