# SocketServer 版本修改记录

## 记录信息

- 修改日期：2026-03-30
- 修改范围：TCP 服务稳定性修复、内存安全修复、日志安全修复
- 记录人：Codex

## 本次修改目标

本次修改优先处理代码审查中发现的高危问题，目标不是先做协议重构或性能压榨，而是先把最容易导致崩溃、内存泄漏、生命周期错乱的问题收敛下来。

本次重点目标如下：

- 降低服务启动和停止阶段的并发崩溃风险
- 修复明显的内存泄漏点
- 修复日志相关的越界读取和格式串风险
- 修复第三方对象释放方式错误带来的稳定性问题

## 本次涉及文件

- `TcpSockServerObj.cpp`
- `TcpServerListerNet.cpp`
- `Log.cpp`
- `Struct.h`

## 详细修改说明

### 1. 调整 TCP 服务的启动和销毁顺序

涉及文件：

- `TcpSockServerObj.cpp`

修改前的问题：

- 原来的执行顺序是先 `Start()` 服务，再初始化互斥量、再启动线程池。
- 如果客户端在这个窗口期接入，监听回调可能提前进入 `pthread_mutex_lock()` 或线程池 `Submit()`，此时运行时资源尚未准备完成。
- `DeleteObj()` 的清理逻辑依赖 `g_bServerStatus`，但这个状态在旧代码里没有被可靠地置为 `true`，导致停服或重复启动时清理不完整。

本次修改内容：

- 新增 `InitTcpMutexes()`、`DestroyTcpMutexes()`、`ClearTcpRuntimeData()` 三个辅助函数，用于集中管理 TCP 运行时资源。
- 将互斥量初始化和线程池启动前置到 `g_CTcpPackServer->Start(...)` 之前。
- 统一 `DeleteObj()` 清理顺序：
  - 先将 `g_bServerStatus` 置为 `false`
  - 停止 HP-Socket 服务对象
  - 停止线程池
  - 清理连接表、任务表、请求缓存表
  - 销毁互斥量
  - 置空回调和任务计数器
- 在 `CreateTcpSock()` 参数校验通过后先执行一次 `DeleteObj()`，确保重复启动时不会继承上一次残留状态。

为什么这样修改：

- 服务 `Start()` 之后回调可能立刻到来，因此共享资源必须先准备好。
- 清理流程必须可重复执行且完整，否则服务多次启停后会逐步进入不稳定状态。

预期效果：

- 降低服务启动窗口期的竞态风险
- 降低重复启停后的异常概率
- 避免失败启动或停服后残留脏状态

### 2. 修复 HP-Socket 对象释放方式错误

涉及文件：

- `TcpSockServerObj.cpp`

修改前的问题：

- `g_CTcpPackServer` 是通过 `HP_Create_TcpServer(...)` 创建的。
- 旧代码使用 `delete` 直接释放第三方库对象。

本次修改内容：

- 将 `delete g_CTcpPackServer` 改为 `HP_Destroy_TcpServer(g_CTcpPackServer)`。

为什么这样修改：

- 第三方库创建的对象必须使用其配套的销毁函数释放。
- 否则容易出现堆不一致、析构不完整甚至直接崩溃。

预期效果：

- 降低停服和异常清理路径上的崩溃风险

### 3. 修复 `NotifyTask` 数据缓冲泄漏

涉及文件：

- `Struct.h`

修改前的问题：

- `NotifyTask::pBuf` 在数据通知路径里会动态申请内存。
- 析构函数中原本注释掉了 `delete[] pBuf`，导致每次数据通知都可能泄漏一块缓冲区。

本次修改内容：

- 恢复 `NotifyTask` 析构中的 `delete[] pBuf`。

为什么这样修改：

- 以当前设计来看，`NotifyTask` 对 `pBuf` 拥有所有权。
- 任务从 `g_mapTask` 删除后，应由任务对象自己释放这块数据。

预期效果：

- 避免数据通知路径上的持续性内存泄漏
- 避免长时间运行后内存持续增长

### 4. 修复 TCP 原始数据日志打印的越界风险

涉及文件：

- `TcpServerListerNet.cpp`

修改前的问题：

- 旧代码把收到的 TCP 数据按 `%s` 直接打印。
- 但网络数据并不保证是带 `\0` 结尾的字符串。
- 如果是二进制数据或普通文本但未补零，日志打印就可能继续向后读，造成越界读取甚至崩溃。

本次修改内容：

- 删除 TCP 数据通知路径中对原始 payload 的 `%s` 输出。
- 改为只记录安全的元信息：
  - 客户端 IP
  - 客户端端口
  - 通知类型
  - 数据长度

为什么这样修改：

- 网络数据不能默认按 C 字符串处理。
- 在高频路径里记录元信息比直接打印包体更安全，也更稳定。

预期效果：

- 降低日志引起的越界读风险
- 降低收到异常包、二进制包时的崩溃概率

### 5. 补强任务回调的安全保护

涉及文件：

- `TcpServerListerNet.cpp`

修改前的问题：

- `ThreadNotifyTask()` 默认假设 `socketTask`、`socketTask->buf` 和回调状态一定有效。
- 旧代码直接访问 `g_mapClient[pstTask->ullConnID]`，如果连接不存在，会隐式插入一个默认对象。

本次修改内容：

- 为 `socketTask` 和 `socketTask->buf` 增加空指针校验。
- 客户端信息改为先 `find()` 再读取，避免隐式插入默认值。
- 在回调通知上层前增加 `g_bServerStatus` 判断。
- 在以下关键监听回调中增加服务状态保护：
  - `OnPrepareListen`
  - `OnAccept`
  - `OnClose`
  - `OnReceive`

为什么这样修改：

- 启停边界时最容易出现“对象还在、状态已经失效”的情况。
- 这类阶段不应该继续制造新的运行时状态，也不应该继续扩散回调。

预期效果：

- 降低无效状态下继续执行回调的风险
- 避免连接表出现无意义的默认数据

### 6. 修复 Unicode 配置下客户端 IP 拷贝错误

涉及文件：

- `TcpServerListerNet.cpp`

修改前的问题：

- 工程包含 Unicode 配置。
- 原实现把 `TCHAR szAddress[]` 直接 `memcpy` 到 `char szIp[32]`。
- 在 Unicode 配置下，这种做法会导致 IP 乱码、缺少结尾符甚至后续读取异常。

本次修改内容：

- 新增 `CopyClientIp(...)` 辅助函数。
- Unicode 配置下通过 `WideCharToMultiByte(...)` 做字符转换。
- 非 Unicode 配置下通过 `_snprintf(...)` 做安全复制。
- 强制补齐目标缓冲区结尾 `\0`。

为什么这样修改：

- 地址字符串应按字符宽度做转换，不能直接按字节拷贝。
- `wchar_t*` 到 `char*` 的裸拷贝既不安全也不正确。

预期效果：

- Unicode 配置下客户端 IP 输出正确
- 降低日志和回调里的脏数据、越界读风险

### 7. 修复日志刷盘的格式串风险

涉及文件：

- `Log.cpp`

修改前的问题：

- 旧代码用 `fprintf(m_pFileLog, m_vecCacheLog[i].c_str())` 刷盘。
- 如果日志内容里本身包含 `%`，就会被再次当成格式串解释。

本次修改内容：

- 改为 `fprintf(m_pFileLog, "%s", m_vecCacheLog[i].c_str())`。

为什么这样修改：

- 日志内容必须始终按普通数据处理，不能按格式模板处理。
- 特别是网络输入可能间接进入日志时，这种风险必须先消除。

预期效果：

- 降低格式串引发的异常和崩溃风险

### 8. 修复日志格式化时 `va_list` 被重复消费的问题

涉及文件：

- `Log.cpp`

修改前的问题：

- `vsnprintf(...)` 在重试扩容循环里复用了同一个 `va_list`。
- 被消费过的 `va_list` 再次使用属于未定义行为。

本次修改内容：

- 将 `va_start(...)` / `va_end(...)` 移入每次重试循环内部。
- 将 `strContent` 的赋值改为按实际格式化长度 `iRet` 复制，而不是整块临时缓冲区都塞进去。

为什么这样修改：

- 动态扩容重试时，参数列表必须重新初始化。
- 否则日志内容可能异常，严重时会导致未定义行为。

预期效果：

- 提高日志格式化稳定性
- 减少日志路径上的不确定行为

### 9. 调整 TCP 主动关闭接口行为

涉及文件：

- `TcpSockServerObj.cpp`

修改前的问题：

- `TcpSockClose()` 强依赖关闭前必须携带一段非空数据。
- 这导致“只想断开连接，不想额外发包”的场景无法直接调用。

本次修改内容：

- 允许 `TcpSockClose()` 在没有关闭数据包时直接执行断开。
- 只有在调用方确实传入数据时才先发送数据。

为什么这样修改：

- 发送和关闭是两个不同动作。
- 一个正常的断连动作不应依赖额外业务数据。

预期效果：

- 上层主动断连行为更符合直觉

## 构建验证结果

### Release|x64

验证结果：

- 本次修改后的源码可以通过编译阶段。
- 最终失败在链接阶段，不是本次代码改动导致的编译错误。

当前阻塞问题：

- `LNK1104: 无法打开输出文件 ...\\lib\\x64\\libSocketServer.dll`

结论：

- 当前代码修改本身没有阻塞编译。
- 剩余问题更像是输出路径不可写、目标文件被占用，或者项目输出目录配置问题。

### Debug|x64

验证结果：

- 该配置当前仍无法用于有效验证。

当前阻塞问题：

- 缺少第三方头文件目录配置，导致以下头文件找不到：
  - `SocketInterface.h`
  - `pthread.h`

结论：

- `Debug|x64` 的 `AdditionalIncludeDirectories` 没有和 `Release|x64` 对齐。
- 这是工程本身已有的配置问题，不是本次代码修改引入的。

## 编码处理说明

本次修改过的源文件已统一保存为 UTF-8 with BOM。

为什么这样处理：

- 工程中包含中文注释和中文日志文本。
- 在 VS2015 和本地代码页环境下，编码不统一时容易出现 `C4819` 警告或中文显示异常。

## 本轮尚未处理的问题

### 1. TCP 拆包 / 粘包逻辑仍未修复

当前状态：

- 现在的接收逻辑仍然把一次 `OnReceive(...)` 回调视为一条完整业务消息。

为什么仍是风险：

- TCP 是字节流，不是天然消息边界协议。
- 半包、粘包仍然会发生。

为什么本轮没有直接修改：

- 真正修复必须依赖业务协议格式：
  - 定长包
  - 长度头包
  - 分隔符包
  - 自定义包头包尾

### 2. WebSocket / HTTP 路径仍有类似问题

当前状态：

- 本轮优先修的是 TCP 主链路。
- WebSocket 侧代码与 TCP 侧有大量相似结构，理论上也应按同样思路做一轮整理。

为什么本轮没有继续扩展：

- 当前目标是先给出一版可落地的高危修复版本，避免一次性改动过大。

### 3. `ERROR` 宏与 Windows 头文件冲突

当前状态：

- 编译时仍会出现 `ERROR` 宏重定义告警。

为什么仍值得处理：

- 这类宏名冲突虽然不一定立刻导致功能错误，但会增加 include 顺序敏感性，也会制造编译噪音。

建议后续处理：

- 将自定义日志宏 `ERROR` 改为更明确的项目内名称，例如 `LOG_ERROR`。

## 源码注释同步说明

本次已在对应源码修改点补充中文注释，并统一附加修改日期 `2026-03-30`，便于后续维护时快速定位这轮高危问题修复的背景。

## 建议下一步

下一步最值得继续处理的是 `TcpServerListerNet.cpp` 中的 TCP 组包逻辑。
建议先明确业务协议格式，再按协议把拆包、粘包问题彻底修正，然后再进入性能优化阶段。

## 2026-03-30 补充：Web 服务同步修复

本轮在检查 Web 服务时，发现其实现仍保留了 TCP 修复前的大部分旧逻辑，因此按同样思路补做了一轮同步修复。

### 补充修复点 1：Web 主动断开不再依赖 `SO_CLOSE`

涉及文件：

- `publicGlobalvar.h`
- `publicGlobalvar.cpp`
- `WebSockServerObj.cpp`
- `WebServerListerNet.cpp`

修改前的问题：

- `WebSockClose()` 主动断开后，`OnClose()` 里通过 `SO_CLOSE == enOperation` 粗略判断是否是本端主动关闭。
- 这种判断和底层实现细节绑定过深，不够稳定，也不够直观。

本次修改内容：

- 新增 `g_setWebLocalClosing` 集合，显式记录“本端主动关闭中的连接”。
- `WebSockClose()` 在调用 `Disconnect()` 前先插入连接标记。
- `OnClose()` 优先根据集合判断是否属于本端主动断开，再决定是否只做内部清理而不向上层重复派发关闭通知。
- `Disconnect()` 调用失败时，主动回滚标记并清理缓存，避免状态残留。

为什么这样修改：

- 是否属于“本端主动关闭”，应由本端显式记录，而不是靠 `enOperation` 或系统错误码反推。

预期效果：

- Web 服务主动断连语义更清晰
- 降低误判关闭来源的风险

### 补充修复点 2：Web 服务启动/销毁顺序与资源释放同步整改

涉及文件：

- `WebSockServerObj.cpp`
- `publicGlobalvar.h`
- `publicGlobalvar.cpp`

修改前的问题：

- Web 服务同样存在先 `Start()`、后初始化锁和线程池的问题。
- Web 运行状态仍错误复用了 `g_bServerStatus`。
- `IHttpServer` 对象仍用 `delete` 释放。
- 清理阶段还误用了 `g_mapTask`，没有真正清理 `g_mapWebTask`。

本次修改内容：

- 新增独立的 `g_bWebServerStatus` 状态变量。
- 新增 `InitWebMutexes()`、`DestroyWebMutexes()`、`ClearWebRuntimeData()`。
- 将 Web 线程池启动和互斥量初始化前置到 `Start()` 之前。
- `DeleteWebObj()` 改为使用 `HP_Destroy_HttpServer(...)` 释放服务对象。
- 修正清理逻辑，正确清空：
  - `g_mapWebClient`
  - `g_mapWebTask`
  - `g_mapWebQueue`
  - `g_setWebLocalClosing`

为什么这样修改：

- Web 服务和 TCP 一样，也存在启动窗口期竞态和停服清理不完整问题。
- Web 状态不应与 TCP 共用同一个运行标志。

预期效果：

- 降低 Web 服务重复启停风险
- 降低 Web 侧资源泄漏和脏状态残留风险

### 补充修复点 3：Web 通知与收包路径同步加固

涉及文件：

- `WebServerListerNet.cpp`

修改前的问题：

- `ThreadWebNotifyTask()` 直接使用 `g_mapWebClient[pstTask->ullConnID]`，可能隐式插入默认对象。
- 数据通知路径仍按 `%s` 输出 WebSocket 原始数据，存在越界读取风险。
- `OnAccept()` 中客户端地址仍通过 `memcpy` 从 `TCHAR` 直接复制到 `char`。
- 多个回调在停服边界仍可能继续处理。

本次修改内容：

- 为 `ThreadWebNotifyTask()` 增加空指针保护。
- 客户端查询改为 `find()`。
- 数据日志改为只记录元信息，不再直接打印原始包体。
- 新增 `CopyWebClientIp(...)`，统一处理 Unicode 和非 Unicode 地址复制。
- 在以下关键回调中增加 `g_bWebServerStatus` 判断：
  - `OnPrepareListen`
  - `OnAccept`
  - `OnClose`
  - `OnWSMessageBody`
  - `OnReceive`

为什么这样修改：

- Web 服务同样存在停服边界、Unicode 配置和二进制数据日志安全问题。

预期效果：

- 降低 Web 通知路径上的脏状态和越界风险
- 提升 Web 服务与 TCP 服务的一致性

### 补充修复点 4：Web 收包路径顺手修正了两个明显细节

涉及文件：

- `WebServerListerNet.cpp`

本次修改内容：

- 将 `iLength > pSender->GetSocketBufferSize()` 的比较改为显式整型比较，避免告警。
- 将 `_snprintf(... "%llu" ...)` 改为按缓冲区实际类型使用 `%u`，避免格式化参数类型不匹配。

### 本次补充后的验证结果

- `Release|x64` 重新编译后，源码仍可通过编译阶段。
- 当前仍然只失败在链接阶段：
  - `LNK1104: 无法打开 ...\\lib\\x64\\libSocketServer.dll`

说明：

- 本次 Web 服务补充修复没有引入新的编译错误。

## 2026-03-30 补充：TCP 接收策略与 WebSocket 消息边界修正

本轮在继续检查 TCP / Web 服务的收包逻辑时，确认了两件非常关键的事情：

- TCP 服务当前不应在库内做长度头组包。
- WebSocket 服务则应在库内按完整消息结束时再上抛。

### 补充修复点 5：TCP 服务改为按实际收到的字节块直接通知上层

涉及文件：

- `TcpServerListerNet.cpp`

修改背景：

- 旧代码试图在库内把一次 `OnReceive()` 回调当成“完整请求包”处理。
- 但根据当前上层应用设计，真正的接收缓存和协议拆包在上层完成：
  - 上层会先按请求头结构体长度缓存
  - 结构化请求头后拿到包体长度
  - 再继续累计包体直到完整请求就绪
- 因此 TCP 库层不应该自行猜测包边界，更不应该在库内做错误的“伪完整包”判断。

本次修改内容：

- 删除 TCP 接收路径里原先按 `iLength` 分配缓存并判断“完整包”的逻辑。
- 改为收到多少字节，就按多少字节原样复制并通过 `enTcpData` 通知上层。
- 保留 TCP 服务的 Push 模型定位，不在库层实现长度头协议。
- 对 Pull 模型回调补充告警说明，明确当前实例不应该走到该路径。

为什么这样修改：

- 这更符合你们现有的协议处理职责划分。
- TCP 是字节流，上层既然已经有独立缓存并按请求头协议拆包，库层继续做组包反而会干扰上层正确解析。
- 这样可以彻底去掉“一个半包被误判成完整包”或“多个小包被库层错误拼接”的风险。

预期效果：

- TCP 库层职责更清晰，只负责可靠上抛原始接收字节块
- 上层应用可以继续按既有请求头结构体协议完成拆包
- 避免库内伪组包导致的协议错位

### 补充修复点 6：WebSocket 改为完整消息结束后再通知上层

涉及文件：

- `Struct.h`
- `WebServerListerNet.cpp`

修改前的问题：

- 旧代码在 `OnWSMessageBody()` 中把当前一次 Body 回调收到的字节数直接当成完整消息长度。
- 如果一条 WebSocket 消息被拆成多次 Body 回调，旧逻辑会提前把半截消息通知给上层。
- `OnWSMessageComplete()` 明明是“完整消息/帧接收完成”时机，却没有承担最终上抛职责。

本次修改内容：

- 扩展 `ReqCacheData`，补充 WebSocket 当前帧和当前消息的累计状态字段。
- 在 `OnWSMessageHeader()` 中记录当前帧长度、是否结束帧、操作码等状态。
- 在 `OnWSMessageBody()` 中只做累计，不再提前通知上层。
- 在 `OnWSMessageComplete()` 中判断当前消息是否完整结束：
  - 非最终帧：继续等待后续分片
  - 控制帧：只校验当前帧完整性，不上抛业务数据
  - 最终数据消息：一次性通过 `enWebData` 通知上层
- 增加 WebSocket 最大消息长度保护，避免异常长度导致缓存无限膨胀。

为什么这样修改：

- HP-Socket 已经为 WebSocket 提供了 Header / Body / Complete 三级回调，库层应利用这套语义做正确的消息级通知。
- 与 TCP 不同，WebSocket 的消息边界本来就是协议层概念，库层处理完整消息更合适。

预期效果：

- 避免 WebSocket 半消息提前通知上层
- 降低分片消息、控制帧混入时的协议错位风险
- 让 Web 服务的上层回调更接近“完整业务消息”的语义

### 本轮补充后的验证结果

- `Release|x64` 重新编译后，源码仍可通过编译阶段。
- 当前仍然只失败在链接阶段：
  - `LNK1104: 无法打开 ...\\lib\\x64\\libSocketServer.dll`

说明：

- 本轮 TCP / WebSocket 包边界修正没有引入新的编译错误。

## 2026-03-30 补充：WebSocket 协议实现高风险问题修复

### 补充修复点 7：WebSocket 发送和关闭改为真正的消息帧

涉及文件：

- `WebSockServerObj.cpp`

修改前的问题：

- `WebSockSend()` 使用的是 `SendResponse(...)`，实际发送的是 HTTP 响应，不是 WebSocket 消息帧。
- `WebSockClose()` 关闭前如果携带数据，同样走的是 HTTP 响应。
- `WebSockClose()` 随后直接强制 `Disconnect()`，前面的最后一包数据未必能真正发出。

本次修改内容：

- `WebSockSend()` 改为使用 `SendWSMessage(...)` 发送 WebSocket 数据帧。
- 当前统一按二进制帧发送，保持与上层“原始字节块”接口语义一致。
- `WebSockClose()` 如果带业务数据，先发送一帧二进制消息，再补发 WebSocket Close 帧。
- `WebSockClose()` 的断开方式改为优雅断开 `Disconnect(dwConnID, false)`，让排队中的数据帧和 Close 帧有机会送出。

为什么这样修改：

- WebSocket 连接建立后，后续通信必须走 WebSocket 帧协议，不能再混用 HTTP 响应。
- 强制断开会让“关闭前最后一包”存在丢失风险。

预期效果：

- 修正 WebSocket 发送协议错误
- 降低客户端协议异常和关闭前最后消息丢失风险

### 补充修复点 8：移除 WebSocket 消息头回调中的测试回发逻辑

涉及文件：

- `WebServerListerNet.cpp`

修改前的问题：

- `OnWSMessageHeader()` 中残留了测试代码，每次收到消息头都会主动回发一条文本消息。
- 回发使用了 `static char buff[32]` 和 `static int cnt`，并发连接下会出现共享状态竞争。

本次修改内容：

- 删除 `OnWSMessageHeader()` 中的测试回发逻辑。
- 保留消息状态检查，数据通知继续统一走 `OnWSMessageComplete()`。

为什么这样修改：

- 测试代码不应残留在正式服务收包链路中。
- 并发连接共享静态缓冲会带来串包和数据竞争风险。

预期效果：

- 避免服务无故多发消息
- 避免并发下测试状态串扰

### 补充修复点 9：WebSocket 握手改为在请求头完整后统一处理

涉及文件：

- `WebServerListerNet.h`
- `WebServerListerNet.cpp`

修改前的问题：

- 原逻辑在 `OnHeader()` 收到单个 `Sec-WebSocket-Key` 头时就立即回握手。
- 生成握手值时使用 `static char key[256]`，并发握手时不同连接可能互相覆盖。
- 设置了 `Sec-WebSocket-Protocol` 头，但实际发送数量写成了 `3`，该头根本没有发出去。

本次修改内容：

- `OnHeader()` 改为仅做头解析，不再提前发送握手响应。
- 新增 `OnHeadersComplete()` 实现，在请求头完整后统一读取 `Sec-WebSocket-Key` 并生成握手应答值。
- 去掉静态共享缓冲，改为局部变量生成 `Sec-WebSocket-Accept`。
- 统一按 3 个必要握手头发送：
  - `Upgrade: websocket`
  - `Connection: Upgrade`
  - `Sec-WebSocket-Accept: ...`

为什么这样修改：

- 握手应在请求头完整后进行，避免提前回包导致状态不一致。
- 并发握手路径不能使用静态共享缓冲。
- 非必要且未真正发出的子协议头应先去掉，避免误导客户端。

预期效果：

- 提升并发握手稳定性
- 避免握手数据串扰和头部数量错误

## 2026-03-30 补充：TCP 关闭语义、过载保护与 WebSocket 缓存回收

### 补充修复点 10：TCP 关闭改为优雅断开

涉及文件：

- `TcpSockServerObj.cpp`

修改前的问题：

- `TcpSockClose()` 在可选应答数据发送后立即调用默认强制 `Disconnect()`。
- 如果上层依赖“发最后一包再关闭”，最后数据存在来不及真正送达的风险。

本次修改内容：

- 关闭前如果带数据，先检查发送结果并补日志。
- 断开方式改为 `Disconnect(dwConnID, false)`，走优雅关闭路径。

为什么这样修改：

- 在不改变上层接口的前提下，让关闭前已经排队的数据有机会真正发出。

预期效果：

- 降低 TCP 连接关闭前最后一包数据丢失风险

### 补充修复点 11：TCP 原始字节通知增加轻量过载保护

涉及文件：

- `Struct.h`
- `TcpServerListerNet.cpp`

修改前的问题：

- 当前 TCP 服务按你的要求“收到多少字节就原样抛给上层”。
- 但每个小包都会分配一块新内存并投递一个线程池任务。
- 遇到小包洪泛时，任务数和排队内存会按包数持续增长。

本次修改内容：

- 为 `ReqCacheData` 增加 TCP 待处理任务数和待处理字节数统计字段。
- 在 TCP `OnReceive()` 中先为当前连接预留通知配额。
- 当单连接待处理任务数或累计字节数超过阈值时，直接记录错误并返回失败，阻止继续堆积。
- 在数据通知任务执行完成后归还配额，空闲时顺手删除统计对象。

为什么这样修改：

- 不改变“原样抛上层”的语义，但为异常流量增加一层最基本的自保护能力。

预期效果：

- 降低小包洪泛导致的线程池和内存持续膨胀风险
- 保留现有上层协议缓存逻辑不变

### 补充修复点 12：WebSocket 大缓存消息完成后按阈值回收

涉及文件：

- `WebServerListerNet.cpp`

修改前的问题：

- 某个长连接只要收过一次大消息，就可能长期占住那块大缓存。
- 多连接场景下，这会形成明显的内存放大点。

本次修改内容：

- 新增 WebSocket 缓存回收逻辑。
- 完整消息处理结束后，如果缓存容量超过保留阈值，则主动释放缓存，只保留小块缓存复用。

为什么这样修改：

- 大消息属于峰值场景，不应长期占用连接常驻内存。

预期效果：

- 降低 WebSocket 长连接的峰值内存残留
- 保留小消息场景下的缓存复用能力

## 2026-03-30 补充：剩余中风险边界修正

### 补充修复点 13：修正 TCP 过载保护中的大包边界判断

涉及文件：

- `TcpServerListerNet.cpp`

修改前的问题：

- TCP 待处理字节配额判断中直接使用了 `limit - uiDataLen`。
- 当单次数据块本身大于配额上限时，会发生无符号下溢，导致保护条件被错误绕过。

本次修改内容：

- 先显式判断 `uiDataLen <= 配额上限`，再继续做剩余空间比较。

为什么这样修改：

- 避免单包过大时绕过本来用于保护线程池和内存的限流逻辑。

预期效果：

- TCP 原始字节通知的过载保护边界更可靠

### 补充修复点 14：WebSocket 握手增加标准请求校验

涉及文件：

- `WebServerListerNet.cpp`

修改前的问题：

- 只要请求头里带 `Sec-WebSocket-Key`，就可能直接进入握手响应。
- 缺少对以下关键条件的统一校验：
  - 请求方法是否为 `GET`
  - `Upgrade` 头是否为 `websocket`
  - `Connection` 头是否包含 `Upgrade`
  - `Sec-WebSocket-Version` 是否为 `13`
  - 协议升级类型是否确认为 `WebSocket`

本次修改内容：

- 新增 ASCII 不区分大小写比较和头部 token 检查辅助函数。
- 在 `OnHeadersComplete()` 中统一校验标准 WebSocket 升级条件。
- 条件不满足时直接按握手失败处理，不再误升级。

为什么这样修改：

- 避免异常或伪造请求头把普通 HTTP 请求误带入 WebSocket 握手流程。

预期效果：

- 提升 WebSocket 握手阶段的协议严谨性
- 降低异常请求扰乱连接状态机的风险

### 补充修复点 15：收到对端 Close 帧时改为标准回关闭流程

涉及文件：

- `WebServerListerNet.cpp`

修改前的问题：

- 收到对端 `Close` 帧后直接返回 `HR_ERROR`，更像异常中断而不是协议级关闭。

本次修改内容：

- 收到对端 `Close` 帧时先尝试回发一个 `Close` 帧。
- 然后调用优雅断开，让后续 `OnClose()` 继续走现有关闭通知流程。

为什么这样修改：

- WebSocket 关闭应尽量遵循协议规定的双向关闭握手，而不是直接硬断。

预期效果：

- 降低客户端把正常关闭识别成异常断开的概率

## 2026-03-30 补充说明：Linux 兼容性注意事项

说明：

- 本次新增逻辑内部已尽量统一使用标准 C++ 写法，例如 `const char*`、`bool`、`true/false`，避免继续引入新的 Windows 专用类型或宏。
- 当前 `WebServerListerNet.h/.cpp` 中少量 `LPCSTR`、`BOOL` 出现在监听器回调函数签名里，这部分来自现有 HP-Socket 接口定义要求，不能在当前 Windows 工程里直接改掉，否则会破坏 `override` 关系。
- 后续如果同步 Linux 版本，新增实现代码应继续优先使用标准 C++ 类型；接口层则以 Linux 侧对应的适配头文件或抽象层定义为准，不再额外扩散 Windows 风格类型。

## 2026-03-30 补充：停服任务生命周期、共享日志与 HTTP Body 行为修正

### 补充修复点 16：线程池停服改为等待任务自然退出

涉及文件：

- `TcpSockServerObj.cpp`
- `WebSockServerObj.cpp`

修改前的问题：

- 停服时线程池使用 `Stop(0)`。
- HP-Socket 说明里已经明确，超过等待时间会尝试强制关闭工作线程。
- 结合当前通知任务的内存持有方式，强制停池后立刻清理 `g_mapTask/g_mapWebTask`，存在把仍在执行中的任务数据提前释放掉的风险。

本次修改内容：

- TCP 和 Web 停服时都改为调用线程池默认 `Stop()`，等待已提交任务自然退出。

为什么这样修改：

- 避免“任务线程还没退，任务对象先被清掉”导致的悬空指针和崩溃风险。

预期效果：

- 提升停服阶段的稳定性
- 降低强制停池引发的任务内存越界访问风险

### 补充修复点 17：旧通知提交流程统一改为安全封装

涉及文件：

- `TcpServerListerNet.cpp`
- `WebServerListerNet.cpp`

修改前的问题：

- TCP/Web 的连接通知、关闭通知、错误通知仍然走旧代码路径。
- 旧路径会先把 `NotifyTask*` 塞进 `g_mapTask/g_mapWebTask`。
- 如果后续创建任务对象或线程池提交失败，只 `delete pNotifyTask`，没有同步把 map 中的指针删除，后续清理时会再次释放同一块内存。

本次修改内容：

- TCP 的连接通知、关闭通知统一改为走 `SubmitTcpNotifyTask()`。
- Web 的连接通知、关闭通知、错误通知统一改为走 `SubmitWebNotifyTask()`。
- 提交失败时由统一封装完成 map 回滚和对象释放，不再让旧路径残留悬空指针。

为什么这样修改：

- 统一失败清理逻辑，避免个别路径遗漏回滚动作。

预期效果：

- 降低通知任务提交失败时的双重释放和野指针风险
- 让 TCP/Web 通知链路的生命周期管理更一致

### 补充修复点 18：共享日志实例按服务持有状态释放

涉及文件：

- `publicGlobalvar.h`
- `publicGlobalvar.cpp`
- `TcpSockServerObj.cpp`
- `WebSockServerObj.cpp`

修改前的问题：

- TCP 和 Web 共用同一个 `CLog` 单例。
- 任意一侧停服时都会直接 `CLog::Release()`。
- 如果两个服务同时运行，停掉其中一个时可能把另一个仍在使用的日志对象一起释放。

本次修改内容：

- 新增 TCP/Web 对共享日志实例的持有标记。
- 初始化日志成功后标记本服务持有日志。
- 停服时先释放本服务持有标记，只有在 TCP 和 Web 都不再持有时才真正 `CLog::Release()`。

为什么这样修改：

- 共享单例的释放时机应基于“是否还有其他服务在用”，不能由任意一个服务单方面销毁。

预期效果：

- 降低 TCP/Web 并行运行时的日志悬空访问风险

### 补充修复点 19：Web OnBody() 不再按分片自动回响应

涉及文件：

- `WebServerListerNet.h`

修改前的问题：

- `OnBody()` 每收到一次 body 回调就立即 `SendResponse(200, body)`。
- 如果 HTTP Body 被拆成多次回调或使用 chunked 传输，会出现重复响应或半截响应。

本次修改内容：

- 去掉 `OnBody()` 中按分片自动回包的逻辑，仅保留解析通过返回。

为什么这样修改：

- 当前 Web 模块主职责是 WebSocket 服务，不应在未完整组装 HTTP 请求体前就按分片直接回应。

预期效果：

- 避免普通 HTTP Body 分片场景下的错误响应行为

### 补充修复点 20：CreateWssSock() 明确返回未实现

涉及文件：

- `WebSockServerObj.cpp`

修改前的问题：

- `CreateWssSock()` 没有实际实现，但直接返回 `true`。
- 上层可能误判 WSS 已启动成功。

本次修改内容：

- `CreateWssSock()` 改为明确返回 `false`。
- 如果外部提供了错误缓冲，同时写入 `wss not implement` 错误描述。

为什么这样修改：

- 未实现的接口必须明确失败，不能用成功返回值掩盖真实状态。

预期效果：

- 避免上层把未实现能力当成可用能力继续使用

## 2026-03-30 补充：TCP / Web 日志彻底拆分为两个独立单例

### 补充修复点 21：共享日志改为 TCP / Web 两套独立日志单例

涉及文件：

- `Log.h`
- `Log.cpp`
- `TcpSockServerObj.cpp`
- `TcpServerListerNet.cpp`
- `WebSockServerObj.cpp`
- `WebServerListerNet.cpp`

修改前的问题：

- TCP 和 Web 原先共用同一个 `CLog` 单例。
- 即使后面通过“谁在持有日志”的方式协调释放，也仍然解决不了两个服务日志路径不同的问题。
- 后初始化的服务会覆盖前一个服务的日志路径，最终两边日志仍会混到同一个单例里。

本次修改内容：

- 保留原 `CLog` 兼容接口。
- 新增 `CTcpLog` 和 `CWebLog` 两个独立日志单例类。
- 为 TCP 新增 `TCP_INFO / TCP_WARN / TCP_ERROR / TCP_DEBUG` 宏。
- 为 Web 新增 `WEB_INFO / WEB_WARN / WEB_ERROR / WEB_DEBUG` 宏。
- TCP 服务初始化、释放、运行日志统一改为走 `CTcpLog`。
- Web 服务初始化、释放、运行日志统一改为走 `CWebLog`。
- 删除上一版为共享单例增加的“TCP / Web 日志持有标记”协调逻辑。

字段备注：

- `CTcpLog::m_pThis`：TCP 日志单例实例指针，仅服务于 TCP 模块。
- `CWebLog::m_pThis`：Web 日志单例实例指针，仅服务于 Web 模块。

为什么这样修改：

- TCP 和 Web 的日志目录、生命周期、启停节奏本来就是两套独立资源。
- 既然路径不同，最稳妥的方案就是从单例层直接拆开，而不是继续在一个共享单例上叠补丁。

预期效果：

- TCP 和 Web 可以分别写入各自日志目录
- 两个服务互不覆盖日志路径
- 两个服务互不影响日志实例释放

### 补充修复点 22：删除模块名字全局变量，模块名前缀收进 TCP / Web 日志宏

涉及文件：

- `Log.h`
- `publicGlobalvar.h`
- `publicGlobalvar.cpp`
- `TcpSockServerObj.cpp`
- `TcpServerListerNet.cpp`
- `WebSockServerObj.cpp`
- `WebServerListerNet.cpp`

修改前的问题：

- TCP 和 Web 日志已经拆成两套独立单例后，代码里仍然保留 `g_strServerName`、`g_strWebServerName` 这类全局模块名字。
- 每次打日志都还要手工把模块名字作为 `%s` 传进来，存在重复样板代码。

本次修改内容：

- 删除 `g_strServerName`、`g_strWebServerName` 全局变量声明和定义。
- 将 `[TCP服务]` 前缀直接收进 `TCP_INFO / TCP_WARN / TCP_ERROR / TCP_DEBUG` 宏。
- 将 `[WEB服务]` 前缀直接收进 `WEB_INFO / WEB_WARN / WEB_ERROR / WEB_DEBUG` 宏。
- TCP / Web 相关日志调用全部改成直接输出业务内容，不再额外手工传模块名参数。

为什么这样修改：

- TCP 和 Web 的日志实例已经分离，模块标识最适合直接固化在各自日志宏中。
- 这样可以减少重复参数传递，也避免后面新增日志时忘记带模块名。

预期效果：

- TCP / Web 日志格式更统一
- 代码里的日志调用更简洁
- 删除无意义的模块名字全局状态

## 2026-03-30 补充：接口说明与边界路径安全修正

### 补充修复点 23：修正 ConnID 日志输出、Accept 参数说明与 Web Pull 收包边界

涉及文件：

- `TcpServerListerNet.cpp`
- `TcpSockServerObj.cpp`
- `WebServerListerNet.cpp`
- `WebSockServerObj.cpp`
- `SocketServer.h`

修改前的问题：

- `CONNID` 在 HP-Socket 中是指针宽度整数，64 位下继续用不匹配的格式串输出，存在日志错乱和可变参数未定义行为风险。
- `p_uiMaxAcceptNum` 在接口注释里一直写成“同一IP最大连接数”，但内部实际传给的是 `SetAcceptSocketCount()`，语义是底层 Accept 预分配数量，容易误导上层调用方。
- Web 服务的原始 `OnReceive(ITcpServer*, CONNID, int)` Pull 收包回调没有真正取数据，旧实现一旦进入会错误分配缓存、覆盖状态，属于危险的兜底路径。

本次修改内容：

- 将本轮涉及 `ConnID` 的关键日志统一按 64 位安全格式输出，并显式转换为 `unsigned long long`。
- 将 `SocketServer.h` 中 TCP / HTTP / WebSocket 三处 `p_uiMaxAcceptNum` 注释统一改为真实语义说明。
- 保留 TCP / Web 创建流程中对 `SetAcceptSocketCount()` 的中文注释，明确说明这不是“同一 IP 最大连接数”限流。
- 将 Web 原始 Pull 收包回调改成安全拒绝路径：记录告警后直接返回错误，不再继续分配错误缓存和推进错误状态。

为什么这样修改：

- 连接 ID 属于跨 32/64 位都可能变化的底层句柄，日志格式必须与真实宽度匹配，否则在高并发定位问题时会把关键信息打印错，甚至引入未定义行为。
- 对外接口说明如果和内部真实行为不一致，后续使用方会在容量规划、限流判断上做出错误假设，这比单纯的代码 bug 更隐蔽。
- 对当前 Web 实现来说，HTTP 解析与 WebSocket 回调才是主收包链路；Pull 回调既然不应该进入，就应该显式拒绝，而不是留下一个实现错误的备用路径。

预期效果：

- TCP / Web 关键连接日志在 64 位环境下输出稳定可靠
- `p_uiMaxAcceptNum` 的接口语义与实现保持一致
- Web 服务不会再因为误入 Pull 收包回调而出现错误缓存分配和后续状态污染
