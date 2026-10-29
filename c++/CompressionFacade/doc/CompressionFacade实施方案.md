# CompressionFacade 实施方案

## 目标

建立由 CloudNetDataApi 私有吸收的压缩组件，首期静态吸收 Snappy，后续按稳定枚举扩展 zlib 和 gzip。未实现算法明确返回 `FACADE_NOT_SUPPORTED`，不自动回退或替换算法。

## 接口与实现

- 公共 C ABI 使用不透明 codec 句柄、只读输入视图和调用方输出缓冲区。
- `ICompressionCodecFactory` 与 Codec 具体实现只留在组件内部。
- 创建配置同时限制单次最大输入和最大解压输出，所有长度在分配前校验。
- V3 生产只使用 ReleaseStatic 生成的 `CompressionStatic.lib`；Release DLL 仅保留兼容构建，不进入生产包。

## 验收

- 覆盖 Snappy 固定向量、往返、空输入、损坏数据、容量不足、输出上限和未知算法。
- 业务工程与 CloudNet 源码不直接包含 Snappy；Snappy 只由 `CompressionStatic.lib` 封装并最终进入 CloudNetDataApi。
