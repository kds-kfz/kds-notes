# FoundationFacade

`FoundationFacade` 提供文件、目录、日期、休眠和基础平台信息。公共头位于 `include`，内部 Provider 与工厂位于 `src`，VS2022 工程位于 `project/vs2022/FoundationFacade`。

V3 生产使用 `FoundationStatic.lib`，由最终领域 Facade 私有吸收，不单独部署 Foundation DLL。入口创建不透明平台句柄并在同一最终模块释放；同步调用期间输入仅借用，组件不保存调用方缓冲区。
