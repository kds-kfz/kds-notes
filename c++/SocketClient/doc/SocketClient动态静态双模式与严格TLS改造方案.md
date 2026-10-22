# SocketClient 动态静态双模式与严格 TLS 改造方案

> 日期：2026-10-21  
> 状态：`IMPLEMENTING`

## 背景

SocketClient 已支持 DLL 和静态归档，但只有静态 HP-Socket 路径执行 SAN/CN 主机名校验；动态路径开启
严格校验会关闭放行。V3 本次改为动态链接，因此必须先让 HP-Socket DLL 和 SocketClient DLL 共用
OpenSSL 3.5.6 DLL，再开放同等的严格校验。

## 实施

- `Release|x64`：SocketClient DLL + HP-Socket 6.0.9 DLL + OpenSSL 3.5.6 DLL。
- `ReleaseStatic|x64`：SocketClient 静态库 + HP-Socket 6.0.9 静态库；V3 最终服务继续链接 OpenSSL
  3.5.6 动态导入库，保证 NATS、TLS、JWT 和 PostgreSQL 共用一份密码运行时。
- 两种模式使用相同源码、ABI 4、C++17 和 `/MD` 调用方约束。
- 动态 SocketClient 私有链接 OpenSSL 3.5.6 导入库，仅用于握手后的 `X509_check_host` 和
  `X509_check_ip_asc`；HP-Socket DLL 必须动态链接同一组 OpenSSL DLL。
- 删除“动态模式无法校验主机名”的分支和文档限制，保留 CA、SNI、DNS/IP 主机名全部通过才通知连接成功。
- 每个 HTTP/HTTPS 实例仍为单在途；并发通过多实例或上层连接池实现，不改变回调归属语义。

## 静态并发结论

静态模式继续使用 6.0.9/3.5.6，同版本而非另选版本。七服务的最佳静态形态是“静态 SocketClient/
HP-Socket + 共享 OpenSSL DLL”；完全静态 OpenSSL 只用于隔离工具。链接形态不改变单实例在途限制，
也不自动提高并发。

## 验收

- 动静态库均构建并完成现有回环测试。
- 动态 DLL 依赖显示 HPSocket 和 OpenSSL 3.5.6；静态归档保留未解析依赖供最终程序闭环。
- 两种模式均通过正确/错误 CA、DNS、IP 和证书有效期测试。
- 产物、PDB、版本、构建参数和 SHA-256 写入版本说明和来源清单。

