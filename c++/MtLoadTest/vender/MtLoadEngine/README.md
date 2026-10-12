# MtLoadEngine 发布依赖

本目录保存 `MtLoadTest` 对压测执行库的稳定依赖副本。界面工程不引用 `MtLoadEngine` 源码目录，也不实现网络、调度、解析和指标计算。

## 发布口径

- 平台：x64
- 配置：Release（动态）与 ReleaseStatic（静态）
- 工具集：Visual Studio 2022 `v143`
- 运行库：动态 CRT（`/MD`）
- ABI：以 `include\MtLoadTypes.h` 中 `MTLOAD_ABI_VERSION` 为准

## 目录约定

```text
vender\MtLoadEngine\
├─ include\                 # 公共 C ABI 头文件
├─ lib\x64vc14\share\      # 动态版导入库
├─ lib\x64vc14\static\     # 静态版 MtLoadEngine.lib
├─ bin\x64vc14\share\      # 动态版 DLL 及运行时依赖
└─ README.md
```

更新压测库时必须整体同步头文件、两种库及动态运行时依赖，并重新执行 `MtLoadTest Release|x64`、`ReleaseStatic|x64` 构建和启动冒烟测试，不能只替换单个产物。
