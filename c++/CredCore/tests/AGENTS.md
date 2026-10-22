# CredCoreCompatibilityTests 规则

- 同时遵守 `..\project\vs2022\CredCore\AGENTS.md` 和 C++ 统一规则。
- 固定向量必须覆盖 JWT 分段、Base64URL、JWK 转公钥、RS256 成功及签名/算法/`kid`/声明失败路径。
- 测试数据不得包含生产私钥、完整生产令牌或客户数据。
- 动态配置验证 DLL ABI；静态配置只验证库自身兼容能力，V3 调用方仍必须动态链接 CredCore。
- 每个测试辅助结构字段必须说明输入来源、预期结果和敏感数据生命周期。
