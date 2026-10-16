#pragma once

// 单个 C ABI 句柄对应一个 LoadEngine。对象拥有计划、运行线程、指标和错误样本。
// 生命周期操作由 stateMutex_ 串行化；网络热路径只访问独立指标锁和原子计数。

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "../../include/MtLoadTypes.h"
#include "../config/LoadPlan.h"

namespace mtload
{
// 进程内并发许可池，限制同时进入网络交换的请求数。
// Acquire/Release 可由工作线程并发调用；对象由 LoadEngine 独占并覆盖整段实例生命周期。
class PermitPool
{
public:
    // 以至少 1 个许可初始化固定容量池。
    explicit PermitPool(uint32_t capacity);

    // 等待一个许可；停止标记置位时立即放弃并返回 false。
    bool Acquire(const std::atomic<bool>& stopping);

    // 归还一个许可并唤醒一个等待者；必须与成功的 Acquire 成对调用。
    void Release();

    // 唤醒全部等待者以重新检查停止条件，不改变许可数量。
    void WakeAll();

private:
    std::mutex mutex_;                  // 保护 available_ 和条件等待谓词。
    std::condition_variable changed_;   // 许可归还或停止时唤醒等待线程。
    uint32_t available_;                // 当前可领取许可数，受 mutex_ 保护。
};

// 单个压测实例的状态机和资源所有者。
// 公共生命周期操作由 stateMutex_ 串行化；工作线程、计划、指标和错误样本均随对象释放。
// 本类不依赖界面，也不允许在持有内部锁时调用宿主代码。
class LoadEngine final
{
public:
    // 复制实例选项并创建空状态；不启动线程或网络连接。
    explicit LoadEngine(const MTLOAD_ENGINE_OPTIONS& options);

    // 请求停止并等待工作线程退出，保证对象销毁后无残留任务。
    ~LoadEngine();

    // 同步解析指定计划文件；仅非运行状态可调用，成功后进入 PLAN_LOADED。
    int LoadPlanFile(const std::string& path);
    // 校验当前计划并初始化选择项和指标定义；成功后进入 VALIDATED。
    int Validate();
    // 返回环境摘要副本；函数在 stateMutex_ 下读取，可与界面查询并发。
    std::vector<MTLOAD_ENVIRONMENT> GetEnvironments() const;
    // 选择本轮环境；运行期间拒绝修改，不存在的 ID 返回 NOT_FOUND。
    int SelectEnvironment(const std::string& environmentId);
    // 返回场景摘要副本及当前选择标记。
    std::vector<MTLOAD_SCENARIO> GetScenarios() const;
    // 选择唯一场景；空 ID 表示运行全部场景。
    int SelectScenario(const std::string& scenarioId);
    // 返回工作负载摘要；工作负载负责场景组合、负载曲线和验收阈值。
    std::vector<MTLOAD_WORKLOAD> GetWorkloads() const;
    // 选择本轮唯一工作负载；运行期间拒绝修改。
    int SelectWorkload(const std::string& workloadId);
    // 返回不含密码和扩展参数的用户摘要副本。
    std::vector<MTLOAD_USER> GetUsers() const;
    // 选择统一用户；空 ID 恢复按场景 UserRefs 分配。
    int SelectUser(const std::string& userId);
    // 重置本轮状态并异步启动工作线程；失败不会留下可联结线程。
    int Start(const std::string& runName);
    // 在调度边界暂停领取新任务，允许已在途请求自然完成。
    int Pause();
    // 从当前虚拟用户位置继续执行，并累计暂停时长以修正速率统计。
    int Resume();
    // 幂等设置停止标记并唤醒全部等待点，不在调用线程等待网络结束。
    int Stop();
    // 非运行状态下清空指标、错误样本和时间信息，保留已校验计划。
    int ClearResults();
    // 等待完成或失败终态；超时返回 MTLOAD_E_TIMEOUT。
    int Wait(uint32_t timeoutMs);
    // 复制当前状态和原子计数到调用方结构，不暴露内部引用。
    int GetState(MTLOAD_RUN_STATE& out) const;
    // 聚合并返回指标行副本；百分位在副本上计算，不阻塞网络热路径。
    std::vector<MTLOAD_METRIC_ROW> GetMetrics() const;
    // 聚合并返回业务事务指标，和接口请求指标保持独立口径。
    std::vector<MTLOAD_SCENARIO_METRIC_ROW> GetScenarioMetrics() const;
    // 返回本轮完成时评估的阈值结果。
    std::vector<MTLOAD_THRESHOLD_RESULT> GetThresholdResults() const;
    // 返回错误样本副本，调用方可在释放锁后展示。
    std::vector<MTLOAD_ERROR_SAMPLE> GetErrors() const;
    // 生成当前状态和指标的 UTF-8 JSON 快照。
    std::string GetSnapshotJson() const;
    // 返回最近一次引擎级错误文本副本。
    std::string GetLastError() const;
    // 将当前不可变指标快照按格式位掩码导出到目录。
    int Export(const std::string& directory, uint32_t formats);
    // 以调用方指定文件路径导出单一格式。
    int ExportFile(const std::string& absoluteFilePath, uint32_t format);
    // 按计划 Reports.directory 和时间戳命名规则导出单一格式。
    int ExportConfigured(uint32_t format, std::string& outputPath);

private:
    // 指标热路径累加器，仅在 metricsMutex_ 保护下读写。
    struct MetricAccumulator
    {
        std::string scenario;              // 场景显示名称。
        std::string step;                  // 步骤显示名称。
        std::string label;                 // 请求显示标签。
        std::string method;                // HTTP 方法。
        std::string path;                  // 配置路径模板，不含展开后的敏感值。
        uint64_t succeeded = 0;            // 累计成功样本数。
        uint64_t failed = 0;               // 累计失败样本数。
        uint64_t sentBytes = 0;            // 累计请求正文字节数。
        uint64_t receivedBytes = 0;        // 累计响应正文字节数。
        uint64_t plannedRequests = 0;      // 当前指标维度的计划请求数。
        std::vector<double> latencyMs;     // 每个完成样本的毫秒耗时，用于计算分位数。
    };

    // 完整业务事务热路径累加器，耗时覆盖普通步骤和 alwaysRun 清理步骤。
    struct ScenarioAccumulator
    {
        std::string scenarioId;            // 场景稳定 ID。
        std::string scenario;              // 场景显示名称。
        uint64_t started = 0;               // 已进入执行的事务数。
        uint64_t completed = 0;             // 已到达事务终点的事务数。
        uint64_t succeeded = 0;             // 所有步骤均成功的事务数。
        uint64_t failed = 0;                // 至少一步失败的事务数。
        uint64_t dropped = 0;               // arrival 无空闲用户时丢弃的事务数。
        std::vector<double> latencyMs;       // 完整事务耗时样本。
    };

    // 单次请求内部结果；只在线程栈和错误记录阶段传递。
    struct RequestOutcome
    {
        bool success = false;                               // 传输、断言和提取是否全部通过。
        int result = MTLOAD_E_INTERNAL;                     // 对应 MTLOAD_RESULT。
        int httpStatus = 0;                                // 已收到响应时的 HTTP 状态码。
        double elapsedMs = 0.0;                            // 单次请求端到端耗时。
        std::string error;                                 // 已脱敏的失败摘要。
        std::map<std::string, std::string> extracted;       // 成功提取的会话变量。
    };

    // 一次完整场景迭代的终态；未完成通常表示停止请求打断。
    struct IterationOutcome
    {
        bool completed = false;             // 是否执行到包括清理步骤在内的事务终点。
        bool success = false;               // 任一普通或清理请求失败时为 false。
        double elapsedMs = 0.0;              // 完整事务端到端耗时。
    };

    // 工作线程总入口：复制可运行场景、创建场景任务并设置唯一终态。
    void Run();
    // 使用指定 closed 参数创建虚拟用户，支持单场景和确定性混合场景。
    void RunClosedWorkload(const Workload& workload);
    // 按目标到达率调度事务；没有空闲用户时立即记录丢弃，不排队掩盖变慢。
    void RunArrivalWorkload(const Workload& workload);
    // 旧“全部场景”兼容入口，各场景仍使用自身 Scenario.Load 独立执行。
    void RunLegacyAll(const Workload& workload);
    // 为单个旧场景创建虚拟用户线程，并按 rampUpMs 分散启动时间。
    void RunScenario(const Scenario& scenario, const Load& load);
    // 执行某虚拟用户的一次完整场景循环并返回事务终态。
    IterationOutcome RunIteration(const Scenario& scenario, uint32_t userIndex, uint64_t iteration);
    // 展开请求模板、执行传输、断言及提取，返回不共享所有权的结果值。
    RequestOutcome RunRequest(const Scenario& scenario, const Step& step,
        const Request& request, const std::map<std::string, std::string>& dataRow,
        const class VariableContext& variables);
    // 用当前选择环境覆盖目标主机、端口和 basePath，返回独立副本。
    Target ResolveTarget(const Target& configured) const;
    // 在指标锁内追加完成样本和实际字节数。
    void RecordMetric(const Scenario& scenario, const Step& step,
        const Request& request, bool success, double elapsedMs,
        uint64_t sentBytes, uint64_t receivedBytes);
    // 将失败转换为有限、脱敏的错误样本并关联当前 runId。
    void RecordError(const Scenario& scenario, const Step& step,
        const Request& request, const RequestOutcome& outcome);
    // 在同一指标临界区记录事务开始、完成、成败和完整耗时。
    void RecordTransactionStarted(const Scenario& scenario);
    void RecordTransactionCompleted(const Scenario& scenario, const IterationOutcome& outcome);
    // arrival 调度无法分配工作用户时记录该场景丢弃事务。
    void RecordDroppedTransaction(const Scenario& scenario);
    // 运行结束后从不可变指标快照计算当前工作负载的全部阈值。
    bool EvaluateThresholds(const Workload& workload);
    // 在暂停条件变量上等待 RUNNING；停止时返回 false。
    bool WaitUntilRunnable();
    // 完成暂停门禁并增加在途请求计数；并发许可由调用方单独持有。
    bool BeginRequest();
    // 执行可被暂停和停止打断的活动时间延迟，用于 ramp-up。
    bool WaitForActiveDelay(uint64_t delayMs);
    // 减少在途计数并在最后一个请求结束时确认 PAUSED，必须与成功 BeginRequest 成对调用。
    void FinishRequest();
    // 返回已完成和当前进行中的暂停时长，供吞吐率扣除暂停时间。
    uint64_t PausedDurationMs() const;
    // 更新最近错误码和文本；调用方负责保证不会与状态转换冲突。
    void SetError(int result, const std::string& error);
    // 设置唯一终态、结束时间并唤醒等待 MtLoad_Wait 的线程。
    void SetTerminalState(int state, int result);
    // 按选择场景预创建指标行，确保尚未请求的接口也可展示 0 进度。
    void ResetMetricDefinitions(const Plan& plan, const std::string& selectedWorkloadId);
    // 对原子峰值执行无锁 compare_exchange 更新。
    static void UpdatePeak(std::atomic<uint32_t>& peak, uint32_t value);

    MTLOAD_ENGINE_OPTIONS options_{};        // 创建时复制的固定运行选项。
    mutable std::mutex stateMutex_;          // 保护计划、状态、选择项和错误文本。
    mutable std::condition_variable stateChanged_; // 状态改变时唤醒 Wait 调用者。
    Plan plan_;                              // 当前已载入计划，由 stateMutex_ 保护替换。
    bool hasPlan_ = false;                   // 是否已经成功完成 XML 解析。
    bool validated_ = false;                 // 当前 plan_ 是否通过语义校验。
    int state_ = MTLOAD_STATE_CREATED;       // MTLOAD_STATE 状态机当前值。
    int lastResult_ = MTLOAD_OK;             // 最近一次生命周期结果或终态结果。
    std::string lastError_;                  // 最近一次引擎级 UTF-8 诊断。
    std::string runName_;                    // 本轮调用方标签，不参与业务请求。
    std::string selectedEnvironmentId_;      // 本轮使用的环境 ID。
    std::string selectedScenarioId_;         // 空表示全部场景，否则只运行指定 ID。
    std::string selectedWorkloadId_;         // 当前独立工作负载 ID。
    // 为空时使用场景 UserRefs；非空时覆盖所有场景的用户变量来源。
    std::string selectedUserId_;
    uint64_t runId_ = 0;                     // 每次成功 Start 后递增。
    int64_t startUnixMs_ = 0;                // 本轮开始 Unix 毫秒时间戳。
    int64_t endUnixMs_ = 0;                  // 本轮终态 Unix 毫秒时间戳。
    uint64_t plannedRequests_ = 0;           // 所选场景的计划请求总数。
    std::thread worker_;                     // 唯一顶层运行线程，由本对象联结。
    std::atomic<bool> stopping_{ false };     // 所有等待点共享的协作停止标志。
    std::atomic<bool> pauseRequested_{ false }; // 暂停门禁标志，不取消在途请求。
    std::atomic<uint32_t> activeScenarios_{ 0 }; // 正在执行的场景任务数。
    std::atomic<uint32_t> activeUsers_{ 0 };  // 正在执行的虚拟用户线程数。
    std::atomic<uint32_t> activeRequests_{ 0 }; // 已取得许可且尚未 Finish 的请求数。
    std::atomic<uint32_t> peakActiveUsers_{ 0 }; // 本轮同时执行事务用户峰值。
    std::atomic<uint32_t> peakActiveRequests_{ 0 }; // 本轮同时在途请求峰值。
    std::atomic<uint64_t> totalSamples_{ 0 }; // 已完成请求总数。
    std::atomic<uint64_t> scheduledIterations_{ 0 }; // 到达曲线要求启动的事务数。
    std::atomic<uint64_t> completedIterations_{ 0 }; // 已完整结束的事务数。
    std::atomic<uint64_t> droppedIterations_{ 0 }; // 未能准时启动的事务数。
    PermitPool permits_;                     // 全实例网络并发许可池。

    mutable std::mutex pauseMutex_;           // 保护暂停计时和条件谓词。
    std::condition_variable pauseChanged_;    // 继续或停止时唤醒暂停线程。
    std::chrono::steady_clock::time_point pauseStarted_{}; // 当前暂停开始的单调时钟点。
    uint64_t pausedDurationMs_ = 0;           // 已完成暂停区间累计毫秒数。

    mutable std::mutex metricsMutex_;         // 保护 metrics_ 和 errors_ 快照一致性。
    std::map<std::string, MetricAccumulator> metrics_; // 按场景/步骤/请求键聚合指标。
    std::map<std::string, ScenarioAccumulator> scenarioMetrics_; // 按 Scenario.id 聚合事务。
    std::vector<MTLOAD_THRESHOLD_RESULT> thresholdResults_; // 本轮终态阈值明细。
    std::vector<MTLOAD_ERROR_SAMPLE> errors_; // 有界失败样本集合，供宿主诊断。
};
}
