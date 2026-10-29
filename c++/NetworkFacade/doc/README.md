# NetworkFacade

`NetworkFacade` 是 V3 唯一网络运行时边界，私有静态吸收 SocketServer、SocketClient 和 HP-Socket，并动态使用 OpenSSL 3.5.6。业务进程只部署 `libNetworkFacade.dll`，不再部署三个旧网络 DLL。

SocketServer 已移除对 nsdk 和 Breakpad 的隐式依赖；NetworkFacade 不吸收崩溃处理实现。Breakpad、Dump 路径和进程级异常处理继续由最终 EXE 唯一负责，避免同一进程出现多套异常处理状态。

首阶段保留旧 SocketServer/SocketClient C 工厂导出作为源码兼容入口，同时新增 `NetworkFacade_GetAbiInfo` 和 `NetworkFacade_GetCapabilities` 稳定 C ABI。兼容入口不允许承载新业务；后续按业务语义迁移到 opaque handle 后再删除旧 C++ 虚表 ABI。

## 2026-10-23 pthreads-w32 依赖收敛计划

- SocketServer 的进程内工作线程、条件变量和日志互斥改用 C++17 标准线程原语，NetworkFacade 删除
  `pthreadVC2.lib`。这些位置不依赖进程共享、robust、递归或超时互斥语义，可以等价替换。
- Linux SocketServer 继续以同一套源码构建，并保留编译、链接参数 `-pthread`；标准线程库在 Linux
  下仍使用 POSIX 线程实现，因此本次不是取消 Linux pthread 支持。
- nsdk 的命名/进程共享同步保持原生平台实现，本次不修改。
- 完成后以 Dynamic/Static 构建、SocketServer 并发回归及 NetworkFacade PE 依赖表为门禁；当前
  版本没有 pthreads-w32 使用者，因此不导入也不部署 `pthreadVC2.dll`。该库不是永久禁用依赖，
  今后出现标准库无法覆盖的明确 POSIX 语义时，应重新指定唯一所有者后按需引入。
