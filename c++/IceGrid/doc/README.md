# IceGrid 3.8.2 本地部署包说明

## 目录用途

- `bin`：Ice 3.8.2 注册中心、节点和管理工具。
- `config`：`registry.cfg`、`node.cfg`、`admin.cfg`。
- `app`：CloudNetDataApi 的 IceGrid 应用模板。
- `db`：新版 IceGrid 运行数据库，不复用旧 Ice 3.5.1 数据。
- `log`：Registry 和 Node 日志。

## 默认注册中心

- 实例名：`MtIceGrid`
- Locator：`MtIceGrid/Locator:default -h 127.0.0.1 -p 18000`
- 管理账号：`admin`
- 管理密码：`mtadmin`

## 默认 6 插件服务名

- `GatewayRPC`
- `MtQuoteServiceRPC`
- `MtTradeServiceRPC`
- `MtQueryServiceRPC`
- `IceEventServiceRPC`
- `DeriveServiceRPC`

## 启动顺序

1. 运行 `start_registry.bat` 启动注册中心。
2. 运行 `start_node.bat` 启动节点。
3. 运行 `admin.bat` 验证可以进入管理控制台。
4. 启动业务插件时，建议先启动 `IceEventService`，再启动 MT 服务、派生服务和 `Gateway`。

## 能力边界

- 当前 v1 主要使用 IceGrid/Locator 做注册和发现，插件进程默认由上层服务自行启动。
- `CloudNetDataApi.icegrid.xml` 中的 exe 路径是后续 IceGrid 托管插件进程的占位符。
- `IceEventService` 第一版统一承载行情事件通道和可靠事件预留通道；ACK、Replay、落库由后续插件源码实现。
