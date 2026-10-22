# MtGatewayServiceProtocolTests 工程规则

- 同时遵守 `F:\开发资料\MyCode\kds-notes\c++\AGENTS.md`。
- 测试固定协议边界、错误码、空结果、超长输入、分包和断连，不连接生产服务。
- 固定报文需要注释字段来源和预期结果；新增协议字段必须增加新旧兼容用例。
- VS2022 Release 使用 v143、C++17、`/MD` 和 Level4。
