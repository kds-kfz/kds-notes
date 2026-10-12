# SocketServer Linux 构建

Windows 继续使用 `project/vs2022/SocketServer/SocketServer.sln`。Linux 使用
`make -C project` 构建 `lib/Lnx64/libsocketserver.so`，不使用 CMake。

先准备与公共接口兼容的 Linux HP-Socket 开发头文件及 `libhpsocket.so`，
并准备同级 `nsdk` 的头文件及 `lib/Lnx64/libnsdk.so`。仓库中的
`SocketServer/vender/hpsocket` 是 Windows SDK，不得用它构建 Linux 版本。
如果依赖未安装在默认位置，可显式指定路径：

```sh
make -C project HP_INCLUDE=/opt/hpsocket/include HP_LIB_DIR=/opt/hpsocket/lib \
  NSDK_INCLUDE=../../nsdk/include NSDK_LIB_DIR=../../nsdk/lib/Lnx64
```

运行时需让动态链接器找到 `libhpsocket.so` 和 `libnsdk.so`，例如设置
`LD_LIBRARY_PATH` 或在部署环境中配置动态链接器路径。Linux 仅捕获 C++
异常，不能用 Windows SEH 的方式截获访问违规等信号。

当前仓库未提供 Linux HP-Socket 库，本机未完成 Linux 编译和端到端验证。
在目标 Linux 环境完成构建后，需要运行 TCP、HTTP、WebSocket 回显联调，
再确认部署适用；HTTPS/WSS 接口仍是工程原有的未实现状态。
