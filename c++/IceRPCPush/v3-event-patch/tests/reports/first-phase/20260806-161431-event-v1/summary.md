# MtEventService 第一阶段与 Quote v1 本地验证记录

## 1. 验证范围

- Quote Tick Binary 唯一 v1、108 字节固定头及旧开发版本拒绝。
- Event `1211` 三层校验、固定分片、Epoch/Sequence 连续性、队列容量和 `1176` 心跳。
- Event dev/test 第一阶段配置和单副本 Reliable Event。
- 六服务严格构建、协议测试、dev/test 全量与增量打包。

## 2. 构建与测试

| 项目 | 结果 |
| --- | --- |
| `MtEventService Release|x64 /W4 /WX` | 0 警告、0 错误 |
| `MtQuoteService Release|x64 /W4 /WX` | 0 警告、0 错误 |
| 六服务 `TradingTerminal.sln Release|x64 /W4 /WX` | 0 警告、0 错误 |
| `MtGatewayServiceProtocolTests` | `ALL_TESTS_PASSED` |
| `build.bat 1 dev` | 成功 |
| `build.bat 2 dev` | 成功，未变化 XML/DLL 均跳过 |
| `build.bat 1 test` | 成功 |
| `build.bat 2 test` | 成功，未变化 XML/DLL 均跳过 |

四次打包均执行 Clean、Build、配置部署、EXE/PDB 校验、运行库同步和 Tags 更新；所有出口均完成根 `Temp` 清理。

## 3. 配置与产物

- Event dev/test 解析结果均为 `Cluster=0`、`Broadcast=0`、`queueCapacity=0`、`workerThreads=4`、`ReliableEvent=1`、`replicas=1`。
- `bin\x64vc14` 和 Tags 均包含六个 EXE/PDB 及六份无后缀运行 XML；`_dev/_test.xml` 运行副本数量均为 0。
- `Temp` 最终不存在。
- Event test 模板与 Tags `MtEventService.xml` SHA-256：`0746CD873D379AC9BBCEB529ED9D4B14E49AAE5F63124054CEB5505B5CE5258E`。
- Quote test 模板与 Tags `MtQuoteService.xml` SHA-256：`25A1BB393D657F627313134EEC86D06FD40BDF78712EF22F0B123482440A8D56`。
- `bin` XML 由 `configure_instance.vbs` 重新序列化并展开 AdapterId/实例占位符，因此不要求与源模板逐字节相同；解析后的第一阶段开关和值符合预期。

## 4. 静态检查

- `QuoteBinaryProtocol.h` 固定 `QUOTE_BINARY_PROTOCOL_VERSION=1`、`QUOTE_BINARY_HEADER_SIZE=108`。
- Quote、Event 和协议测试只使用 v1 编解码；旧开发态 v3-v6 测试正文被拒绝。
- `1176` 已进入协议枚举、协议名称、Event 工程和协议测试工程。
- Quote 当前路径无 WAL、Snapshot、TIME_QUERY、`1122/1123/1182` 业务实现；Derive/Query 自身保留的时间权威和 OpenPrice 字段不属于 Quote v1 正文。

## 5. 未完成项

验证时本机未运行 `nats-server`、IceGrid Registry/Node、MtQuoteService、MtEventService 或测试消费者，因此未执行真实 MT、单节点 JetStream 和十分钟逐条对账。当前证据只支持“可编译、可单测”，不支持生产平替结论。
