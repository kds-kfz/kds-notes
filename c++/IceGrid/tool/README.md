# IceGrid 3.8.2 运维脚本说明

## 目录用途

- `bin`：Ice 3.8.2 注册中心、节点、管理工具和 Windows 服务安装器。
- `config`：`registry.cfg`、`node.cfg`、`admin.cfg`；`config\service` 由安装脚本生成。
- `app`：`MtApplication.xml`，CloudNetDataApi 的 IceGrid 应用模板。
- `db`：新版 IceGrid 运行数据库，不复用旧 Ice 3.5.1 数据。
- `log`：Registry 和 Node 日志。
- `tool`：所有安装、启动、部署、管理和卸载批处理，以及本说明文件；根目录不再保留重复脚本。

## 脚本入口

| 脚本 | 用途 |
| --- | --- |
| `tool\install_icegrid_wtgrid.bat` | 首次安装并启动本机 Registry 和默认 Node1。 |
| `tool\start_registry.bat` | 启动已安装 Registry 并执行健康检查。 |
| `tool\start_node.bat` | 启动已安装默认 Node1 并确认注册。 |
| `tool\deploy_app.bat` | 自动判断并执行 application add/update。 |
| `tool\admin.bat` | 进入交互式管理控制台。 |
| `tool\uninstall_icegrid_wtgrid.bat` | 按 Node→Registry 卸载默认服务，保留数据和日志。 |

所有批处理都使用中文窗口标题和中文行内说明。详细使用场景、客户可修改项、多节点配置及故障解决方法见部署根目录的 `部署步骤.md`。

## 查看脚本帮助

每个脚本都支持 `--help`、`/?` 和 `-h`，帮助模式只显示用途、参数、前置条件和示例，不执行安装、启动、部署或卸载操作：

```bat
cd /d <IceGrid部署根目录>\tool
install_icegrid_wtgrid.bat --help
start_registry.bat /?
start_node.bat -h
deploy_app.bat --help
admin.bat /?
uninstall_icegrid_wtgrid.bat --help
```

所有脚本均可使用 `--nopause` 关闭失败暂停，便于自动化调用。例如：

```bat
start_registry.bat --nopause
start_node.bat --nopause
deploy_app.bat --nopause
```

未识别参数会打印正确用法并返回非零错误码。

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

## 服务安装和启动

1. 首次部署时，以管理员身份运行 `tool\install_icegrid_wtgrid.bat`。
2. 脚本安装并启动 `icegridregistry.MtIceGrid` 和 `icegridnode.MtIceGrid.CloudNetNode1`，两个服务均设置为自动启动。
3. 安装脚本按实际部署目录生成绝对路径配置；数据写入 `db`，标准输出和错误日志写入 `log`。
4. 需要手工恢复已安装服务时，分别运行 `tool\start_registry.bat`、`tool\start_node.bat`。
5. 运行 `tool\deploy_app.bat`；首次自动执行 `application add`，已有 `CloudNetDataApiGrid` 时自动执行 `application update`。
6. 运行 `tool\admin.bat`，执行 `node list` 和 `application list`，确认能看到 `CloudNetNode1`、`CloudNetDataApiGrid`。
7. 启动业务插件时，建议先启动 `IceEventService`，再启动 MT 服务、派生服务和 `Gateway`。

以管理员身份运行 `tool\uninstall_icegrid_wtgrid.bat` 可以按 Node、Registry 顺序卸载服务；`db` 和 `log` 数据会保留。

## 同一节点重新部署

1. 先停止业务程序，再按 Node、Registry 顺序停止 Windows 服务。
2. 确认服务完全停止后，备份并清空 `db\node1` 的内容，但保留目录本身。
3. 保留 `db\registry`；常规更新不得清空 Registry 的 LMDB。
4. 覆盖新版程序和 `app\MtApplication.xml`。
5. 依次运行 `tool\start_registry.bat`、`tool\start_node.bat`、`tool\deploy_app.bat`、`tool\admin.bat`。

## 多节点和负载均衡

- 远程 Node 使用 Registry 的业务网 IP，并发布 Registry 可回连的 Node 业务网 IP。
- 远程 Node 单独安装，使用 `--DependOnRegistry 0`；不要在远程机运行会同时安装 Registry 的默认总脚本。
- 多个业务实例必须使用唯一 Adapter ID、相同 Replica Group ID 和 Locator 间接代理。
- 同机多实例必须使用不同 RPC、回调、推送和管理端口，并使用独立日志/缓存目录。
- `round-robin` 在代理解析和建连时选择副本；长连接不会保证每个请求严格轮流。

## 能力边界

- 当前 v1 主要使用 IceGrid/Locator 做注册和发现，插件进程默认由上层服务自行启动。
- `MtApplication.xml` 中的 exe 路径是后续 IceGrid 托管插件进程的占位符。
- `IceEventService` 第一版统一承载行情事件通道和可靠事件预留通道；ACK、Replay、落库由后续插件源码实现。
