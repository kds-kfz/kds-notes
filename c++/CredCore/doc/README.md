# CredCore 授权库

## 作用与边界

CredCore 独立承载原 nsdk 的加密、编码和 TradingTerminal Token 生成/校验逻辑。拆分后 nsdk
只保留通用系统能力，静态链接 nsdk 的服务不会再被动带入授权密钥、授权格式和校验实现。

公开接口包括：

- `Aes`、`Deaes`；
- `Base64Encode`、`Base64Decode`；
- `MakeFeatrue`、`BuildFeatrue`、`AuthFeatrue`；
- `BuildFeatrueByPlainClientInfo`、`BuildFeatrueByPlainClientInfoSeconds`；
- `AuthFeatrueByPlainClientInfo`。

函数名中的 `Featrue` 是历史协议拼写，为保持已有 Token 和调用方兼容而保留，不能自行改名。

## 头文件与 ABI

- `CredCore_define.h` 使用 `CRED_CORE_DEFINE_H` 保护宏，并提供
  `BGN_NAMESPACE_CREDCORE`、`END_NAMESPACE_CREDCORE`、`USE_NAMESPACE_CREDCORE`。
- 公开函数位于 `credcore` 命名空间中，并使用 `extern "C"` 固定 DLL 导出名，避免 MSVC C++
  名称修饰随函数签名变化。
- 接口参数和返回值仍包含 `std::string`，所以它不是 C ABI。调用方必须使用兼容的 MSVC C++
  ABI、`/MD` 运行时和相同的标准库边界。
- `CREDCORE_EXPORTS` 用于构建 DLL，`CREDCORE_STATIC` 用于静态消费；普通 DLL 调用方使用默认
  `dllimport`。

## 构建产物

- `Release|x64` 生成 `libCredCore.dll/lib/pdb`，供 Gateway 和 Trade 动态调用。
- `ReleaseStatic|x64` 生成 `libCredCore_static.lib`，仅作为可选构建形态；V3 生产发布不静态链接。
- 本库不依赖 nsdk DLL，避免授权实现重新经由 nsdk 进入服务 EXE。
- 公司信息固定为 `Shenzhen Guangwz Technology Co., Ltd.`，其他产品描述保持原工程约定。

## V3 集成

- Gateway 调用 `credcore::AuthFeatrueByPlainClientInfo` 校验 Token。
- Trade 调用 `credcore::BuildFeatrueByPlainClientInfo` 生成 Token。
- Quote、Query、Event、Derive 和 ClientData 不链接 CredCore。
- V3 七服务统一静态链接 nsdk/snappy；CloudNetDataApi、IceRPCPush、SocketServer、HP-Socket
  继续保持 DLL 边界。

## 验证

- 迁移前后兼容测试覆盖 Base64、AES、自然日 Token、UTC 秒 Token 和传统 Token，结果一致。
- 独立 `CredCoreCompatibilityTests` 覆盖编码往返、加解密往返、正确客户信息及错误客户信息。
- `dumpbin /EXPORTS` 已确认 10 个公开函数均以未修饰名称导出。
