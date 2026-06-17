# IceRPCPush 开发说明

## Ice Slice 代码生成

`src\Corba\JSONBINRPCU.ICE` 是 Ice RPC 接口的 Slice 源文件，`JSONBINRPCU.h` 和 `JSONBINRPCU.cpp` 由 Ice 3.8.2 的 `slice2cpp.exe` 生成。

生成文件属于工具产物，不手工逐函数补注释。接口含义、参数约束和业务说明应优先写在 `JSONBINRPCU.ICE`、手写封装层代码或本文档中，避免重新生成后丢失。

## 工具位置

外部 Ice 3.8.2 工具原始位置：

```powershell
F:\gwz\trank\ice-3.8.2\cpp\bin\x64\Release\slice2cpp.exe
```

项目内固定工具位置：

```powershell
F:\开发资料\MyCode\kds-notes\c++\IceRPCPush\tool\slice2cpp.exe
```

复制命令：

```powershell
Copy-Item -LiteralPath "F:\gwz\trank\ice-3.8.2\cpp\bin\x64\Release\slice2cpp.exe" -Destination "F:\开发资料\MyCode\kds-notes\c++\IceRPCPush\tool\slice2cpp.exe" -Force
```

当前版本依赖检查结果显示 `slice2cpp.exe` 只依赖系统 DLL `KERNEL32.dll`，不需要额外随项目复制 Ice DLL。以后如果替换 Ice 版本，需要重新执行依赖检查：

```powershell
dumpbin /dependents "F:\开发资料\MyCode\kds-notes\c++\IceRPCPush\tool\slice2cpp.exe"
```

## 生成流程

进入 Slice 文件目录：

```powershell
cd /d "F:\开发资料\MyCode\kds-notes\c++\IceRPCPush\src\Corba"
```

使用项目内工具生成 C++ 文件：

```powershell
..\..\tool\slice2cpp.exe -IF:\gwz\trank\ice-3.8.2\slice JSONBINRPCU.ICE
```

`JSONBINRPCU.ICE` 中包含 `<Ice/Identity.ice>`，所以必须通过 `-I` 指定 Ice Slice 文件目录。未指定 `-I` 时，工具会提示无法打开 `Ice/Identity.ice`。

原始外部工具命令为：

```powershell
F:\gwz\trank\ice-3.8.2\cpp\bin\x64\Release\slice2cpp.exe JSONBINRPCU.ICE
```

本工程实际可运行的外部工具命令为：

```powershell
F:\gwz\trank\ice-3.8.2\cpp\bin\x64\Release\slice2cpp.exe -IF:\gwz\trank\ice-3.8.2\slice JSONBINRPCU.ICE
```

生成后应得到以下文件：

- `src\Corba\JSONBINRPCU.h`
- `src\Corba\JSONBINRPCU.cpp`

## 校验步骤

确认工具版本：

```powershell
.\tool\slice2cpp.exe --version
```

期望输出：

```text
3.8.2
```

重新生成后检查文件是否能参与 Release x64 构建：

```powershell
msbuild project\vs2022\IceRPCPush.sln /p:Configuration=Release /p:Platform=x64
```

如果生成文件内容发生变化，需要重点检查 Ice 3.8.2 生成代码中的 `std::byte`、新版代理类、异步回调和 AMD `response/exception` 相关适配是否仍与手写代码匹配。
