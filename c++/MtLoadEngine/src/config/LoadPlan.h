#pragma once

// 压测计划的内部不可变模型。解析和校验结束后由运行线程只读共享。

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace mtload
{
// 单个 HTTP 请求头模板；值允许包含运行时变量占位符。
struct Header
{
    std::string name;       // 请求头名称，按配置原样交给传输层。
    std::string value;      // UTF-8 值模板，在虚拟用户上下文中展开。
};

// 请求目标定义；对象在计划校验后只读共享，不拥有任何网络句柄。
struct Target
{
    std::string id;                         // XML Target.id，供 Request.targetId 引用。
    std::string transport;                  // 传输类型；当前已实现 http 和 https。
    std::string host;                       // 默认域名或 IP，可被所选环境覆盖。
    uint16_t port = 0;                      // 显式端口；0 表示由协议选择默认端口。
    std::string basePath;                   // 拼接在请求路径之前的统一前缀。
    std::string tlsCaFile;                  // HTTPS 服务端证书校验使用的 PEM 证书链全路径。
    bool tlsVerify = true;                  // true 时校验服务端证书链，默认严格校验。
    uint32_t connectTimeoutMs = 3000;       // 建立连接超时，单位：毫秒。
    uint32_t requestTimeoutMs = 5000;       // 完整请求超时，单位：毫秒。
    uint32_t maxConnections = 100;          // 目标建议并发上限，单位：连接数。
};

// 一套可由控制端选择的运行环境。当前阶段同一轮压测中的 HTTP Target
// 共用该入口地址；basePath 用于区分直连服务和反向代理路径。
struct Environment
{
    std::string id;             // XML Environment.id，作为界面选择和默认值引用键。
    std::string name;           // 支持中文的环境显示名称。
    std::string scheme;         // 环境连接协议：http 或 https，缺省为 http。
    std::string host;           // 环境域名或 IP，不包含 scheme、端口和路径。
    uint16_t port = 0;          // 可选显式端口；0 表示沿用 Target 或协议默认值。
    std::string basePath;       // 环境级 URL 前缀，可为空。
    std::string tlsCaFile;      // HTTPS 环境使用的 PEM 证书链全路径；HTTP 环境应为空。
    bool tlsVerify = true;      // 是否校验服务端证书链；仅用于诊断的环境可显式关闭。
};

// 编号用户是虚拟用户线程的变量来源。标准属性无论是否填写都会映射为请求模板中的
// ${UserId}/${Version}/${No}/${Login}/${Password}；空属性保留为空字符串，扩展参数也进入 variables。
// 本模型不判断这些值是否符合具体业务接口契约。
struct UserProfile
{
    std::string id;                                 // XML User.id，供场景 UserRef 引用。
    std::string name;                               // 界面显示名称，不参与请求匹配。
    std::map<std::string, std::string> variables;   // 标准账号字段及 Param 扩展变量。
};

// 请求完成后的断言定义；HTTP 状态断言和 JSON 断言共用该结构。
struct Assertion
{
    enum class Kind
    {
        HttpStatus,     // 校验响应状态码范围。
        Json            // 校验响应 JSON 指定路径。
    } kind = Kind::HttpStatus;                      // 当前断言种类。
    int minStatus = 200;                            // HTTP 状态断言允许的最小值。
    int maxStatus = 299;                            // HTTP 状态断言允许的最大值。
    std::string path;                               // JSON 路径，仅 Json 断言使用。
    std::string op;                                 // 比较操作，例如 equals 或 exists。
    std::string value;                              // 期望值，按字符串语义比较。
};

// 响应变量提取规则；提取结果写入当前虚拟用户会话，不跨线程共享。
struct Extractor
{
    std::string path;               // 响应 JSON 路径。
    std::string variable;           // 写入 VariableContext 的变量名。
    std::string scope;              // 变量作用域；当前支持 user。
    bool required = true;           // true 表示路径缺失时该请求判定失败。
};

// 单个逻辑请求模板；copies 表示同一步骤中并行执行的副本数。
struct Request
{
    std::string id;                             // XML Request.id，在场景内唯一。
    std::string label;                          // 指标表和报告使用的中文显示名。
    std::string targetId;                       // 被调用 Target.id。
    std::string method;                         // HTTP 方法，例如 POST。
    std::string path;                           // 可包含变量的相对请求路径。
    std::string codec;                          // 报文编解码类型；当前支持 json。
    uint32_t copies = 1;                        // 同一步骤并行请求副本数。
    std::string body;                           // UTF-8 请求正文模板。
    std::vector<Header> headers;                // 请求级头部，覆盖或补充全局头部。
    std::vector<Assertion> assertions;          // 传输完成后依次执行的断言。
    std::vector<Extractor> extractors;          // 断言通过后执行的会话变量提取规则。
};

// 场景中的顺序执行单元；步骤之间串行，步骤内部 Request/copies 可并行。
struct Step
{
    std::string id;                             // XML Step.id，在场景内唯一。
    std::string name;                           // 指标和界面使用的步骤名称。
    std::string onError = "stop_iteration";    // 请求失败后的迭代控制策略。
    // 清理步骤即使前置业务步骤失败也必须执行，用于删除本轮生成的隔离测试数据。
    bool alwaysRun = false;
    std::vector<Request> requests;              // 本步骤需要执行的请求模板。
};

// 开放负载阶段。每段从上一目标速率线性变化到本段 targetRatePerSec。
struct LoadStage
{
    uint32_t durationMs = 0;            // 阶段持续时间，单位：毫秒。
    double targetRatePerSec = 0.0;      // 阶段结束时目标业务迭代到达率。
};

// 工作负载参数。closed 以虚拟用户闭环驱动，arrival 以业务迭代到达率驱动。
struct Load
{
    std::string mode = "closed";        // closed 或 arrival。
    uint32_t threads = 1;               // closed 并发虚拟用户数量。
    uint32_t rampUpMs = 0;              // closed 全部用户启动总时间，单位：毫秒。
    uint32_t durationMs = 0;            // 固定负载持续时间；0 表示使用迭代数或阶段总时长。
    uint32_t iterations = 1;            // closed 每用户完整场景循环次数。
    double ratePerSec = 0.0;            // arrival 固定初始业务迭代到达率。
    uint32_t preAllocatedUsers = 1;      // arrival 启动时预建的工作用户数。
    uint32_t maxUsers = 1;               // arrival 允许按需增长到的最大工作用户数。
    std::vector<LoadStage> stages;       // arrival 阶段曲线；为空时使用固定 ratePerSec。
};

// 完整压测场景；运行阶段只读共享，用户变量上下文由每个线程单独创建。
struct Scenario
{
    std::string id;                         // XML Scenario.id，全计划唯一。
    std::string name;                       // 支持中文的场景显示名称。
    std::string dataSetId;                  // 可选 DataSet.id；空表示不使用数据行。
    std::vector<std::string> userIds;       // 按虚拟用户索引循环分配的账号 ID。
    Load load;                              // 本场景的线程、爬坡、时长和循环参数。
    std::vector<Step> steps;                // 按配置顺序执行的场景步骤。
};

// 工作负载中的场景引用。权重表示业务事务比例，不表示 HTTP 请求比例。
struct WorkloadScenarioRef
{
    std::string scenarioId;              // 被引用的 Scenario.id。
    uint32_t weight = 1;                 // 确定性轮询权重，必须大于 0。
};

// 运行结束后的自动验收阈值定义。
struct Threshold
{
    std::string id;                      // 全工作负载唯一 ID。
    std::string name;                    // 报告和界面显示名称。
    std::string scope = "run";          // run、request 或 scenario。
    std::string metric;                  // errorRate、p95Ms、p99Ms、throughputPerSec 或 droppedIterations。
    std::string target;                  // 请求 label 或 Scenario.id；运行级为空。
    std::string comparison = "lte";     // lte、gte 或 eq。
    double value = 0.0;                  // 期望阈值。
};

// 场景和负载曲线的独立组合。旧 Scenario.Load 会生成 implicit 工作负载。
struct Workload
{
    std::string id;                              // Workload.id，全计划唯一。
    std::string name;                            // 支持中文的显示名称。
    std::string testType = "average";           // smoke/average/stress/spike/breakpoint/soak。
    Load load;                                   // 工作负载自己的负载模型。
    std::vector<WorkloadScenarioRef> scenarios;  // 按 XML 顺序保存场景权重。
    std::vector<Threshold> thresholds;           // 运行结束时统一评估。
    bool implicit = false;                       // 是否由旧 Scenario.Load 自动生成。
    bool useScenarioLoads = false;               // 兼容“全部场景”时分别沿用各场景 Load。
};

// 数据驱动参数集；CSV 或内联 Row 在加载阶段统一转换为 rows。
struct DataSet
{
    std::string id;                                         // XML DataSet.id。
    std::string path;                                       // CSV 相对或绝对路径；内联数据时为空。
    std::string rowMode = "circular";                       // 行分配策略；当前支持 circular。
    std::vector<std::map<std::string, std::string>> rows;   // 已解析的列名和值集合。
};

// 一份 XML 计划的完整内存模型。成功载入后由 LoadEngine 持有。
struct Plan
{
    std::string sourcePath;                                 // XML 的绝对或调用方传入路径。
    std::string name;                                       // 计划显示名称。
    // 报告目录可为空、相对或绝对；为空时使用 sourcePath 所在目录。
    std::string reportDirectory;
    std::string defaultEnvironmentId;                       // 校验成功后默认选中的环境 ID。
    std::map<std::string, Environment> environments;        // 按 ID 索引的可选环境。
    std::map<std::string, UserProfile> users;               // 按 ID 索引的账号参数。
    std::map<std::string, Target> targets;                   // 按 ID 索引的服务目标。
    std::map<std::string, DataSet> dataSets;                 // 按 ID 索引的数据集。
    std::vector<Header> headers;                             // 应用于全部请求的全局请求头。
    std::vector<Scenario> scenarios;                        // 按 XML 顺序保存的压测场景。
    std::string defaultWorkloadId;                          // 默认工作负载 ID。
    std::vector<Workload> workloads;                        // 独立可选择工作负载及旧配置兼容项。
};
}
