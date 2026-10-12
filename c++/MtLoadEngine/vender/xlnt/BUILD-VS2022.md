# xlnt VS2022 x64 动态/静态构建说明

## 依赖信息

- 源码位置：`F:\开发资料\test_code\xlnt\xlnt`
- xlnt 版本：`1.5.0`
- 许可证：MIT，完整文本见同目录 `LICENSE.md`
- 工具链：Visual Studio 2022，`v143`，MSVC 19.44
- 目标平台：`x64`
- 构建类型：`Release`
- C++ 标准：C++17
- 运行库：动态 CRT（`/MD`）
- 库类型：共享库与静态库，两种模式统一使用 `/MD`

## CMake 参数

推荐从 `MtLoadEngine` 根目录执行：

```powershell
.\tools\BuildXlnt.ps1 -SourceRoot F:\开发资料\test_code\xlnt
```

脚本分别使用 `STATIC=OFF` 和 `STATIC=ON` 构建同一份源码，并固定 `TESTS=OFF`、`STATIC_CRT=OFF`、`XLNT_CXX_LANG=17`。静态消费方必须定义 `XLNT_STATIC`。

原版 1.5.0 在 Visual Studio 新版生成器下，静态目标可能因未启用 `CMP0091` 而回退到 `/MT`。本项目在顶层 `CMakeLists.txt` 启用 `CMP0091`，并让 `STATIC_CRT=OFF/ON` 显式选择 `/MD` 或 `/MT`；该修正只解耦库类型和 CRT 类型，不修改 xlnt 业务实现。

构建后应使用 `dumpbin /directives xlnt.lib` 检查静态库：本工程 `/MD` 产物必须出现 `DEFAULTLIB:MSVCRT`，不得出现 `LIBCMT`。

## 目录约定

```text
vender\xlnt\
├─ include\xlnt\                 # 公共头文件
├─ lib\x64vc14\share\xlnt.lib   # 动态版导入库
├─ lib\x64vc14\static\xlnt.lib  # 静态库
├─ bin\x64vc14\share\xlnt.dll   # 动态运行库
├─ LICENSE.md
└─ BUILD-VS2022.md
```

`x64vc14` 是现有工程沿用的目录名称，实际产物由 VS2022 `v143` 生成，不代表 VS2015 工具集。

## 使用边界

业务源码只通过 `vender\xlnt` 引用 xlnt，不直接引用原始源码目录或临时构建目录。动态和静态库同名，必须通过 `share/static` 目录隔离；发布动态 `MtLoadEngine.dll` 时复制 `xlnt.dll`，静态链不得部署或导入 `xlnt.dll`。
