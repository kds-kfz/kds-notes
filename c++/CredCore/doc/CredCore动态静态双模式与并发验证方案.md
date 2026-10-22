# CredCore 动态静态双模式与并发验证方案

> 日期：2026-10-21  
> 状态：`IMPLEMENTING`

## 目标

CredCore 保留 `Release|x64` DLL 和 `ReleaseStatic|x64` 静态归档。本次 V3 使用 DLL，但静态库必须是
可实际消费、可并发验证的正式产物。这里必须区分两层策略：CredCore 工程不禁止静态构建和静态消费能力；
V3 七服务出于算法实现隔离、查毒误报控制和统一升级要求，在所有依赖模式下只允许动态消费 `libCredCore.dll`。

## 版本与链接

- 动态、静态均使用同一 CredCore 源码、OpenSSL 3.5.6、nlohmann/json 3.12.0、VS2022 v143、C++17。
- 动态 DLL 私有链接 `libcrypto.lib` 导入库，运行时使用 `libcrypto-3-x64.dll`。
- 静态归档保留 OpenSSL 未解析引用，供 CredCore 自身兼容测试、明确选择静态链接的独立工具或其他消费方使用；
  具体消费方必须显式定义 `CREDCORE_STATIC`，并自行选择同版本动态导入库或完全静态 `libcrypto.lib`。
- V3 的动态、静态两种依赖模式都不定义 `CREDCORE_STATIC`，统一链接 DLL 导入库，使 NATS、TLS、JWT 和
  PostgreSQL 共用 `libcrypto-3-x64.dll`。`libCredCore_static.lib` 不复制到 V3 vender 和发布目录。
- 静态模式不改变公开函数、返回码、Token 格式或 RS256 语义。

## 并发

- 公钥句柄创建完成后不可变，允许多线程并发调用验签；销毁与所有验签调用必须串行化。
- 每次验签创建独立摘要上下文，不共享可变 OpenSSL 上下文和错误栈。
- 动态或静态链接不改变上述线程模型；版本必须一致，才能把性能差异限定为链接和装载差异。
- V3 七服务的两种依赖模式都通过 CredCore DLL 使用共享 OpenSSL 3.5.6 DLL；CredCore 静态库的并发测试
  作为独立产物验证，不代表 V3 将其合入服务 EXE。完全静态 `no-asm` 产物只作为隔离工具的功能与兼容基线。

## 验收

- 两种配置构建成功，历史导出和兼容向量不变。
- CredCore DLL 测试和 CredCore 静态库独立消费测试均通过；V3 的 Dynamic/Static 依赖模式都必须通过
  `dumpbin /DEPENDENTS`、MAP 或链接器详细日志证明其使用 `libCredCore.dll`，且 EXE 不含 CredCore 算法静态符号。
- 8 线程共享只读公钥句柄的固定并发测试通过，并记录吞吐、失败数和资源释放结果。
- 更新 README 和版本说明，明确 CredCore 保留动态/静态产物，而 V3 固定选择动态消费。

