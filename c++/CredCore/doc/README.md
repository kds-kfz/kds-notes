# CredCore 授权库

## 作用与边界

CredCore 独立承载原 nsdk 的加密、编码和 TradingTerminal Token 生成/校验逻辑，并封装 mthub
Access JWT 所需的 RS256 公钥验签原语。拆分后 nsdk 只保留通用系统能力，Gateway 也不需要直接包含或
链接 OpenSSL。

公开接口包括：

- `Aes`、`Deaes`；
- `Base64Encode`、`Base64Decode`；
- `MakeFeatrue`、`BuildFeatrue`、`AuthFeatrue`；
- `BuildFeatrueByPlainClientInfo`、`BuildFeatrueByPlainClientInfoSeconds`；
- `AuthFeatrueByPlainClientInfo`。
- `GetCompactJwtSha256Fingerprint`、`ReadRs256JwtKeyId`；
- `CreateRs256PublicKeyVerifier`、`VerifyRs256JwtSignature`、
  `DestroyRs256PublicKeyVerifier`。

函数名中的 `Featrue` 是历史协议拼写，为保持已有 Token 和调用方兼容而保留，不能自行改名。

## 工程规则与代码结构

- 项目级规则位于 `project\vs2022\CredCore\AGENTS.md`，由 V3 通用规则裁剪而来；七服务、V2 迁移、
  Gateway 路由和部署门禁不适用于本库。
- `include\CredCore.h`：稳定公开接口、不透明句柄和 `EN_CREDCORE_RS256_RESULT` 返回码。
- `src\CredCore.cpp`：历史 Base64、AES 和 Token 兼容封装层。
- `src\aes.h/.cpp`：历史 AES/Rijndael 内核；成员、参数和固定表遵守 CredCore 命名规则，算法字节行为
  由固定向量锁定。
- `src\JwtRs256Verifier.h/.cpp`：JWK 公钥构造、JWT Header 严格解析、指纹和 RS256 验签私有实现。
- `tests\CredCoreCompatibilityTests.cpp`：历史协议固定向量、RS256 负例和并发验签测试。
- 当前规范整改的边界、风险和验收标准见 `doc\CredCore代码规范与RS256实现整改方案.md`。

## 头文件与 ABI

- `CredCore_define.h` 使用 `CRED_CORE_DEFINE_H` 保护宏，并提供
  `BGN_NAMESPACE_CREDCORE`、`END_NAMESPACE_CREDCORE`、`USE_NAMESPACE_CREDCORE`。
- 公开函数位于 `credcore` 命名空间中，并使用 `extern "C"` 固定 DLL 导出名，避免 MSVC C++
  名称修饰随函数签名变化。
- 接口参数和返回值仍包含 `std::string`，所以它不是 C ABI。调用方必须使用兼容的 MSVC C++
  ABI、`/MD` 运行时和相同的标准库边界。
- `CREDCORE_EXPORTS` 用于构建 DLL，`CREDCORE_STATIC` 用于静态消费；普通 DLL 调用方使用默认
  `dllimport`。

## RS256 验签边界

- RS256 公共头只暴露 `void*` 不透明句柄，不暴露 `EVP_PKEY` 或任何 OpenSSL 类型。公钥句柄创建后
  不可变，可由多个请求线程并发验签；调用方必须保证最后一个请求退出后再销毁句柄。
- `ReadRs256JwtKeyId` 严格解析 Compact JWT Header，只接受 `alg=RS256` 和 1..128 字节的非空
  `kid`。签名成功前，该 `kid` 只能用于选择公钥，不能被视为可信身份。
- `CreateRs256PublicKeyVerifier` 严格解码 JWK Base64URLUInt `n/e`，拒绝前导零、异常指数以及小于
  2048 位或大于 4096 位的 RSA 模数，并用 OpenSSL 3 Provider API 构造公钥。
- `VerifyRs256JwtSignature` 再次检查三段结构、`alg` 和 `kid`，固定使用 SHA-256、RSA PKCS#1 v1.5
  验证原始 signing input。只有签名成功才返回解码后的 Payload 文本。
- CredCore 不校验 `iss/aud/exp/iat/nbf/login/instance_id`，也不签发令牌。这些声明及
  `Version/No/Login` 绑定属于 Gateway 业务策略。
- RS256 返回码使用 `EN_CREDCORE_RS256_RESULT` 独立区间，区分参数、JWT 格式、Base64URL、Header JSON、
  算法、`kid`、JWK、密钥长度、签名、OpenSSL 和依赖版本错误；旧授权函数的返回码不变。
- 库不调用全局 `OPENSSL_cleanup()`，避免影响同一进程中的 libpq、NATS 或其他 OpenSSL 使用方。

## 构建产物

- `Release|x64` 生成 `libCredCore.dll/lib/pdb`，供 Gateway 和 Trade 动态调用。
- `ReleaseStatic|x64` 生成 `libCredCore_static.lib`，仅作为可选构建形态；V3 生产发布不静态链接。
- RS256 私有实现固定使用 OpenSSL 3.5.6。DLL 只导入 `libcrypto-3-x64.dll`，不导入 `libssl`；
  JSON Header 使用随仓库保存的 nlohmann/json 3.12.0 严格解析。
- 静态 CredCore 归档包含 RS256 对 OpenSSL 的未解析引用，只有明确理解最终链接边界的独立调用方才能
  使用；V3 禁止采用该形态，否则密码学实现和 OpenSSL 依赖会进入服务 EXE。
- 本库不依赖 nsdk DLL，避免授权实现重新经由 nsdk 进入服务 EXE。
- 公司信息固定为 `Shenzhen Guangwz Technology Co., Ltd.`，其他产品描述保持原工程约定。

## V3 集成

- Gateway 调用 `credcore::AuthFeatrueByPlainClientInfo` 校验 Token。
- Trade 调用 `credcore::BuildFeatrueByPlainClientInfo` 生成 Token。
- Quote、Query、Event、Derive 和 ClientData 不链接 CredCore。
- V3 七服务统一静态链接 nsdk/snappy；CloudNetDataApi、IceRPCPush、SocketServer、HP-Socket
  继续保持 DLL 边界。

## 验证

- 迁移前后兼容测试覆盖 Base64、AES 固定密文向量、自然日 Token、UTC 秒 Token 和传统 Token，结果一致。
- 独立 `CredCoreCompatibilityTests` 覆盖编码往返、加解密往返、正确客户信息及错误客户信息。
- RS256 测试使用独立生成的 2048 位测试公钥和固定签名向量，覆盖合法签名、Payload 篡改、错误算法、
  `kid` 不匹配、非法 Base64URL、异常 JWK 以及 8 线程共享句柄并发验签。
- 运行测试时把 `lib/x64vc14` 与 `vender/openssl/3.5.6/bin/x64vc14/dynamic` 加入 `PATH`。
- `dumpbin /EXPORTS` 已确认旧 10 个和新增 5 个公开函数均以未修饰名称导出；
  `dumpbin /DEPENDENTS` 已确认 DLL 只新增 `libcrypto-3-x64.dll` 依赖。
