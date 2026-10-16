#pragma once

// 本文件定义 MtLoadEngine 对外稳定 C ABI 使用的数据类型。
// 仅包含定长 POD，不暴露 STL、MFC 或第三方库类型；所有字符串均为 UTF-8。

#include <stdint.h>

#define MTLOAD_ABI_VERSION 4U       // 当前公共结构和导出函数遵循的 ABI 版本号。
#define MTLOAD_TEXT_SMALL 64U       // 标识符、步骤名等短 UTF-8 文本的固定容量。
#define MTLOAD_TEXT_MEDIUM 256U     // URL、标签等中等 UTF-8 文本的固定容量。

typedef void* MTLOAD_HANDLE;        // 不透明引擎句柄，仅允许交给 MtLoad_* 接口使用。

// 公共接口返回码。0 表示成功，其余值按参数、配置、运行、报告和内部错误分段。
typedef enum MTLOAD_RESULT
{
    MTLOAD_OK = 0,                       // 操作成功。
    MTLOAD_E_INVALID_ARGUMENT = 1001,   // 指针、长度、格式或取值不合法。
    MTLOAD_E_ABI_MISMATCH = 1002,       // 调用方结构大小或 ABI 版本不匹配。
    MTLOAD_E_INVALID_STATE = 1003,      // 当前状态不允许执行该操作。
    MTLOAD_E_CONFIG_IO = 1101,          // 配置文件读取或关联文件访问失败。
    MTLOAD_E_CONFIG_SYNTAX = 1102,      // XML、CSV 或字段语法无法解析。
    MTLOAD_E_CONFIG_SEMANTIC = 1103,    // ID、引用、范围或请求总数校验失败。
    MTLOAD_E_UNSUPPORTED = 1104,        // 配置使用了当前版本未实现的能力。
    MTLOAD_E_START_FAILED = 1201,       // 工作线程或运行资源启动失败。
    MTLOAD_E_TIMEOUT = 1202,            // 等待超过调用方指定时限。
    MTLOAD_E_BUFFER_TOO_SMALL = 1203,   // 调用方提供的数组或文本缓冲区不足。
    MTLOAD_E_NOT_FOUND = 1204,          // 指定环境、场景、用户或其他 ID 不存在。
    MTLOAD_E_REPORT = 1301,             // 报告创建、序列化或写入失败。
    MTLOAD_E_THRESHOLD_FAILED = 1302,   // 运行已完成，但一个或多个验收阈值未通过。
    MTLOAD_E_INTERNAL = 1999            // 未归类的内部异常或执行错误。
} MTLOAD_RESULT;

// 引擎状态机。控制端依据该值决定按钮状态，不得自行推断运行阶段。
typedef enum MTLOAD_STATE
{
    MTLOAD_STATE_CREATED = 0,       // 实例已创建，尚未载入计划。
    MTLOAD_STATE_PLAN_LOADED = 1,   // XML 已解析，尚未完成语义校验。
    MTLOAD_STATE_VALIDATED = 2,     // 计划校验通过，可以启动。
    MTLOAD_STATE_STARTING = 3,      // 正在建立本轮运行上下文和工作线程。
    MTLOAD_STATE_RUNNING = 4,       // 虚拟用户正在执行场景。
    MTLOAD_STATE_STOPPING = 5,      // 已请求停止，正在等待在途任务收敛。
    MTLOAD_STATE_COMPLETED = 6,     // 本轮正常完成或按停止请求完成收敛。
    MTLOAD_STATE_FAILED = 7,        // 本轮因不可恢复错误结束。
    MTLOAD_STATE_PAUSING = 8,       // 已请求暂停，等待调度到达安全边界。
    MTLOAD_STATE_PAUSED = 9         // 已暂停，不再领取新请求。
} MTLOAD_STATE;

// 报告格式位掩码。单文件接口只接受单一值，目录导出接口允许按位组合。
typedef enum MTLOAD_REPORT_FORMAT
{
    MTLOAD_REPORT_JSON = 1,         // UTF-8 JSON 报告。
    MTLOAD_REPORT_CSV = 2,          // UTF-8 CSV 报告。
    MTLOAD_REPORT_XLSX = 4,         // Office Open XML 工作簿。
    MTLOAD_REPORT_XML = 8,          // 支持中文内容的 UTF-8 XML 报告。
    MTLOAD_REPORT_ALL = 15          // 同时导出当前版本支持的全部格式。
} MTLOAD_REPORT_FORMAT;

// 创建引擎实例时的固定布局选项；reserved 必须保持为 0 以便后续扩展。
typedef struct MTLOAD_ENGINE_OPTIONS
{
    uint32_t structSize;           // 调用方填写 sizeof(MTLOAD_ENGINE_OPTIONS)。
    uint32_t abiVersion;           // 调用方填写 MTLOAD_ABI_VERSION。
    uint32_t maxConnections;       // 实例允许的并发网络请求上限，单位：个。
    uint32_t eventQueueCapacity;   // 预留给异步完成队列，单位：条。
    uint32_t snapshotIntervalMs;   // 指标快照建议刷新周期，单位：毫秒。
    uint32_t maxResponseBytes;     // 单个 HTTP 响应正文上限，单位：字节。
    uint64_t reserved[8];          // 保留字段，当前版本必须全部为 0。
} MTLOAD_ENGINE_OPTIONS;

// 单次运行的轻量状态快照；所有计数均在调用时复制，不借用引擎内存。
typedef struct MTLOAD_RUN_STATE
{
    uint32_t structSize;           // 调用方填写本结构大小。
    uint32_t abiVersion;           // 调用方填写当前 ABI 版本。
    uint64_t runId;                // 每次成功启动递增的运行编号。
    int32_t state;                 // MTLOAD_STATE 状态值。
    int32_t lastResult;            // 最近一次生命周期操作或终态结果码。
    int64_t startUnixMs;           // 本轮开始的 Unix 毫秒时间戳，未启动时为 0。
    int64_t endUnixMs;             // 本轮结束的 Unix 毫秒时间戳，运行中为 0。
    uint32_t activeScenarios;      // 当前仍在执行的场景任务数。
    uint32_t activeUsers;          // 当前仍在执行的虚拟用户数。
    uint32_t activeRequests;       // 已发出但尚未完成的逻辑请求数。
    uint32_t activeConnections;    // 当前占用的网络并发许可数。
    uint64_t queuedRequests;       // 预留的排队请求统计，当前实现固定为 0。
    uint64_t totalSamples;         // 已完成请求样本总数，成功和失败均计入。
    uint64_t plannedRequests;      // 有限迭代模式的计划请求总数；0 表示持续时间模式，无法预知。
    uint64_t effectiveElapsedMs;   // 扣除暂停时间后的有效运行时间，单位：毫秒。
    double totalThroughputPerSec;  // 本轮全部请求的全程平均吞吐，单位：req/s。
    uint32_t peakActiveUsers;      // 本轮同时执行事务的虚拟用户峰值。
    uint32_t peakActiveRequests;   // 本轮同时在途逻辑请求峰值。
    uint64_t scheduledIterations;  // 负载曲线要求启动的业务迭代数，包含被丢弃迭代。
    uint64_t completedIterations;  // 已完整执行普通步骤和 alwaysRun 清理步骤的业务迭代数。
    uint64_t droppedIterations;    // 开放模型达到 maxUsers 后未能准时启动的业务迭代数。
    uint64_t reserved[1];          // 保留字段，调用方不得解释。
} MTLOAD_RUN_STATE;

// 单个“场景+步骤+请求”维度的累计指标，字节和耗时均来自同批完成样本。
typedef struct MTLOAD_METRIC_ROW
{
    uint32_t structSize;                    // 本结构的字节大小。
    uint32_t abiVersion;                    // 生成快照的 ABI 版本。
    char scenario[MTLOAD_TEXT_SMALL];       // 场景显示名称，UTF-8。
    char step[MTLOAD_TEXT_SMALL];           // 步骤显示名称，UTF-8。
    char label[MTLOAD_TEXT_MEDIUM];         // 请求标签，UTF-8。
    char method[16];                        // HTTP 方法，例如 POST。
    char path[MTLOAD_TEXT_MEDIUM];          // 配置中的请求路径，不包含密码等变量值。
    uint64_t samples;                       // 已完成样本数。
    uint64_t succeeded;                     // 通过传输、状态和断言校验的样本数。
    uint64_t failed;                        // 传输、状态、断言或提取失败的样本数。
    uint64_t sentBytes;                     // 累计实际请求正文大小，单位：字节。
    uint64_t receivedBytes;                 // 累计实际响应正文大小，单位：字节。
    double errorRate;                       // 失败数除以样本数，范围 0.0 至 1.0。
    double minMs;                           // 最短端到端耗时，单位：毫秒。
    double maxMs;                           // 最长端到端耗时，单位：毫秒。
    double averageMs;                       // 算术平均耗时，单位：毫秒。
    double medianMs;                        // P50 中位耗时，单位：毫秒。
    double p90Ms;                           // 第 90 百分位耗时，单位：毫秒。
    double p95Ms;                           // 第 95 百分位耗时，单位：毫秒。
    double p99Ms;                           // 第 99 百分位耗时，单位：毫秒。
    double throughputPerSec;                // 从本轮有效运行时间计算的每秒请求数。
    double sentKiBPerSec;                   // 每秒发送正文大小，按 1024 字节换算 KiB。
    double receivedKiBPerSec;               // 每秒接收正文大小，按 1024 字节换算 KiB。
    uint64_t plannedRequests;      // 当前接口的计划请求数；0 表示无法预知。
    uint64_t reserved[3];                   // 保留字段，调用方不得解释。
} MTLOAD_METRIC_ROW;

// 失败请求的有限样本，用于定位问题而不是替代完整业务日志。
typedef struct MTLOAD_ERROR_SAMPLE
{
    uint32_t structSize;                    // 本结构的字节大小。
    uint32_t abiVersion;                    // 生成样本的 ABI 版本。
    int64_t timestampUnixMs;                // 失败发生的 Unix 毫秒时间戳。
    uint64_t runId;                         // 失败所属运行编号。
    int32_t result;                         // MTLOAD_RESULT 错误码。
    int32_t httpStatus;                     // 已收到响应时的 HTTP 状态码，否则为 0。
    double elapsedMs;                       // 失败前已消耗的端到端毫秒数。
    char scenario[MTLOAD_TEXT_SMALL];       // 场景名称，UTF-8。
    char step[MTLOAD_TEXT_SMALL];           // 步骤名称，UTF-8。
    char label[MTLOAD_TEXT_MEDIUM];         // 请求标签，UTF-8。
    char category[MTLOAD_TEXT_SMALL];       // 错误类别，例如 transport 或 assertion。
    char summary[512];                      // 已脱敏的简要诊断，UTF-8。
    uint64_t reserved[4];                   // 保留字段，调用方不得解释。
} MTLOAD_ERROR_SAMPLE;

// XML 中可选择的运行环境。字符串均为 UTF-8；selected 为 1 表示当前环境。
typedef struct MTLOAD_ENVIRONMENT
{
    uint32_t structSize;                    // 本结构的字节大小。
    uint32_t abiVersion;                    // 生成摘要的 ABI 版本。
    char id[MTLOAD_TEXT_SMALL];             // XML Environment.id，UTF-8。
    char name[MTLOAD_TEXT_SMALL];           // 界面显示名称，UTF-8。
    char scheme[16];                        // 连接协议：http 或 https。
    char host[MTLOAD_TEXT_MEDIUM];          // 域名或 IP，不包含 scheme 和路径。
    char basePath[MTLOAD_TEXT_MEDIUM];      // 统一 URL 前缀，可为空。
    uint32_t port;                          // 显式端口；配置省略时为协议默认端口。
    uint32_t selected;                      // 当前选择标记，1 为选中、0 为未选中。
    uint64_t reserved[4];                   // 保留字段，调用方不得解释。
} MTLOAD_ENVIRONMENT;

// XML 中的场景摘要。name 使用 UTF-8，可直接保存中文；selected 为 1 表示当前只运行该场景。
typedef struct MTLOAD_SCENARIO
{
    uint32_t structSize;                    // 本结构的字节大小。
    uint32_t abiVersion;                    // 生成摘要的 ABI 版本。
    char id[MTLOAD_TEXT_SMALL];             // XML Scenario.id，UTF-8。
    char name[MTLOAD_TEXT_MEDIUM];          // 支持中文的场景显示名称，UTF-8。
    uint32_t threads;                       // 并发虚拟用户数。
    uint32_t iterations;                    // 每个虚拟用户执行场景的循环次数。
    uint64_t requestsPerIteration;          // 单个用户完成一次场景循环的请求数。
    uint64_t plannedRequests;               // threads * iterations * 单循环请求数。
    uint32_t selected;                      // 当前选择标记，1 为选中、0 为未选中。
    uint32_t reserved32;                    // 32 位对齐保留字段，当前必须为 0。
    uint64_t reserved[4];                   // 保留字段，调用方不得解释。
} MTLOAD_SCENARIO;

// 可复用工作负载摘要。工作负载引用场景并定义闭合并发或开放到达率曲线。
typedef struct MTLOAD_WORKLOAD
{
    uint32_t structSize;                    // 本结构的字节大小。
    uint32_t abiVersion;                    // 生成摘要的 ABI 版本。
    char id[MTLOAD_TEXT_SMALL];             // XML Workload.id；旧配置会生成 legacy-* 隐式 ID。
    char name[MTLOAD_TEXT_MEDIUM];          // 支持中文的工作负载显示名称，UTF-8。
    char mode[16];                          // closed 或 arrival。
    char testType[32];                      // smoke、average、stress、spike、breakpoint 或 soak。
    uint32_t scenarioCount;                 // 引用的业务场景数量。
    uint32_t users;                         // closed 并发用户数，arrival 为预分配用户数。
    uint32_t maxUsers;                      // arrival 最大执行用户数；closed 与 users 相同。
    uint32_t stageCount;                    // 到达率阶段数量；固定速率时为 0。
    uint64_t durationMs;                    // 计划持续时间；有限迭代 closed 模式可为 0。
    double ratePerSec;                      // arrival 初始业务迭代到达率，单位：次/秒。
    uint32_t selected;                      // 当前选择标记，1 为选中、0 为未选中。
    uint32_t implicit;                      // 由旧 Scenario.Load 兼容生成时为 1。
    uint64_t reserved[4];                   // 保留字段，调用方不得解释。
} MTLOAD_WORKLOAD;

// 场景事务维度累计指标。一次事务包含普通步骤及其 alwaysRun 清理步骤。
typedef struct MTLOAD_SCENARIO_METRIC_ROW
{
    uint32_t structSize;                    // 本结构的字节大小。
    uint32_t abiVersion;                    // 生成快照的 ABI 版本。
    char scenarioId[MTLOAD_TEXT_SMALL];     // XML Scenario.id，UTF-8。
    char scenario[MTLOAD_TEXT_MEDIUM];      // 场景显示名称，UTF-8。
    uint64_t started;                       // 已进入场景执行的事务数。
    uint64_t completed;                     // 已运行到事务终点的事务数。
    uint64_t succeeded;                     // 所有普通及清理步骤均成功的事务数。
    uint64_t failed;                        // 任一步骤失败的事务数。
    uint64_t dropped;                       // 开放模型中未获得执行用户的事务数。
    double minMs;                           // 最短完整事务耗时，单位：毫秒。
    double maxMs;                           // 最长完整事务耗时，单位：毫秒。
    double averageMs;                       // 完整事务平均耗时，单位：毫秒。
    double medianMs;                        // 完整事务 P50，单位：毫秒。
    double p90Ms;                           // 完整事务 P90，单位：毫秒。
    double p95Ms;                           // 完整事务 P95，单位：毫秒。
    double p99Ms;                           // 完整事务 P99，单位：毫秒。
    double throughputPerSec;                // 完整事务全程平均吞吐，单位：次/秒。
    uint64_t reserved[4];                   // 保留字段，调用方不得解释。
} MTLOAD_SCENARIO_METRIC_ROW;

// 单条阈值判定结果。运行完成后可查询，并随 JSON、XML、XLSX 和日志输出。
typedef struct MTLOAD_THRESHOLD_RESULT
{
    uint32_t structSize;                    // 本结构的字节大小。
    uint32_t abiVersion;                    // 生成结果的 ABI 版本。
    char id[MTLOAD_TEXT_SMALL];             // XML Threshold.id，UTF-8。
    char name[MTLOAD_TEXT_MEDIUM];          // 阈值显示名称，UTF-8。
    char scope[16];                         // run、request 或 scenario。
    char metric[32];                        // errorRate、p95Ms、p99Ms、throughputPerSec 或 droppedIterations。
    char target[MTLOAD_TEXT_MEDIUM];        // 请求标签或 Scenario.id；运行级阈值为空。
    char comparison[8];                     // lte、gte 或 eq。
    double expected;                        // 配置的阈值。
    double actual;                          // 本轮实际值。
    uint32_t passed;                        // 1 表示通过，0 表示失败。
    uint32_t reserved32;                    // 32 位对齐保留字段。
    uint64_t reserved[4];                   // 保留字段，调用方不得解释。
} MTLOAD_THRESHOLD_RESULT;

// XML 中可由控制端选择的用户摘要。password 和扩展参数不会通过 ABI 暴露。
// selected 为 1 表示本轮所有场景都使用该用户；全部为 0 表示按 Scenario.UserRefs 分配。
typedef struct MTLOAD_USER
{
    uint32_t structSize;                    // 本结构的字节大小。
    uint32_t abiVersion;                    // 生成摘要的 ABI 版本。
    char id[MTLOAD_TEXT_SMALL];             // XML User.id，UTF-8。
    char name[MTLOAD_TEXT_MEDIUM];          // 账号显示名称，UTF-8。
    char version[16];                       // MT 主版本变量，例如 4 或 5。
    char no[16];                            // 节点编号变量。
    char login[MTLOAD_TEXT_SMALL];          // 登录账号，仅用于选择摘要。
    uint32_t selected;                      // 当前选择标记，1 为选中、0 为未选中。
    uint32_t reserved32;                    // 32 位对齐保留字段，当前必须为 0。
    uint64_t reserved[4];                   // 保留字段，调用方不得解释。
} MTLOAD_USER;

