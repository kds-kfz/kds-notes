# demoClient 与 SocketEchoServer

`demoServer/src/SocketEchoServer.cpp` 是独立的库级回显服务示例，使用
`SocketServer` 动态库同时监听 TCP、HTTP、WebSocket；不改动原
`demoServer.cpp` 的业务演示逻辑。`demoClient` 通过 `SocketClient` 动态库
并发创建多个命名实例，对三个端口的连接、请求和应答进行校验。

Windows：分别编译 `demoServer/project/vs2022/SocketEchoServer/SocketEchoServer.sln`
和 `demoClient/project/vs2022/demoClient/demoClient.sln` 的 Release|x64。
运行前确保服务进程能找到本次编译的 `libSocketServer.dll`、`HPSocket.dll`、
`libnsdk.dll`、`pthreadVC2.dll`，客户端进程能找到 `libSocketClient.dll`、
`HPSocket.dll`。Windows 会优先从 exe 所在目录加载 DLL；旧 demoServer 的
`bin/x64vc14` 已有旧库，不能只靠修改 PATH 覆盖它。回显服务输出到独立的
`bin/socketecho/x64vc14`，可以把本次所需 DLL 放在该目录，或把 exe 和 DLL
放进单独测试目录运行；不要覆盖旧 demoServer 的库。

先运行 `SocketEchoServer.exe 39071`，再运行
`demoClient.exe 127.0.0.1 39071 8`。服务端分别监听
127.0.0.1:39071/39072/39073；服务端第二个参数可指定自动运行的秒数，
例如 `SocketEchoServer.exe 39071 30`。客户端第三个参数是每种协议的连接数，
缺省为 2，最大为 256。全部响应匹配时返回码为 0，否则为 1。

Linux：先准备与公共头文件 ABI 一致的 `libsocketserver.so`，再使用
`make -C SocketClient/project`、`make -C demoServer/project`、
`make -C demoClient/project`。Makefile 使用 Lnx64 目录约定，可指定
`HP_INCLUDE`、`HP_LIB_DIR`、`SERVER_LIB`、`CLIENT_LIB` 指向目标机器依赖。
Linux HP-Socket 与 OpenSSL Crypto 开发包必须预先安装；现有 SocketServer
Makefile 及 Linux 服务端 `.so` 不在本次验证范围。当前 Windows 环境没有
执行 Linux 实机编译或联调，不能把 Makefile 的存在当作测试通过。
