# SocketClient 与 SocketServer 联调 Demo

## 用途

`demoServer/src/SocketEchoServer.cpp` 是网络库专用回显服务，使用当前
`libSocketServer.dll` 同时提供 TCP、HTTP 和 WebSocket 服务。
`demoClient` 使用当前 `libSocketClient.dll` 与三个服务并发联调。

原有 `demoServer/src/demoServer.cpp` 是旧业务示例，本次不修改，也不作为网络库
验收入口。

## Windows 编译

使用 Visual Studio 2022 编译以下 Release|x64 工程：

1. `SocketServer/project/vs2022/SocketServer/SocketServer.sln`
2. `SocketClient/project/vs2022/SocketClient/SocketClient.sln`
3. `demoServer/project/vs2022/SocketEchoServer/SocketEchoServer.sln`
4. `demoClient/project/vs2022/demoClient/demoClient.sln`

两个 Demo 工程会在编译后自动复制当前依赖，不需要手工搬运 DLL：

- 服务端：`libSocketServer.dll`、`HPSocket.dll`、`pthreadVC2.dll`
- 客户端：`libSocketClient.dll`、`HPSocket.dll`

`nsdk` 和 `snappy` 统一静态链接，Demo 运行目录不得出现或加载
`libnsdk.dll`、`libsnappy.dll`。

服务端输出目录为 `demoServer/bin/socketecho/x64vc14`，客户端输出目录为
`demoClient/bin/x64vc14`。专用输出目录不会覆盖旧业务 Demo 的依赖。

## 手工联调

先启动服务端：

```powershell
demoServer\bin\socketecho\x64vc14\SocketEchoServer.exe 39071
```

服务端监听：

- TCP：`127.0.0.1:39071`
- HTTP：`127.0.0.1:39072`
- WebSocket：`127.0.0.1:39073`

再运行客户端：

```powershell
demoClient\bin\x64vc14\demoClient.exe 127.0.0.1 39071 8 5
```

参数依次为主机、起始端口、每种协议的并发客户端数、完整测试轮数。服务端还可用
第二个参数指定自动退出秒数，例如 `SocketEchoServer.exe 39071 30`。

## 自动联调

在 `c++` 目录执行：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File `
  .\demoClient\tests\verify_socket_demos.ps1
```

脚本依次执行：

1. 编译当前 SocketServer、SocketClient 和两个 Demo。
2. 用 SHA-256 确认 Demo 输出目录没有混入旧 DLL。
3. 启动 SocketEchoServer 并等待三个协议全部 READY。
4. 每轮并发验证 TCP 回显、HTTP 同连接两次请求和 WebSocket 回显。
5. 多轮重复创建、停止和销毁相同名称的实例，检查客户端句柄没有持续增长。
6. 连续执行两批相同负载，比较服务端句柄数和私有内存，检查资源稳定性。
7. 通过标准输入优雅停止服务端，并检查退出码。
8. 检查 PE 导入表，确认结果不依赖 `libnsdk.dll`、`libsnappy.dll`。

成功标志为：

```text
Socket demo integration: PASS
```

测试使用明文 TCP/HTTP/WebSocket。TLS 终止仍属于代理或上层服务边界，不在本
Demo 的验证范围。Linux Makefile 只保留构建入口，本次 Windows HP-Socket 6.0.9
适配不等同于 Linux 实机验证。
