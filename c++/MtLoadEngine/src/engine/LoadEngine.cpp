#include "LoadEngine.h"

#include <json/json.h>
#include <xlnt/xlnt.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <ctime>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <sstream>

#include "../codec/JsonCodec.h"
#include "../config/PlanParser.h"
#include "../runtime/VariableContext.h"
#include "../transport/HttpTransport.h"

namespace mtload
{
namespace
{
// 返回系统时钟的 Unix 毫秒时间戳，用于外部状态和错误样本。
int64_t UnixMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

// 安全复制 UTF-8 文本到固定 ABI 数组，始终保留结尾零字符。
template<size_t N> void CopyText(char (&destination)[N], const std::string& source)
{
    const size_t count = (std::min)(source.size(), N - 1);
    std::memcpy(destination, source.data(), count);
    destination[count] = '\0';
}

// 在排序副本上按线性位置计算分位数，不改变热路径保存的原始样本顺序。
double Percentile(std::vector<double> values, double percentile)
{
    if (values.empty()) return 0.0;
    std::sort(values.begin(), values.end());
    const double position = percentile * static_cast<double>(values.size() - 1);
    const size_t lower = static_cast<size_t>(position);
    const size_t upper = (std::min)(lower + 1, values.size() - 1);
    return values[lower] + (values[upper] - values[lower]) * (position - static_cast<double>(lower));
}

// 使用不可见分隔符组合稳定指标键，避免显示名称简单拼接造成碰撞。
std::string MetricKey(const Scenario& scenario, const Step& step, const Request& request)
{
    return scenario.id + '\x1f' + step.id + '\x1f' + request.id;
}

// 按 CSV 双引号规则转义单元格，确保中文、逗号和换行可被表格工具解析。
std::string CsvEscape(const std::string& value)
{
    if (value.find_first_of(",\"\r\n") == std::string::npos) return value;
    std::string escaped = "\"";
    for (char ch : value) escaped += (ch == '"' ? "\"\"" : std::string(1, ch));
    return escaped + "\"";
}

// 规范化 basePath 与请求路径之间的斜杠，仅处理路径而不改变查询参数。
std::string JoinPath(const std::string& basePath, const std::string& requestPath)
{
    if (basePath.empty()) return requestPath;
    if (requestPath.empty()) return basePath;
    if (basePath.back() == '/' && requestPath.front() == '/')
        return basePath.substr(0, basePath.size() - 1) + requestPath;
    if (basePath.back() != '/' && requestPath.front() != '/')
        return basePath + '/' + requestPath;
    return basePath + requestPath;
}

// 转义 XML 文本中的五个特殊字符，报告内容保持 UTF-8 原文。
std::string XmlEscape(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (const char ch : value)
    {
        switch (ch)
        {
        case '&': escaped += "&amp;"; break;
        case '<': escaped += "&lt;"; break;
        case '>': escaped += "&gt;"; break;
        case '\"': escaped += "&quot;"; break;
        case '\'': escaped += "&apos;"; break;
        default: escaped.push_back(ch); break;
        }
    }
    return escaped;
}

// 生成本地时间报告后缀 yyyyMMdd_HHmmss，确保默认导出名称可排序。
std::string ReportTimestamp()
{
    const std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_s(&local, &now);
    char value[32]{};
    std::strftime(value, sizeof(value), "%Y%m%d_%H%M%S", &local);
    return value;
}

// 把同一运行状态和指标快照序列化为 UTF-8 XML，写入失败通过异常上抛给导出边界。
void WriteXmlReport(const std::filesystem::path& path, const MTLOAD_RUN_STATE& state,
    const std::vector<MTLOAD_METRIC_ROW>& metrics,
    const std::vector<MTLOAD_SCENARIO_METRIC_ROW>& scenarios,
    const std::vector<MTLOAD_THRESHOLD_RESULT>& thresholds,
    const std::vector<MTLOAD_ERROR_SAMPLE>& errors)
{
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) throw std::runtime_error("无法创建 XML 报告");
    stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n"
        << "<压测结果 运行编号=\"" << state.runId << "\" 状态=\"" << state.state
        << "\" 结果码=\"" << state.lastResult
        << "\" 已请求=\"" << state.totalSamples << "\" 总请求=\""
        << state.plannedRequests << "\" 有效运行毫秒=\"" << state.effectiveElapsedMs
        << "\" 总吞吐每秒请求数=\"" << state.totalThroughputPerSec
        << "\" 峰值用户=\"" << state.peakActiveUsers << "\" 峰值在途请求=\""
        << state.peakActiveRequests << "\" 已调度事务=\"" << state.scheduledIterations
        << "\" 已完成事务=\"" << state.completedIterations << "\" 丢弃事务=\""
        << state.droppedIterations << "\">\r\n  <指标列表>\r\n";
    for (const auto& row : metrics)
    {
        const double progress = row.plannedRequests == 0 ? 0.0 :
            static_cast<double>(row.samples) / row.plannedRequests;
        stream << "    <指标 场景=\"" << XmlEscape(row.scenario) << "\" 步骤=\""
            << XmlEscape(row.step) << "\" 接口=\"" << XmlEscape(row.label)
            << "\" 请求方法=\"" << XmlEscape(row.method) << "\" 请求路径=\""
            << XmlEscape(row.path) << "\" 已请求=\"" << row.samples
            << "\" 总请求=\"" << row.plannedRequests << "\" 进度=\"" << progress
            << "\" 成功=\"" << row.succeeded << "\" 失败=\"" << row.failed
            << "\" 错误率=\"" << row.errorRate << "\" 平均耗时毫秒=\""
            << row.averageMs << "\" P95耗时毫秒=\"" << row.p95Ms
            << "\" 吞吐每秒请求数=\"" << row.throughputPerSec << "\" />\r\n";
    }
    stream << "  </指标列表>\r\n  <业务事务列表>\r\n";
    for (const auto& row : scenarios)
    {
        stream << "    <业务事务 场景ID=\"" << XmlEscape(row.scenarioId) << "\" 场景=\""
            << XmlEscape(row.scenario) << "\" 启动=\"" << row.started << "\" 完成=\""
            << row.completed << "\" 成功=\"" << row.succeeded << "\" 失败=\"" << row.failed
            << "\" 丢弃=\"" << row.dropped << "\" 平均耗时毫秒=\"" << row.averageMs
            << "\" P95耗时毫秒=\"" << row.p95Ms << "\" 吞吐每秒事务数=\""
            << row.throughputPerSec << "\" />\r\n";
    }
    stream << "  </业务事务列表>\r\n  <阈值列表>\r\n";
    for (const auto& row : thresholds)
    {
        stream << "    <阈值 ID=\"" << XmlEscape(row.id) << "\" 名称=\"" << XmlEscape(row.name)
            << "\" 范围=\"" << XmlEscape(row.scope) << "\" 指标=\"" << XmlEscape(row.metric)
            << "\" 目标=\"" << XmlEscape(row.target) << "\" 比较=\"" << XmlEscape(row.comparison)
            << "\" 期望=\"" << row.expected << "\" 实际=\"" << row.actual
            << "\" 通过=\"" << row.passed << "\" />\r\n";
    }
    stream << "  </阈值列表>\r\n  <错误列表>\r\n";
    for (const auto& item : errors)
    {
        stream << "    <错误 场景=\"" << XmlEscape(item.scenario) << "\" 步骤=\""
            << XmlEscape(item.step) << "\" 接口=\"" << XmlEscape(item.label)
            << "\" 分类=\"" << XmlEscape(item.category) << "\" 错误码=\""
            << item.result << "\" HTTP状态=\"" << item.httpStatus << "\" 耗时毫秒=\""
            << item.elapsedMs << "\" 摘要=\"" << XmlEscape(item.summary) << "\" />\r\n";
    }
    stream << "  </错误列表>\r\n</压测结果>\r\n";
    if (!stream) throw std::runtime_error("写入 XML 报告失败");
}

// 使用 xlnt 创建指标工作簿，并按固定列顺序写入运行状态和聚合值。
void WriteXlsxReport(const std::filesystem::path& path, const MTLOAD_RUN_STATE& state,
    const std::vector<MTLOAD_METRIC_ROW>& metrics,
    const std::vector<MTLOAD_SCENARIO_METRIC_ROW>& scenarios,
    const std::vector<MTLOAD_THRESHOLD_RESULT>& thresholds,
    const std::vector<MTLOAD_ERROR_SAMPLE>& errors)
{
    xlnt::workbook workbook;
    xlnt::worksheet summary = workbook.active_sheet();
    summary.title(u8"汇总");
    const char* summaryNames[] = { u8"运行编号", u8"状态", u8"结果码", u8"已请求", u8"总请求",
        u8"有效运行(ms)", u8"总吞吐(req/s)", u8"峰值用户", u8"峰值在途请求",
        u8"已调度事务", u8"已完成事务", u8"丢弃事务" };
    const std::string summaryValues[] = { std::to_string(state.runId), std::to_string(state.state),
        std::to_string(state.lastResult), std::to_string(state.totalSamples),
        std::to_string(state.plannedRequests), std::to_string(state.effectiveElapsedMs),
        std::to_string(state.totalThroughputPerSec), std::to_string(state.peakActiveUsers),
        std::to_string(state.peakActiveRequests), std::to_string(state.scheduledIterations),
        std::to_string(state.completedIterations), std::to_string(state.droppedIterations) };
    for (int index = 0; index < static_cast<int>(_countof(summaryNames)); ++index)
    {
        summary.cell(xlnt::cell_reference(1, index + 1)).value(summaryNames[index]);
        summary.cell(xlnt::cell_reference(2, index + 1)).value(summaryValues[index]);
    }

    xlnt::worksheet scenarioSheet = workbook.create_sheet();
    scenarioSheet.title(u8"业务事务");
    const char* scenarioHeaders[] = { u8"场景ID", u8"场景", u8"启动", u8"完成", u8"成功",
        u8"失败", u8"丢弃", u8"平均耗时(ms)", u8"P50(ms)", u8"P90(ms)", u8"P95(ms)",
        u8"P99(ms)", u8"事务吞吐(tx/s)" };
    for (int column = 0; column < static_cast<int>(_countof(scenarioHeaders)); ++column)
        scenarioSheet.cell(xlnt::cell_reference(column + 1, 1)).value(scenarioHeaders[column]);
    for (int index = 0; index < static_cast<int>(scenarios.size()); ++index)
    {
        const auto& row = scenarios[index];
        const int line = index + 2;
        scenarioSheet.cell(xlnt::cell_reference(1, line)).value(row.scenarioId);
        scenarioSheet.cell(xlnt::cell_reference(2, line)).value(row.scenario);
        scenarioSheet.cell(xlnt::cell_reference(3, line)).value(std::to_string(row.started));
        scenarioSheet.cell(xlnt::cell_reference(4, line)).value(std::to_string(row.completed));
        scenarioSheet.cell(xlnt::cell_reference(5, line)).value(std::to_string(row.succeeded));
        scenarioSheet.cell(xlnt::cell_reference(6, line)).value(std::to_string(row.failed));
        scenarioSheet.cell(xlnt::cell_reference(7, line)).value(std::to_string(row.dropped));
        scenarioSheet.cell(xlnt::cell_reference(8, line)).value(row.averageMs);
        scenarioSheet.cell(xlnt::cell_reference(9, line)).value(row.medianMs);
        scenarioSheet.cell(xlnt::cell_reference(10, line)).value(row.p90Ms);
        scenarioSheet.cell(xlnt::cell_reference(11, line)).value(row.p95Ms);
        scenarioSheet.cell(xlnt::cell_reference(12, line)).value(row.p99Ms);
        scenarioSheet.cell(xlnt::cell_reference(13, line)).value(row.throughputPerSec);
    }

    xlnt::worksheet thresholdSheet = workbook.create_sheet();
    thresholdSheet.title(u8"阈值结果");
    const char* thresholdHeaders[] = { u8"ID", u8"名称", u8"范围", u8"指标", u8"目标",
        u8"比较", u8"期望", u8"实际", u8"通过" };
    for (int column = 0; column < static_cast<int>(_countof(thresholdHeaders)); ++column)
        thresholdSheet.cell(xlnt::cell_reference(column + 1, 1)).value(thresholdHeaders[column]);
    for (int index = 0; index < static_cast<int>(thresholds.size()); ++index)
    {
        const auto& row = thresholds[index];
        const int line = index + 2;
        thresholdSheet.cell(xlnt::cell_reference(1, line)).value(row.id);
        thresholdSheet.cell(xlnt::cell_reference(2, line)).value(row.name);
        thresholdSheet.cell(xlnt::cell_reference(3, line)).value(row.scope);
        thresholdSheet.cell(xlnt::cell_reference(4, line)).value(row.metric);
        thresholdSheet.cell(xlnt::cell_reference(5, line)).value(row.target);
        thresholdSheet.cell(xlnt::cell_reference(6, line)).value(row.comparison);
        thresholdSheet.cell(xlnt::cell_reference(7, line)).value(row.expected);
        thresholdSheet.cell(xlnt::cell_reference(8, line)).value(row.actual);
        thresholdSheet.cell(xlnt::cell_reference(9, line)).value(row.passed == 1 ? u8"通过" : u8"失败");
    }

    xlnt::worksheet metricSheet = workbook.create_sheet();
    metricSheet.title(u8"接口指标");
    const char* metricHeaders[] = { u8"场景", u8"步骤", u8"接口", u8"方法", u8"路径",
        u8"已请求", u8"总请求", u8"进度", u8"成功", u8"失败", u8"错误率",
        u8"平均耗时(ms)", u8"P90(ms)", u8"P95(ms)", u8"P99(ms)", u8"吞吐(req/s)" };
    for (int column = 0; column < static_cast<int>(_countof(metricHeaders)); ++column)
        metricSheet.cell(xlnt::cell_reference(column + 1, 1)).value(metricHeaders[column]);
    for (int index = 0; index < static_cast<int>(metrics.size()); ++index)
    {
        const auto& row = metrics[index];
        const int line = index + 2;
        metricSheet.cell(xlnt::cell_reference(1, line)).value(row.scenario);
        metricSheet.cell(xlnt::cell_reference(2, line)).value(row.step);
        metricSheet.cell(xlnt::cell_reference(3, line)).value(row.label);
        metricSheet.cell(xlnt::cell_reference(4, line)).value(row.method);
        metricSheet.cell(xlnt::cell_reference(5, line)).value(row.path);
        metricSheet.cell(xlnt::cell_reference(6, line)).value(std::to_string(row.samples));
        metricSheet.cell(xlnt::cell_reference(7, line)).value(std::to_string(row.plannedRequests));
        metricSheet.cell(xlnt::cell_reference(8, line)).value(row.plannedRequests == 0 ? 0.0 :
            static_cast<double>(row.samples) / row.plannedRequests);
        metricSheet.cell(xlnt::cell_reference(9, line)).value(std::to_string(row.succeeded));
        metricSheet.cell(xlnt::cell_reference(10, line)).value(std::to_string(row.failed));
        metricSheet.cell(xlnt::cell_reference(11, line)).value(row.errorRate);
        metricSheet.cell(xlnt::cell_reference(12, line)).value(row.averageMs);
        metricSheet.cell(xlnt::cell_reference(13, line)).value(row.p90Ms);
        metricSheet.cell(xlnt::cell_reference(14, line)).value(row.p95Ms);
        metricSheet.cell(xlnt::cell_reference(15, line)).value(row.p99Ms);
        metricSheet.cell(xlnt::cell_reference(16, line)).value(row.throughputPerSec);
    }

    xlnt::worksheet errorSheet = workbook.create_sheet();
    errorSheet.title(u8"错误明细");
    const char* errorHeaders[] = { u8"时间戳", u8"场景", u8"步骤", u8"接口", u8"分类",
        u8"错误码", u8"HTTP状态", u8"耗时(ms)", u8"摘要" };
    for (int column = 0; column < static_cast<int>(_countof(errorHeaders)); ++column)
        errorSheet.cell(xlnt::cell_reference(column + 1, 1)).value(errorHeaders[column]);
    for (int index = 0; index < static_cast<int>(errors.size()); ++index)
    {
        const auto& item = errors[index];
        const int line = index + 2;
        errorSheet.cell(xlnt::cell_reference(1, line)).value(std::to_string(item.timestampUnixMs));
        errorSheet.cell(xlnt::cell_reference(2, line)).value(item.scenario);
        errorSheet.cell(xlnt::cell_reference(3, line)).value(item.step);
        errorSheet.cell(xlnt::cell_reference(4, line)).value(item.label);
        errorSheet.cell(xlnt::cell_reference(5, line)).value(item.category);
        errorSheet.cell(xlnt::cell_reference(6, line)).value(item.result);
        errorSheet.cell(xlnt::cell_reference(7, line)).value(item.httpStatus);
        errorSheet.cell(xlnt::cell_reference(8, line)).value(item.elapsedMs);
        errorSheet.cell(xlnt::cell_reference(9, line)).value(item.summary);
    }
    workbook.save(path.u8string());
}

// 计算指定请求在有限循环场景中的计划次数；持续时间模式返回 0。
uint64_t PlannedRequests(const Scenario& scenario, const Request& request)
{
    if (scenario.load.durationMs != 0 || scenario.load.iterations == 0) return 0;
    return static_cast<uint64_t>(scenario.load.threads) * scenario.load.iterations * request.copies;
}

// 汇总一个虚拟用户完成一次场景循环所执行的请求副本数。
uint64_t RequestsPerIteration(const Scenario& scenario)
{
    uint64_t total = 0;
    for (const Step& step : scenario.steps)
        for (const Request& request : step.requests)
            total += request.copies;
    return total;
}

// 通过稳定 ID 查找只读场景，调用方在已校验计划内使用。
const Scenario* FindScenario(const Plan& plan, const std::string& scenarioId)
{
    for (const Scenario& scenario : plan.scenarios)
        if (scenario.id == scenarioId) return &scenario;
    return nullptr;
}

// 通过稳定 ID 查找只读工作负载，调用方在已校验计划内使用。
const Workload* FindWorkload(const Plan& plan, const std::string& workloadId)
{
    for (const Workload& workload : plan.workloads)
        if (workload.id == workloadId) return &workload;
    return nullptr;
}

// 确定性权重轮询：序号在权重周期内依次落入对应场景，不依赖随机数种子。
const Scenario* SelectWeightedScenario(const Plan& plan, const Workload& workload,
    uint64_t sequence)
{
    uint64_t totalWeight = 0;
    for (const WorkloadScenarioRef& reference : workload.scenarios) totalWeight += reference.weight;
    if (totalWeight == 0) return nullptr;
    const uint64_t position = sequence % totalWeight;
    uint64_t upper = 0;
    for (const WorkloadScenarioRef& reference : workload.scenarios)
    {
        upper += reference.weight;
        if (position < upper) return FindScenario(plan, reference.scenarioId);
    }
    return nullptr;
}

// 计算有限 closed 工作负载中某场景被选择的精确事务次数。
uint64_t PlannedScenarioIterations(const Workload& workload, const std::string& scenarioId)
{
    if (workload.load.mode != "closed" || workload.load.durationMs != 0 ||
        workload.load.iterations == 0) return 0;
    const uint64_t totalIterations = static_cast<uint64_t>(workload.load.threads) *
        workload.load.iterations;
    uint64_t totalWeight = 0;
    uint64_t before = 0;
    uint32_t ownWeight = 0;
    for (const WorkloadScenarioRef& reference : workload.scenarios)
    {
        if (reference.scenarioId == scenarioId)
        {
            before = totalWeight;
            ownWeight = reference.weight;
        }
        totalWeight += reference.weight;
    }
    if (totalWeight == 0 || ownWeight == 0) return 0;
    const uint64_t fullCycles = totalIterations / totalWeight;
    const uint64_t remainder = totalIterations % totalWeight;
    const uint64_t remainderCount = remainder <= before ? 0 :
        (std::min)(static_cast<uint64_t>(ownWeight), remainder - before);
    return fullCycles * ownWeight + remainderCount;
}

// 汇总工作负载有限计划请求数；持续时间和 arrival 模式返回 0。
uint64_t PlannedRequests(const Plan& plan, const Workload& workload)
{
    uint64_t total = 0;
    if (workload.useScenarioLoads)
    {
        for (const WorkloadScenarioRef& reference : workload.scenarios)
        {
            const Scenario* scenario = FindScenario(plan, reference.scenarioId);
            if (scenario == nullptr || scenario->load.durationMs != 0 || scenario->load.iterations == 0)
                return 0;
            total += static_cast<uint64_t>(scenario->load.threads) * scenario->load.iterations *
                RequestsPerIteration(*scenario);
        }
        return total;
    }
    if (workload.load.mode != "closed" || workload.load.durationMs != 0 ||
        workload.load.iterations == 0) return 0;
    for (const WorkloadScenarioRef& reference : workload.scenarios)
    {
        const Scenario* scenario = FindScenario(plan, reference.scenarioId);
        if (scenario != nullptr)
            total += PlannedScenarioIterations(workload, scenario->id) * RequestsPerIteration(*scenario);
    }
    return total;
}

// 计算工作负载中单个请求行的有限计划样本数，开放或持续时间模型返回 0。
uint64_t PlannedRequests(const Plan&, const Workload& workload,
    const Scenario& scenario, const Request& request)
{
    if (workload.useScenarioLoads) return PlannedRequests(scenario, request);
    return PlannedScenarioIterations(workload, scenario.id) * request.copies;
}
}

// 关键实现：容量至少为 1，防止错误配置导致所有工作线程永久等待。
PermitPool::PermitPool(uint32_t capacity) : m_u32Available((std::max)(1U, capacity)) {}

// 关键实现：在条件变量上同时等待许可或停止，成功时在锁内扣减许可。
bool PermitPool::Acquire(const std::atomic<bool>& stopping)
{
    std::unique_lock<std::mutex> lock(m_clMutex);
    m_clChanged.wait(lock, [&]() { return m_u32Available > 0 || stopping.load(); });
    if (stopping.load()) return false;
    --m_u32Available;
    return true;
}

// 关键实现：在锁内归还许可并只唤醒一个竞争线程。
void PermitPool::Release()
{
    std::lock_guard<std::mutex> guard(m_clMutex);
    ++m_u32Available;
    m_clChanged.notify_one();
}

// 关键实现：停机时唤醒全部等待者，让其重新检查 stopping。
void PermitPool::WakeAll()
{
    m_clChanged.notify_all();
}

// 关键实现：复制固定选项并按 maxConnections 建立全实例许可池。
LoadEngine::LoadEngine(const MTLOAD_ENGINE_OPTIONS& options)
    : m_stOptions(options), m_clPermits(options.maxConnections)
{
}

// 关键实现：只在观测值更大时推进峰值，多个请求和用户线程可无锁竞争。
void LoadEngine::UpdatePeak(std::atomic<uint32_t>& peak, uint32_t value)
{
    uint32_t current = peak.load();
    while (current < value && !peak.compare_exchange_weak(current, value)) {}
}

// 关键实现：复用 Stop 并联结顶层线程，析构返回时不再有线程访问成员。
LoadEngine::~LoadEngine()
{
    Stop();
    Wait(UINT32_MAX);
    if (m_clWorker.joinable()) m_clWorker.join();
}

// 关键实现：在状态锁内同步保存结果码和诊断，供 ABI 随后查询。
void LoadEngine::SetError(int result, const std::string& error)
{
    std::lock_guard<std::mutex> guard(m_clStateMutex);
    m_iLastResult = result;
    m_strLastError = error;
}

// 关键实现：先在锁外解析局部计划，再在状态锁内一次性替换当前模型。
int LoadEngine::LoadPlanFile(const std::string& path)
{
    bool reapCompletedWorker = false;
    {
        std::lock_guard<std::mutex> guard(m_clStateMutex);
        if (m_iState == MTLOAD_STATE_STARTING || m_iState == MTLOAD_STATE_RUNNING ||
            m_iState == MTLOAD_STATE_PAUSING || m_iState == MTLOAD_STATE_PAUSED ||
            m_iState == MTLOAD_STATE_STOPPING)
            return MTLOAD_E_INVALID_STATE;
        // 终态只表示工作已结束；std::thread 仍需由控制线程 join 后才能启动下一轮。
        reapCompletedWorker = m_clWorker.joinable();
    }
    if (reapCompletedWorker) m_clWorker.join();

    Plan parsed;
    std::string error;
    const int result = ParsePlan(path, parsed, error);
    if (result != MTLOAD_OK)
    {
        SetError(result, error);
        return result;
    }
    std::lock_guard<std::mutex> guard(m_clStateMutex);
    m_stPlan = std::move(parsed);
    m_strSelectedEnvironmentId = m_stPlan.defaultEnvironmentId;
    m_strSelectedScenarioId.clear();
    m_strSelectedWorkloadId = m_stPlan.defaultWorkloadId;
    m_strSelectedUserId.clear();
    m_bHasPlan = true;
    m_bValidated = false;
    m_iState = MTLOAD_STATE_PLAN_LOADED;
    m_iLastResult = MTLOAD_OK;
    m_strLastError.clear();
    m_i64StartUnixMs = 0;
    m_i64EndUnixMs = 0;
    m_u64PlannedRequests = 0;
    m_u64PausedDurationMs = 0;
    m_bPauseRequested.store(false);
    m_u64TotalSamples.store(0);
    {
        std::lock_guard<std::mutex> metricGuard(m_clMetricsMutex);
        m_mapMetrics.clear();
        m_mapScenarioMetrics.clear();
        m_vecThresholdResults.clear();
        m_vecErrors.clear();
    }
    return MTLOAD_OK;
}

// 关键实现：在锁外执行语义校验，成功后初始化默认选择和零值指标行。
int LoadEngine::Validate()
{
    Plan copy;
    {
        std::lock_guard<std::mutex> guard(m_clStateMutex);
        if (!m_bHasPlan || m_iState != MTLOAD_STATE_PLAN_LOADED) return MTLOAD_E_INVALID_STATE;
        copy = m_stPlan;
    }
    std::string error;
    const int result = ValidatePlan(copy, error);
    {
        std::lock_guard<std::mutex> guard(m_clStateMutex);
        if (result == MTLOAD_OK)
        {
            m_bValidated = true;
            m_iState = MTLOAD_STATE_VALIDATED;
            const Workload* workload = FindWorkload(copy, m_strSelectedWorkloadId);
            m_u64PlannedRequests = workload == nullptr ? 0 : PlannedRequests(copy, *workload);
            m_strLastError.clear();
        }
        else
        {
            m_bValidated = false;
            m_u64PlannedRequests = 0;
            m_strLastError = error;
        }
        m_iLastResult = result;
    }
    if (result == MTLOAD_OK) ResetMetricDefinitions(copy, m_strSelectedWorkloadId);
    return result;
}

// 关键实现：在状态锁内把环境模型转换为不含业务敏感值的 ABI POD。
std::vector<MTLOAD_ENVIRONMENT> LoadEngine::GetEnvironments() const
{
    std::lock_guard<std::mutex> guard(m_clStateMutex);
    std::vector<MTLOAD_ENVIRONMENT> rows;
    rows.reserve(m_stPlan.environments.size());
    for (const auto& pair : m_stPlan.environments)
    {
        const Environment& source = pair.second;
        MTLOAD_ENVIRONMENT row{};
        row.structSize = sizeof(row);
        row.abiVersion = MTLOAD_ABI_VERSION;
        CopyText(row.id, source.id);
        CopyText(row.name, source.name);
        CopyText(row.scheme, source.scheme);
        CopyText(row.host, source.host);
        CopyText(row.basePath, source.basePath);
        row.port = source.port;
        row.selected = source.id == m_strSelectedEnvironmentId ? 1U : 0U;
        rows.push_back(row);
    }
    return rows;
}

// 关键实现：仅在非活动状态接受存在的环境 ID，并保持计划内容不变。
int LoadEngine::SelectEnvironment(const std::string& environmentId)
{
    std::lock_guard<std::mutex> guard(m_clStateMutex);
    if (!m_bHasPlan || environmentId.empty()) return MTLOAD_E_INVALID_ARGUMENT;
    if (m_iState == MTLOAD_STATE_STARTING || m_iState == MTLOAD_STATE_RUNNING ||
        m_iState == MTLOAD_STATE_PAUSING || m_iState == MTLOAD_STATE_PAUSED ||
        m_iState == MTLOAD_STATE_STOPPING)
        return MTLOAD_E_INVALID_STATE;
    if (m_stPlan.environments.find(environmentId) == m_stPlan.environments.end())
    {
        m_iLastResult = MTLOAD_E_NOT_FOUND;
        m_strLastError = "环境不存在: " + environmentId;
        return m_iLastResult;
    }
    m_strSelectedEnvironmentId = environmentId;
    m_iLastResult = MTLOAD_OK;
    m_strLastError.clear();
    return MTLOAD_OK;
}

// 关键实现：复制场景负载参数并预计算单循环和总请求数。
std::vector<MTLOAD_SCENARIO> LoadEngine::GetScenarios() const
{
    std::lock_guard<std::mutex> guard(m_clStateMutex);
    std::vector<MTLOAD_SCENARIO> rows;
    rows.reserve(m_stPlan.scenarios.size());
    for (const Scenario& source : m_stPlan.scenarios)
    {
        MTLOAD_SCENARIO row{};
        row.structSize = sizeof(row);
        row.abiVersion = MTLOAD_ABI_VERSION;
        CopyText(row.id, source.id);
        CopyText(row.name, source.name);
        row.threads = source.load.threads;
        row.iterations = source.load.iterations;
        row.requestsPerIteration = RequestsPerIteration(source);
        row.plannedRequests = source.load.durationMs == 0 ?
            row.requestsPerIteration * source.load.threads * source.load.iterations : 0;
        row.selected = source.id == m_strSelectedScenarioId ? 1U : 0U;
        rows.push_back(row);
    }
    return rows;
}

// 关键实现：切换场景后重建零值指标定义，界面可立即看到完整步骤列表。
int LoadEngine::SelectScenario(const std::string& scenarioId)
{
    Plan planCopy;
    {
        std::lock_guard<std::mutex> guard(m_clStateMutex);
        if (!m_bValidated) return MTLOAD_E_INVALID_STATE;
        if (m_iState == MTLOAD_STATE_STARTING || m_iState == MTLOAD_STATE_RUNNING ||
            m_iState == MTLOAD_STATE_PAUSING || m_iState == MTLOAD_STATE_PAUSED ||
            m_iState == MTLOAD_STATE_STOPPING)
            return MTLOAD_E_INVALID_STATE;
        if (!scenarioId.empty() && std::none_of(m_stPlan.scenarios.begin(), m_stPlan.scenarios.end(),
            [&](const Scenario& scenario) { return scenario.id == scenarioId; }))
        {
            m_iLastResult = MTLOAD_E_NOT_FOUND;
            m_strLastError = "场景不存在: " + scenarioId;
            return m_iLastResult;
        }
        m_strSelectedScenarioId = scenarioId;
        m_strSelectedWorkloadId = scenarioId.empty() ? "legacy-all" : "legacy-" + scenarioId;
        const Workload* workload = FindWorkload(m_stPlan, m_strSelectedWorkloadId);
        if (workload == nullptr)
        {
            m_iLastResult = MTLOAD_E_NOT_FOUND;
            m_strLastError = "兼容工作负载不存在: " + m_strSelectedWorkloadId;
            return m_iLastResult;
        }
        m_u64PlannedRequests = PlannedRequests(m_stPlan, *workload);
        m_iLastResult = MTLOAD_OK;
        m_strLastError.clear();
        planCopy = m_stPlan;
    }
    ResetMetricDefinitions(planCopy, m_strSelectedWorkloadId);
    return MTLOAD_OK;
}

// 关键实现：把内部工作负载转换为稳定 POD，旧配置兼容项也明确标记 implicit。
std::vector<MTLOAD_WORKLOAD> LoadEngine::GetWorkloads() const
{
    std::lock_guard<std::mutex> guard(m_clStateMutex);
    std::vector<MTLOAD_WORKLOAD> rows;
    rows.reserve(m_stPlan.workloads.size());
    for (const Workload& source : m_stPlan.workloads)
    {
        MTLOAD_WORKLOAD row{};
        row.structSize = sizeof(row);
        row.abiVersion = MTLOAD_ABI_VERSION;
        CopyText(row.id, source.id);
        CopyText(row.name, source.name);
        CopyText(row.mode, source.useScenarioLoads ? "closed" : source.load.mode);
        CopyText(row.testType, source.testType);
        row.scenarioCount = static_cast<uint32_t>(source.scenarios.size());
        row.users = source.load.mode == "arrival" ? source.load.preAllocatedUsers : source.load.threads;
        row.maxUsers = source.load.mode == "arrival" ? source.load.maxUsers : source.load.threads;
        row.stageCount = static_cast<uint32_t>(source.load.stages.size());
        uint64_t stageDuration = 0;
        for (const LoadStage& stage : source.load.stages) stageDuration += stage.durationMs;
        row.durationMs = source.load.durationMs != 0 ? source.load.durationMs : stageDuration;
        row.ratePerSec = source.load.ratePerSec;
        row.selected = source.id == m_strSelectedWorkloadId ? 1U : 0U;
        row.implicit = source.implicit ? 1U : 0U;
        rows.push_back(row);
    }
    return rows;
}

// 关键实现：工作负载切换与场景兼容选择互斥，切换后重建接口和事务零值行。
int LoadEngine::SelectWorkload(const std::string& workloadId)
{
    Plan planCopy;
    {
        std::lock_guard<std::mutex> guard(m_clStateMutex);
        if (!m_bValidated || workloadId.empty()) return MTLOAD_E_INVALID_ARGUMENT;
        if (m_iState == MTLOAD_STATE_STARTING || m_iState == MTLOAD_STATE_RUNNING ||
            m_iState == MTLOAD_STATE_PAUSING || m_iState == MTLOAD_STATE_PAUSED ||
            m_iState == MTLOAD_STATE_STOPPING)
            return MTLOAD_E_INVALID_STATE;
        const Workload* workload = FindWorkload(m_stPlan, workloadId);
        if (workload == nullptr)
        {
            m_iLastResult = MTLOAD_E_NOT_FOUND;
            m_strLastError = "工作负载不存在: " + workloadId;
            return m_iLastResult;
        }
        m_strSelectedWorkloadId = workloadId;
        m_strSelectedScenarioId.clear();
        if (workload->implicit && workload->scenarios.size() == 1)
            m_strSelectedScenarioId = workload->scenarios.front().scenarioId;
        m_u64PlannedRequests = PlannedRequests(m_stPlan, *workload);
        m_iLastResult = MTLOAD_OK;
        m_strLastError.clear();
        planCopy = m_stPlan;
    }
    ResetMetricDefinitions(planCopy, workloadId);
    return MTLOAD_OK;
}

// 关键实现：只复制 Version/No/Login 等摘要，密码和 Param 永不跨 ABI。
std::vector<MTLOAD_USER> LoadEngine::GetUsers() const
{
    std::lock_guard<std::mutex> guard(m_clStateMutex);
    std::vector<MTLOAD_USER> rows;
    rows.reserve(m_stPlan.users.size());
    for (const auto& pair : m_stPlan.users)
    {
        const UserProfile& source = pair.second;
        MTLOAD_USER row{};
        row.structSize = sizeof(row);
        row.abiVersion = MTLOAD_ABI_VERSION;
        CopyText(row.id, source.id);
        CopyText(row.name, source.name);
        CopyText(row.version, source.variables.at("Version"));
        CopyText(row.no, source.variables.at("No"));
        CopyText(row.login, source.variables.at("Login"));
        row.selected = source.id == m_strSelectedUserId ? 1U : 0U;
        rows.push_back(row);
    }
    return rows;
}

// 关键实现：保存统一用户覆盖 ID；空字符串恢复各场景 UserRefs。
int LoadEngine::SelectUser(const std::string& userId)
{
    std::lock_guard<std::mutex> guard(m_clStateMutex);
    if (!m_bValidated) return MTLOAD_E_INVALID_STATE;
    if (m_iState == MTLOAD_STATE_STARTING || m_iState == MTLOAD_STATE_RUNNING ||
        m_iState == MTLOAD_STATE_PAUSING || m_iState == MTLOAD_STATE_PAUSED ||
        m_iState == MTLOAD_STATE_STOPPING)
        return MTLOAD_E_INVALID_STATE;
    if (!userId.empty() && m_stPlan.users.find(userId) == m_stPlan.users.end())
    {
        m_iLastResult = MTLOAD_E_NOT_FOUND;
        m_strLastError = "用户不存在: " + userId;
        return m_iLastResult;
    }
    m_strSelectedUserId = userId;
    m_iLastResult = MTLOAD_OK;
    m_strLastError.clear();
    return MTLOAD_OK;
}

// 关键实现：清理上一轮、建立新 runId 和指标定义后再发布 STARTING 并创建线程。
int LoadEngine::Start(const std::string& runName)
{
    // 先在锁内判断上一轮是否可回收，再在锁外 join，避免工作线程结束时反向等待状态锁。
    bool reapCompletedWorker = false;
    {
        std::lock_guard<std::mutex> guard(m_clStateMutex);
        const bool restartable = m_iState == MTLOAD_STATE_VALIDATED ||
            m_iState == MTLOAD_STATE_COMPLETED || m_iState == MTLOAD_STATE_FAILED;
        if (!m_bValidated || !restartable) return MTLOAD_E_INVALID_STATE;
        reapCompletedWorker = m_clWorker.joinable();
    }
    if (reapCompletedWorker) m_clWorker.join();

    // 重新取得锁后二次确认状态，并原子建立完整的新一轮运行上下文。
    std::lock_guard<std::mutex> guard(m_clStateMutex);
    const bool restartable = m_iState == MTLOAD_STATE_VALIDATED ||
        m_iState == MTLOAD_STATE_COMPLETED || m_iState == MTLOAD_STATE_FAILED;
    if (!m_bValidated || !restartable || m_clWorker.joinable()) return MTLOAD_E_INVALID_STATE;
    m_strRunName = runName.empty() ? m_stPlan.name : runName;
    ++m_u64RunId;
    m_i64StartUnixMs = UnixMs();
    m_i64EndUnixMs = 0;
    m_iLastResult = MTLOAD_OK;
    m_strLastError.clear();
    m_iState = MTLOAD_STATE_STARTING;
    m_bStopping.store(false);
    m_bPauseRequested.store(false);
    m_u64PausedDurationMs = 0;
    m_u64TotalSamples.store(0);
    m_u32ActiveScenarios.store(0);
    m_u32ActiveUsers.store(0);
    m_u32ActiveRequests.store(0);
    m_u32PeakActiveUsers.store(0);
    m_u32PeakActiveRequests.store(0);
    m_u64ScheduledIterations.store(0);
    m_u64CompletedIterations.store(0);
    m_u64DroppedIterations.store(0);
    ResetMetricDefinitions(m_stPlan, m_strSelectedWorkloadId);
    try
    {
        m_clWorker = std::thread(&LoadEngine::Run, this);
    }
    catch (const std::exception& exception)
    {
        m_iState = MTLOAD_STATE_FAILED;
        m_iLastResult = MTLOAD_E_START_FAILED;
        m_strLastError = std::string("创建运行线程失败: ") + exception.what();
        m_i64EndUnixMs = UnixMs();
        return m_iLastResult;
    }
    return MTLOAD_OK;
}

// 关键实现：只设置暂停标记和 PAUSING，具体 PAUSED 在调度线程到达边界时确认。
int LoadEngine::Pause()
{
    std::lock_guard<std::mutex> pauseGuard(m_clPauseMutex);
    {
        std::lock_guard<std::mutex> guard(m_clStateMutex);
        if (m_iState == MTLOAD_STATE_PAUSING || m_iState == MTLOAD_STATE_PAUSED) return MTLOAD_OK;
        if (m_iState != MTLOAD_STATE_STARTING && m_iState != MTLOAD_STATE_RUNNING)
            return MTLOAD_E_INVALID_STATE;
        m_bPauseRequested.store(true);
        m_clPauseStarted = std::chrono::steady_clock::now();
        m_iState = m_u32ActiveRequests.load() == 0 ? MTLOAD_STATE_PAUSED : MTLOAD_STATE_PAUSING;
    }
    m_clPauseChanged.notify_all();
    m_clStateChanged.notify_all();
    return MTLOAD_OK;
}

// 关键实现：累计本次暂停时长、清除标记并唤醒所有虚拟用户。
int LoadEngine::Resume()
{
    {
        std::lock_guard<std::mutex> guard(m_clStateMutex);
        if (m_iState == MTLOAD_STATE_RUNNING || m_iState == MTLOAD_STATE_STARTING) return MTLOAD_OK;
        if (m_iState != MTLOAD_STATE_PAUSING && m_iState != MTLOAD_STATE_PAUSED)
            return MTLOAD_E_INVALID_STATE;
        if (m_bPauseRequested.exchange(false))
        {
            m_u64PausedDurationMs += static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - m_clPauseStarted).count());
        }
        m_iState = MTLOAD_STATE_RUNNING;
    }
    m_clPauseChanged.notify_all();
    m_clStateChanged.notify_all();
    return MTLOAD_OK;
}

// 关键实现：幂等发布 STOPPING，依次唤醒暂停门禁和并发许可等待者。
int LoadEngine::Stop()
{
    {
        std::lock_guard<std::mutex> guard(m_clStateMutex);
        if (m_iState == MTLOAD_STATE_STARTING || m_iState == MTLOAD_STATE_RUNNING ||
            m_iState == MTLOAD_STATE_PAUSING || m_iState == MTLOAD_STATE_PAUSED)
            m_iState = MTLOAD_STATE_STOPPING;
        else if (m_iState != MTLOAD_STATE_STOPPING) return MTLOAD_OK;
        if (m_bPauseRequested.exchange(false))
        {
            m_u64PausedDurationMs += static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - m_clPauseStarted).count());
        }
    }
    m_bStopping.store(true);
    m_clPermits.WakeAll();
    m_clPauseChanged.notify_all();
    m_clStateChanged.notify_all();
    return MTLOAD_OK;
}

// 关键实现：只在稳定非活动状态清空结果，随后恢复与计划校验状态一致的状态值。
int LoadEngine::ClearResults()
{
    std::lock_guard<std::mutex> guard(m_clStateMutex);
    if (m_iState == MTLOAD_STATE_STARTING || m_iState == MTLOAD_STATE_RUNNING ||
        m_iState == MTLOAD_STATE_PAUSING || m_iState == MTLOAD_STATE_PAUSED ||
        m_iState == MTLOAD_STATE_STOPPING)
        return MTLOAD_E_INVALID_STATE;
    m_i64StartUnixMs = 0;
    m_i64EndUnixMs = 0;
    m_u64PausedDurationMs = 0;
    m_u64TotalSamples.store(0);
    m_u32PeakActiveUsers.store(0);
    m_u32PeakActiveRequests.store(0);
    m_u64ScheduledIterations.store(0);
    m_u64CompletedIterations.store(0);
    m_u64DroppedIterations.store(0);
    m_iLastResult = MTLOAD_OK;
    m_strLastError.clear();
    std::lock_guard<std::mutex> metricGuard(m_clMetricsMutex);
    m_mapMetrics.clear();
    m_mapScenarioMetrics.clear();
    m_vecThresholdResults.clear();
    m_vecErrors.clear();
    return MTLOAD_OK;
}

// 关键实现：在 m_clStateChanged 上等待终态谓词，有限等待超时不改变运行状态。
int LoadEngine::Wait(uint32_t timeoutMs)
{
    std::unique_lock<std::mutex> lock(m_clStateMutex);
    // 未启动的 CREATED/PLAN_LOADED/VALIDATED 实例没有工作线程，应立即返回。
    auto terminal = [&]() { return !m_clWorker.joinable() ||
        m_iState == MTLOAD_STATE_COMPLETED || m_iState == MTLOAD_STATE_FAILED; };
    bool done = true;
    if (!terminal())
    {
        if (timeoutMs == UINT32_MAX) m_clStateChanged.wait(lock, terminal);
        else done = m_clStateChanged.wait_for(lock, std::chrono::milliseconds(timeoutMs), terminal);
    }
    lock.unlock();
    if (!done) return MTLOAD_E_TIMEOUT;
    if (m_clWorker.joinable()) m_clWorker.join();
    return MTLOAD_OK;
}

// 关键实现：在状态锁内写入终态和结束时间，再广播唤醒所有等待者。
void LoadEngine::SetTerminalState(int state, int result)
{
    {
        std::lock_guard<std::mutex> guard(m_clStateMutex);
        if (m_bPauseRequested.exchange(false))
        {
            m_u64PausedDurationMs += static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - m_clPauseStarted).count());
        }
        m_iState = state;
        m_iLastResult = result;
        m_i64EndUnixMs = UnixMs();
    }
    m_clPauseChanged.notify_all();
    m_clStateChanged.notify_all();
}

// 关键实现：按已选择工作负载进入 closed、arrival 或旧全部场景路径，
// 所有工作完成后统一评估阈值；阈值失败仍保持 COMPLETED 终态。
void LoadEngine::Run()
{
    try
    {
        // 进入工作线程后才发布 RUNNING，保证 STARTING 表示线程尚未真正接管执行。
        {
            std::lock_guard<std::mutex> guard(m_clStateMutex);
            if (m_iState == MTLOAD_STATE_STARTING) m_iState = MTLOAD_STATE_RUNNING;
        }
        m_clStateChanged.notify_all();
        const Workload* workload = FindWorkload(m_stPlan, m_strSelectedWorkloadId);
        if (workload == nullptr) throw std::runtime_error("所选工作负载不存在");
        if (workload->useScenarioLoads) RunLegacyAll(*workload);
        else if (workload->load.mode == "arrival") RunArrivalWorkload(*workload);
        else RunClosedWorkload(*workload);
        const bool thresholdsPassed = EvaluateThresholds(*workload);
        SetTerminalState(MTLOAD_STATE_COMPLETED,
            thresholdsPassed ? MTLOAD_OK : MTLOAD_E_THRESHOLD_FAILED);
    }
    catch (const std::exception& exception)
    {
        {
            std::lock_guard<std::mutex> guard(m_clStateMutex);
            m_strLastError = std::string("运行线程异常: ") + exception.what();
        }
        SetTerminalState(MTLOAD_STATE_FAILED, MTLOAD_E_INTERNAL);
    }
}

// 关键实现：兼容旧“全部场景”语义，每个场景沿用自身 Load 并彼此并行。
void LoadEngine::RunLegacyAll(const Workload& workload)
{
    std::vector<std::thread> scenarios;
    for (const WorkloadScenarioRef& reference : workload.scenarios)
    {
        const Scenario* scenario = FindScenario(m_stPlan, reference.scenarioId);
        if (scenario != nullptr)
            scenarios.emplace_back(&LoadEngine::RunScenario, this, std::cref(*scenario), std::cref(scenario->load));
    }
    for (auto& thread : scenarios) thread.join();
}

// 关键实现：closed 工作负载的每个线程代表一个虚拟用户；多场景按全局事务序号
// 进行确定性权重轮询，权重对应完整事务而不是 HTTP 请求数量。
void LoadEngine::RunClosedWorkload(const Workload& workload)
{
    m_u32ActiveScenarios.fetch_add(1);
    std::atomic<uint64_t> sequence{ 0 };
    std::vector<std::thread> users;
    for (uint32_t index = 0; index < workload.load.threads && !m_bStopping.load(); ++index)
    {
        users.emplace_back([&, index]()
        {
            const uint32_t active = m_u32ActiveUsers.fetch_add(1) + 1;
            UpdatePeak(m_u32PeakActiveUsers, active);
            struct UserGuard
            {
                std::atomic<uint32_t>& count;
                ~UserGuard() { count.fetch_sub(1); }
            } userGuard{ m_u32ActiveUsers };
            if (workload.load.rampUpMs > 0 && workload.load.threads > 1)
            {
                const uint64_t delayMs = static_cast<uint64_t>(workload.load.rampUpMs) *
                    index / workload.load.threads;
                if (!WaitForActiveDelay(delayMs)) return;
            }
            if (!WaitUntilRunnable()) return;
            const auto started = std::chrono::steady_clock::now();
            const uint64_t pausedAtStart = PausedDurationMs();
            uint64_t localIteration = 0;
            while (!m_bStopping.load())
            {
                if (workload.load.iterations > 0 && localIteration >= workload.load.iterations) break;
                if (workload.load.durationMs > 0)
                {
                    const uint64_t wallMs = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - started).count());
                    const uint64_t pausedMs = PausedDurationMs() - pausedAtStart;
                    if (wallMs >= pausedMs + workload.load.durationMs) break;
                }
                if (!WaitUntilRunnable()) break;
                const uint64_t current = sequence.fetch_add(1);
                const Scenario* scenario = SelectWeightedScenario(m_stPlan, workload, current);
                if (scenario == nullptr) break;
                m_u64ScheduledIterations.fetch_add(1);
                RunIteration(*scenario, index, current);
                ++localIteration;
            }
        });
    }
    for (auto& user : users) user.join();
    m_u32ActiveScenarios.fetch_sub(1);
}

// 关键实现：旧单场景 closed 调度器保留原有线程、迭代、持续时间和 ramp-up 语义。
void LoadEngine::RunScenario(const Scenario& scenario, const Load& load)
{
    m_u32ActiveScenarios.fetch_add(1);
    // 每个用户线程拥有独立迭代位置和变量上下文，相同账号也不共享可变会话值。
    std::vector<std::thread> users;
    for (uint32_t index = 0; index < load.threads && !m_bStopping.load(); ++index)
    {
        users.emplace_back([&, index]()
        {
            const uint32_t active = m_u32ActiveUsers.fetch_add(1) + 1;
            UpdatePeak(m_u32PeakActiveUsers, active);
            // 栈守卫保证提前停止、延迟取消和异常路径均能归还活动用户计数。
            struct UserGuard
            {
                std::atomic<uint32_t>& count; // 借用 LoadEngine 活动用户计数器。
                // 用户线程离开作用域时只执行一次计数回收。
                ~UserGuard() { count.fetch_sub(1); }
            } userGuard{ m_u32ActiveUsers };
            if (load.rampUpMs > 0 && load.threads > 1)
            {
                const uint64_t delayMs = static_cast<uint64_t>(load.rampUpMs) * index / load.threads;
                if (!WaitForActiveDelay(delayMs)) return;
            }
            if (!WaitUntilRunnable()) return;
            // 持续时间模式使用单调时钟，并扣除暂停区间。
            const auto started = std::chrono::steady_clock::now();
            const uint64_t pausedAtStart = PausedDurationMs();
            uint64_t iteration = 0;
            while (!m_bStopping.load())
            {
                if (load.iterations > 0 && iteration >= load.iterations) break;
                if (load.durationMs > 0)
                {
                    const uint64_t wallMs = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - started).count());
                    const uint64_t pausedMs = PausedDurationMs() - pausedAtStart;
                    if (wallMs >= pausedMs + load.durationMs) break;
                }
                if (!WaitUntilRunnable()) break;
                m_u64ScheduledIterations.fetch_add(1);
                RunIteration(scenario, index, iteration++);
            }
        });
    }
    for (auto& user : users) user.join();
    m_u32ActiveScenarios.fetch_sub(1);
}

// 关键实现：开放模型按活动时间积分生成到达事件。空闲用户不足时立即丢弃，
// 工作队列只承载已获得执行容量的事务，因此不会形成隐藏排队和协调遗漏。
void LoadEngine::RunArrivalWorkload(const Workload& workload)
{
    struct Job
    {
        const Scenario* scenario = nullptr;
        uint64_t sequence = 0;
    };
    m_u32ActiveScenarios.fetch_add(1);
    std::mutex poolMutex;
    std::condition_variable poolChanged;
    std::deque<Job> jobs;
    std::vector<std::thread> workers;
    uint32_t idleWorkers = 0;
    bool shutdown = false;

    auto spawnWorker = [&](uint32_t workerIndex)
    {
        workers.emplace_back([&, workerIndex]()
        {
            std::unique_lock<std::mutex> lock(poolMutex);
            for (;;)
            {
                ++idleWorkers;
                poolChanged.notify_all();
                poolChanged.wait(lock, [&]() { return shutdown || m_bStopping.load() || !jobs.empty(); });
                --idleWorkers;
                if ((shutdown || m_bStopping.load()) && jobs.empty()) break;
                if (jobs.empty()) continue;
                const Job job = jobs.front();
                jobs.pop_front();
                lock.unlock();
                const uint32_t active = m_u32ActiveUsers.fetch_add(1) + 1;
                UpdatePeak(m_u32PeakActiveUsers, active);
                RunIteration(*job.scenario, workerIndex, job.sequence);
                m_u32ActiveUsers.fetch_sub(1);
                lock.lock();
            }
            poolChanged.notify_all();
        });
    };

    for (uint32_t index = 0; index < workload.load.preAllocatedUsers; ++index) spawnWorker(index);
    {
        std::unique_lock<std::mutex> lock(poolMutex);
        poolChanged.wait(lock, [&]() { return idleWorkers == workers.size(); });
    }

    auto rateAt = [&](uint64_t activeMs)
    {
        if (workload.load.stages.empty()) return workload.load.ratePerSec;
        uint64_t offset = 0;
        double startRate = workload.load.ratePerSec;
        for (const LoadStage& stage : workload.load.stages)
        {
            const uint64_t end = offset + stage.durationMs;
            if (activeMs <= end)
            {
                const double progress = static_cast<double>(activeMs - offset) / stage.durationMs;
                return startRate + (stage.targetRatePerSec - startRate) * progress;
            }
            offset = end;
            startRate = stage.targetRatePerSec;
        }
        return startRate;
    };
    uint64_t durationMs = workload.load.durationMs;
    if (!workload.load.stages.empty())
    {
        durationMs = 0;
        for (const LoadStage& stage : workload.load.stages) durationMs += stage.durationMs;
    }
    const auto started = std::chrono::steady_clock::now();
    const uint64_t pausedAtStart = PausedDurationMs();
    uint64_t lastActiveMs = 0;
    uint64_t sequence = 0;
    double dueCredit = 0.0;
    while (!m_bStopping.load())
    {
        if (!WaitUntilRunnable()) break;
        const uint64_t wallMs = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started).count());
        const uint64_t pausedMs = PausedDurationMs() - pausedAtStart;
        const uint64_t activeMs = wallMs > pausedMs ? wallMs - pausedMs : 0;
        if (activeMs >= durationMs) break;
        const uint64_t deltaMs = activeMs - lastActiveMs;
        if (deltaMs > 0)
        {
            dueCredit += (rateAt(lastActiveMs) + rateAt(activeMs)) * 0.5 * deltaMs / 1000.0;
            lastActiveMs = activeMs;
        }
        while (dueCredit >= 1.0 && !m_bStopping.load())
        {
            dueCredit -= 1.0;
            const Scenario* scenario = SelectWeightedScenario(m_stPlan, workload, sequence);
            if (scenario == nullptr) break;
            m_u64ScheduledIterations.fetch_add(1);
            bool accepted = false;
            {
                std::lock_guard<std::mutex> lock(poolMutex);
                if (idleWorkers > jobs.size())
                {
                    jobs.push_back({ scenario, sequence });
                    accepted = true;
                }
                else if (workers.size() < workload.load.maxUsers)
                {
                    jobs.push_back({ scenario, sequence });
                    spawnWorker(static_cast<uint32_t>(workers.size()));
                    accepted = true;
                }
            }
            if (accepted) poolChanged.notify_one();
            else RecordDroppedTransaction(*scenario);
            ++sequence;
        }
        if (!WaitForActiveDelay(5)) break;
    }

    // 调度结束后直接关闭工作池。工作线程只在队列为空时退出，因此正常完成仍会
    // 排空已接收任务；停止时任务会沿 m_bStopping 快速完成取消记账。这里不能等待
    // idleWorkers == workers.size()，因为收到停止通知的空闲线程会直接退出，不会再
    // 回到空闲计数，继续等待将使顶层运行线程永久卡在 STOPPING。
    {
        std::lock_guard<std::mutex> lock(poolMutex);
        shutdown = true;
    }
    poolChanged.notify_all();
    for (auto& worker : workers) worker.join();
    m_u32ActiveScenarios.fetch_sub(1);
}

// 关键实现：为当前用户选择账号/数据行，步骤串行、步骤内请求副本并行并等待屏障；
// 任一步失败都会使事务失败，即使 onError=continue，alwaysRun 清理仍纳入完整耗时。
LoadEngine::IterationOutcome LoadEngine::RunIteration(const Scenario& scenario,
    uint32_t userIndex, uint64_t iteration)
{
    IterationOutcome result;
    const auto transactionStarted = std::chrono::steady_clock::now();
    RecordTransactionStarted(scenario);
    // 所有退出路径统一完成事务记账。手工终止会取消后续 HTTP 请求，但已被调度的
    // 事务仍必须闭合 started/completed，否则界面和报告会永久残留未完成事务。
    auto finishTransaction = [&](bool success)
    {
        result.completed = true;
        result.success = success;
        result.elapsedMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - transactionStarted).count();
        RecordTransactionCompleted(scenario, result);
        return result;
    };
    // 为本轮选择统一用户、UserRefs 或数据行，复制后交给该用户独占。
    std::map<std::string, std::string> row;
    if (!m_strSelectedUserId.empty())
    {
        row = m_stPlan.users.at(m_strSelectedUserId).variables;
    }
    else if (!scenario.userIds.empty())
    {
        const std::string& userId = scenario.userIds[userIndex % scenario.userIds.size()];
        row = m_stPlan.users.at(userId).variables;
    }
    else if (!scenario.dataSetId.empty())
    {
        const DataSet& dataSet = m_stPlan.dataSets.at(scenario.dataSetId);
        if (!dataSet.rows.empty()) row = dataSet.rows[(static_cast<uint64_t>(userIndex) + iteration) % dataSet.rows.size()];
    }
    // 运行时标识由引擎生成并放入只读作用域，保证同账号并发线程和循环的数据键互不覆盖。
    std::map<std::string, std::string> runtimeVariables;
    runtimeVariables["runId"] = std::to_string(m_u64RunId);
    runtimeVariables["threadIndex"] = std::to_string(static_cast<uint64_t>(userIndex) + 1ULL);
    runtimeVariables["iterationIndex"] = std::to_string(iteration + 1ULL);
    VariableContext variables(row, std::move(runtimeVariables));
    bool anyFailure = false;
    bool stopNormalSteps = false;
    // 步骤严格串行；业务失败后跳过普通步骤，但仍扫描并执行 alwaysRun 清理步骤。
    for (const Step& step : scenario.steps)
    {
        if (m_bStopping.load()) return finishTransaction(false);
        if (stopNormalSteps && !step.alwaysRun) continue;
        if (!WaitUntilRunnable()) return finishTransaction(false);
        std::vector<std::future<RequestOutcome>> futures;
        std::vector<const Request*> requests;
        // copies 展开为独立异步任务，但全部读取进入步骤时的同一只读变量上下文。
        for (const Request& request : step.requests)
        {
            for (uint32_t copy = 0; copy < request.copies; ++copy)
            {
                requests.push_back(&request);
                futures.push_back(std::async(std::launch::async, [&, request]()
                {
                    return RunRequest(scenario, step, request, row, variables);
                }));
            }
        }
        bool stepOk = true;
        // 同一步的请求只能读取进入屏障前的会话变量；等待全部完成后再统一提交提取值。
        for (auto& future : futures) future.wait();
        std::vector<RequestOutcome> outcomes;
        outcomes.reserve(futures.size());
        for (size_t i = 0; i < futures.size(); ++i)
        {
            outcomes.push_back(futures[i].get());
            if (!outcomes.back().success) stepOk = false;
        }
        for (const RequestOutcome& outcome : outcomes)
            for (const auto& pair : outcome.extracted) variables.SetSession(pair.first, pair.second);
        if (!stepOk)
        {
            anyFailure = true;
            if (step.onError != "continue") stopNormalSteps = true;
        }
    }
    return finishTransaction(!anyFailure);
}

// 关键实现：展开路径/正文/请求头，取得并发许可后执行 HTTP，再统一断言、提取和统计。
LoadEngine::RequestOutcome LoadEngine::RunRequest(const Scenario& scenario,
    const Step& step, const Request& request,
    const std::map<std::string, std::string>&, const VariableContext& variables)
{
    RequestOutcome outcome;
    // 先取得全实例并发许可；栈守卫覆盖全部早退路径并自动归还。
    if (!m_clPermits.Acquire(m_bStopping))
    {
        outcome.result = MTLOAD_E_INVALID_STATE;
        outcome.error = "运行已停止";
        return outcome;
    }
    struct PermitGuard
    {
        PermitPool& pool; // 借用全实例许可池。
        // 请求离开作用域时归还一次已取得许可。
        ~PermitGuard() { pool.Release(); }
    } guard{ m_clPermits };
    if (!BeginRequest())
    {
        outcome.result = MTLOAD_E_INVALID_STATE;
        outcome.error = "运行已停止";
        return outcome;
    }
    // 在途计数采用独立作用域守卫，避免变量展开失败时遗留 m_u32ActiveRequests。
    struct ScopeExit
    {
        std::function<void()> callback; // 离开作用域时执行的唯一清理动作。
        // 无论正常返回还是异常展开都执行回调。
        ~ScopeExit() { callback(); }
    } active{ [this]() { FinishRequest(); } };

    // 在发起网络前完成所有模板展开，任何未知变量都作为配置错误记录。
    std::string path;
    std::string body;
    std::string error;
    if (!variables.Expand(request.path, path, error) || !variables.Expand(request.body, body, error))
    {
        outcome.result = MTLOAD_E_CONFIG_SEMANTIC;
        outcome.error = error;
        RecordMetric(scenario, step, request, false, 0, 0, 0);
        RecordError(scenario, step, request, outcome);
        return outcome;
    }

    Request expandedRequest = request;
    // JSON 断言期望值允许引用当前运行、线程、循环和用户变量，用于校验并发隔离标识。
    for (Assertion& assertion : expandedRequest.assertions)
    {
        if (assertion.kind != Assertion::Kind::Json) continue;
        std::string expandedPath;
        std::string expandedValue;
        if (!variables.Expand(assertion.path, expandedPath, error) ||
            !variables.Expand(assertion.value, expandedValue, error))
        {
            outcome.result = MTLOAD_E_CONFIG_SEMANTIC;
            outcome.error = error;
            RecordMetric(scenario, step, request, false, 0, 0, 0);
            RecordError(scenario, step, request, outcome);
            return outcome;
        }
        assertion.path = std::move(expandedPath);
        assertion.value = std::move(expandedValue);
    }
    for (Header& header : expandedRequest.headers)
    {
        std::string expandedName;
        std::string expandedValue;
        if (!variables.Expand(header.name, expandedName, error) ||
            !variables.Expand(header.value, expandedValue, error))
        {
            outcome.result = MTLOAD_E_CONFIG_SEMANTIC;
            outcome.error = error;
            RecordMetric(scenario, step, request, false, 0, 0, 0);
            RecordError(scenario, step, request, outcome);
            return outcome;
        }
        header.name = std::move(expandedName);
        header.value = std::move(expandedValue);
    }
    // 使用环境副本构造最终目标，传输层只接收已经展开和合并的请求。
    const Target target = ResolveTarget(m_stPlan.targets.at(request.targetId));
    const std::string targetPath = JoinPath(target.basePath, path);
    HttpResponse response = ExecuteHttp(target, expandedRequest, m_stPlan.headers, targetPath, body,
        m_stOptions.maxResponseBytes, m_bStopping);
    outcome.result = response.result;
    outcome.httpStatus = response.status;
    outcome.elapsedMs = response.elapsedMs;
    // 仅传输完成后执行 JSON 断言和提取；所有结果最终进入同一指标样本。
    if (response.transportOk)
    {
        JsonEvaluation evaluation = EvaluateJson(expandedRequest, response.status, response.body);
        outcome.success = evaluation.success;
        outcome.error = evaluation.error;
        outcome.extracted = std::move(evaluation.extracted);
        if (!outcome.success) outcome.result = MTLOAD_E_INTERNAL;
    }
    else outcome.error = response.error;

    RecordMetric(scenario, step, request, outcome.success, outcome.elapsedMs,
        response.sentBytes, response.receivedBytes);
    if (!outcome.success) RecordError(scenario, step, request, outcome);
    return outcome;
}

// 关键实现：环境协议和地址共同覆盖连接目标；省略端口时按 HTTP/HTTPS 选 80/443。
Target LoadEngine::ResolveTarget(const Target& configured) const
{
    Target resolved = configured;
    std::lock_guard<std::mutex> guard(m_clStateMutex);
    if (m_stPlan.environments.empty())
    {
        if (resolved.port == 0) resolved.port = resolved.transport == "https" ? 443 : 80;
        return resolved;
    }
    const Environment& environment = m_stPlan.environments.at(m_strSelectedEnvironmentId);
    resolved.transport = environment.scheme;
    resolved.host = environment.host;
    resolved.port = environment.port != 0 ? environment.port :
        (resolved.transport == "https" ? 443 : 80);
    resolved.basePath = environment.basePath;
    resolved.tlsCaFile = environment.tlsCaFile;
    resolved.tlsVerify = environment.tlsVerify;
    return resolved;
}

// 关键实现：按工作负载引用预创建接口和事务累加器，并设置有限 closed 计划请求数。
void LoadEngine::ResetMetricDefinitions(const Plan& plan, const std::string& selectedWorkloadId)
{
    std::lock_guard<std::mutex> guard(m_clMetricsMutex);
    m_mapMetrics.clear();
    m_mapScenarioMetrics.clear();
    m_vecThresholdResults.clear();
    m_vecErrors.clear();
    const Workload* workload = FindWorkload(plan, selectedWorkloadId);
    if (workload == nullptr) return;
    for (const WorkloadScenarioRef& reference : workload->scenarios)
    {
        const Scenario* scenario = FindScenario(plan, reference.scenarioId);
        if (scenario == nullptr) continue;
        ScenarioAccumulator& transaction = m_mapScenarioMetrics[scenario->id];
        transaction.scenarioId = scenario->id;
        transaction.scenario = scenario->name;
        for (const Step& step : scenario->steps)
        {
            for (const Request& request : step.requests)
            {
                MetricAccumulator& metric = m_mapMetrics[MetricKey(*scenario, step, request)];
                metric.scenario = scenario->name;
                metric.step = step.name;
                metric.label = request.label;
                metric.method = request.method;
                metric.path = request.path;
                metric.plannedRequests = PlannedRequests(plan, *workload, *scenario, request);
            }
        }
    }
}

// 关键实现：暂停时在条件变量上等待继续或停止，并由到达线程确认 PAUSED 状态。
bool LoadEngine::WaitUntilRunnable()
{
    std::unique_lock<std::mutex> lock(m_clPauseMutex);
    m_clPauseChanged.wait(lock, [this]() { return !m_bPauseRequested.load() || m_bStopping.load(); });
    return !m_bStopping.load();
}

// 关键实现：在暂停门禁内登记在途请求，防止 Pause 漏掉刚放行的请求。
bool LoadEngine::BeginRequest()
{
    std::unique_lock<std::mutex> lock(m_clPauseMutex);
    m_clPauseChanged.wait(lock, [this]() { return !m_bPauseRequested.load() || m_bStopping.load(); });
    if (m_bStopping.load()) return false;
    // 与 Pause 共用 m_clPauseMutex，保证暂停观察到的 m_u32ActiveRequests 不漏掉刚放行的请求。
    const uint32_t active = m_u32ActiveRequests.fetch_add(1) + 1;
    UpdatePeak(m_u32PeakActiveRequests, active);
    return true;
}

// 关键实现：用单调时钟累计“活动时间”，暂停区间不消耗 ramp-up 延迟。
bool LoadEngine::WaitForActiveDelay(uint64_t delayMs)
{
    uint64_t remainingMs = delayMs;
    while (remainingMs > 0 && !m_bStopping.load())
    {
        if (!WaitUntilRunnable()) return false;
        const auto started = std::chrono::steady_clock::now();
        std::unique_lock<std::mutex> lock(m_clPauseMutex);
        m_clPauseChanged.wait_for(lock, std::chrono::milliseconds(remainingMs),
            [this]() { return m_bPauseRequested.load() || m_bStopping.load(); });
        const uint64_t elapsedMs = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started).count());
        remainingMs = elapsedMs >= remainingMs ? 0 : remainingMs - elapsedMs;
    }
    return !m_bStopping.load();
}

// 关键实现：减少在途计数；最后一个在途请求结束时把 PAUSING 确认为 PAUSED。
void LoadEngine::FinishRequest()
{
    const uint32_t remaining = m_u32ActiveRequests.fetch_sub(1) - 1;
    if (remaining != 0 || !m_bPauseRequested.load()) return;
    {
        std::lock_guard<std::mutex> guard(m_clStateMutex);
        if (m_iState == MTLOAD_STATE_PAUSING) m_iState = MTLOAD_STATE_PAUSED;
    }
    m_clStateChanged.notify_all();
}

// 关键实现：在暂停锁内合并历史暂停时长和当前未结束区间。
uint64_t LoadEngine::PausedDurationMs() const
{
    std::lock_guard<std::mutex> guard(m_clStateMutex);
    uint64_t result = m_u64PausedDurationMs;
    if (m_bPauseRequested.load())
    {
        result += static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - m_clPauseStarted).count());
    }
    return result;
}

// 关键实现：所有样本字段在同一 m_clMetricsMutex 临界区更新，保持统计口径一致。
void LoadEngine::RecordMetric(const Scenario& scenario, const Step& step,
    const Request& request, bool success, double elapsedMs,
    uint64_t sentBytes, uint64_t receivedBytes)
{
    std::lock_guard<std::mutex> guard(m_clMetricsMutex);
    MetricAccumulator& metric = m_mapMetrics[MetricKey(scenario, step, request)];
    metric.scenario = scenario.name;
    metric.step = step.name;
    metric.label = request.label;
    metric.method = request.method;
    metric.path = request.path;
    success ? ++metric.succeeded : ++metric.failed;
    metric.sentBytes += sentBytes;
    metric.receivedBytes += receivedBytes;
    metric.latencyMs.push_back(elapsedMs);
    m_u64TotalSamples.fetch_add(1);
}

// 关键实现：事务开始与接口请求分开计数，避免四步生命周期被误显示为四个事务。
void LoadEngine::RecordTransactionStarted(const Scenario& scenario)
{
    std::lock_guard<std::mutex> guard(m_clMetricsMutex);
    ScenarioAccumulator& metric = m_mapScenarioMetrics[scenario.id];
    metric.scenarioId = scenario.id;
    metric.scenario = scenario.name;
    ++metric.started;
}

// 关键实现：仅完整到达事务终点时计 completed，并把任一步失败归入事务失败。
void LoadEngine::RecordTransactionCompleted(const Scenario& scenario,
    const IterationOutcome& outcome)
{
    if (!outcome.completed) return;
    {
        std::lock_guard<std::mutex> guard(m_clMetricsMutex);
        ScenarioAccumulator& metric = m_mapScenarioMetrics[scenario.id];
        metric.scenarioId = scenario.id;
        metric.scenario = scenario.name;
        ++metric.completed;
        outcome.success ? ++metric.succeeded : ++metric.failed;
        metric.latencyMs.push_back(outcome.elapsedMs);
    }
    m_u64CompletedIterations.fetch_add(1);
}

// 关键实现：arrival 达到 maxUsers 时按已经选定的场景记录丢弃，不进入等待队列。
void LoadEngine::RecordDroppedTransaction(const Scenario& scenario)
{
    {
        std::lock_guard<std::mutex> guard(m_clMetricsMutex);
        ScenarioAccumulator& metric = m_mapScenarioMetrics[scenario.id];
        metric.scenarioId = scenario.id;
        metric.scenario = scenario.name;
        ++metric.dropped;
    }
    m_u64DroppedIterations.fetch_add(1);
}

// 关键实现：构造固定布局脱敏样本并限制保留数量，避免错误洪峰耗尽内存。
void LoadEngine::RecordError(const Scenario& scenario, const Step& step,
    const Request& request, const RequestOutcome& outcome)
{
    std::lock_guard<std::mutex> guard(m_clMetricsMutex);
    if (m_vecErrors.size() >= 1000) return;
    MTLOAD_ERROR_SAMPLE sample{};
    sample.structSize = sizeof(sample);
    sample.abiVersion = MTLOAD_ABI_VERSION;
    sample.timestampUnixMs = UnixMs();
    sample.runId = m_u64RunId;
    sample.result = outcome.result;
    sample.httpStatus = outcome.httpStatus;
    sample.elapsedMs = outcome.elapsedMs;
    CopyText(sample.scenario, scenario.name);
    CopyText(sample.step, step.name);
    CopyText(sample.label, request.label);
    CopyText(sample.category, outcome.httpStatus == 0 ? "transport" : "assertion");
    CopyText(sample.summary, outcome.error);
    m_vecErrors.push_back(sample);
}

// 关键实现：状态字段在 m_clStateMutex 下复制，热计数通过原子读取补齐同一快照。
int LoadEngine::GetState(MTLOAD_RUN_STATE& out) const
{
    std::lock_guard<std::mutex> guard(m_clStateMutex);
    out = {};
    out.structSize = sizeof(out);
    out.abiVersion = MTLOAD_ABI_VERSION;
    out.runId = m_u64RunId;
    out.state = m_iState;
    out.lastResult = m_iLastResult;
    out.startUnixMs = m_i64StartUnixMs;
    out.endUnixMs = m_i64EndUnixMs;
    out.activeScenarios = m_u32ActiveScenarios.load();
    out.activeUsers = m_u32ActiveUsers.load();
    out.activeRequests = m_u32ActiveRequests.load();
    out.activeConnections = m_u32ActiveRequests.load();
    out.totalSamples = m_u64TotalSamples.load();
    out.plannedRequests = m_u64PlannedRequests;
    const int64_t wallMs = m_i64StartUnixMs == 0 ? 0 :
        (m_i64EndUnixMs == 0 ? UnixMs() : m_i64EndUnixMs) - m_i64StartUnixMs;
    uint64_t pausedMs = m_u64PausedDurationMs;
    if (m_bPauseRequested.load())
    {
        pausedMs += static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - m_clPauseStarted).count());
    }
    out.effectiveElapsedMs = wallMs <= static_cast<int64_t>(pausedMs) ? 0 :
        static_cast<uint64_t>(wallMs - static_cast<int64_t>(pausedMs));
    const double seconds = (std::max)(0.001, out.effectiveElapsedMs / 1000.0);
    out.totalThroughputPerSec = out.totalSamples / seconds;
    out.peakActiveUsers = m_u32PeakActiveUsers.load();
    out.peakActiveRequests = m_u32PeakActiveRequests.load();
    out.scheduledIterations = m_u64ScheduledIterations.load();
    out.completedIterations = m_u64CompletedIterations.load();
    out.droppedIterations = m_u64DroppedIterations.load();
    return MTLOAD_OK;
}

// 关键实现：复制累加器后计算平均值、分位数、吞吐和 KiB/s，锁内不做排序。
std::vector<MTLOAD_METRIC_ROW> LoadEngine::GetMetrics() const
{
    std::vector<MTLOAD_METRIC_ROW> rows;
    int64_t start = 0;
    int64_t end = 0;
    uint64_t pausedMs = 0;
    {
        std::lock_guard<std::mutex> stateGuard(m_clStateMutex);
        start = m_i64StartUnixMs;
        end = m_i64EndUnixMs;
        pausedMs = m_u64PausedDurationMs;
        if (m_bPauseRequested.load())
        {
            pausedMs += static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - m_clPauseStarted).count());
        }
    }
    std::lock_guard<std::mutex> guard(m_clMetricsMutex);
    const int64_t wallMs = start == 0 ? 0 : (end == 0 ? UnixMs() : end) - start;
    const double seconds = (std::max)(0.001,
        static_cast<double>((std::max)(int64_t{ 0 }, wallMs - static_cast<int64_t>(pausedMs))) / 1000.0);
    for (const auto& item : m_mapMetrics)
    {
        const MetricAccumulator& source = item.second;
        MTLOAD_METRIC_ROW row{};
        row.structSize = sizeof(row);
        row.abiVersion = MTLOAD_ABI_VERSION;
        CopyText(row.scenario, source.scenario);
        CopyText(row.step, source.step);
        CopyText(row.label, source.label);
        CopyText(row.method, source.method);
        CopyText(row.path, source.path);
        row.succeeded = source.succeeded;
        row.failed = source.failed;
        row.samples = row.succeeded + row.failed;
        row.sentBytes = source.sentBytes;
        row.receivedBytes = source.receivedBytes;
        row.plannedRequests = source.plannedRequests;
        row.errorRate = row.samples == 0 ? 0.0 : static_cast<double>(row.failed) / row.samples;
        if (!source.latencyMs.empty())
        {
            row.minMs = *std::min_element(source.latencyMs.begin(), source.latencyMs.end());
            row.maxMs = *std::max_element(source.latencyMs.begin(), source.latencyMs.end());
            double total = 0;
            for (double value : source.latencyMs) total += value;
            row.averageMs = total / source.latencyMs.size();
            row.medianMs = Percentile(source.latencyMs, 0.50);
            row.p90Ms = Percentile(source.latencyMs, 0.90);
            row.p95Ms = Percentile(source.latencyMs, 0.95);
            row.p99Ms = Percentile(source.latencyMs, 0.99);
        }
        row.throughputPerSec = row.samples / seconds;
        row.sentKiBPerSec = row.sentBytes / 1024.0 / seconds;
        row.receivedKiBPerSec = row.receivedBytes / 1024.0 / seconds;
        rows.push_back(row);
    }
    return rows;
}

// 关键实现：在锁内复制事务累加器，在锁外按完整事务耗时计算分位数和吞吐。
std::vector<MTLOAD_SCENARIO_METRIC_ROW> LoadEngine::GetScenarioMetrics() const
{
    uint64_t elapsedMs = 0;
    {
        MTLOAD_RUN_STATE state{};
        GetState(state);
        elapsedMs = state.effectiveElapsedMs;
    }
    const double seconds = (std::max)(0.001, elapsedMs / 1000.0);
    std::lock_guard<std::mutex> guard(m_clMetricsMutex);
    std::vector<MTLOAD_SCENARIO_METRIC_ROW> rows;
    rows.reserve(m_mapScenarioMetrics.size());
    for (const auto& item : m_mapScenarioMetrics)
    {
        const ScenarioAccumulator& source = item.second;
        MTLOAD_SCENARIO_METRIC_ROW row{};
        row.structSize = sizeof(row);
        row.abiVersion = MTLOAD_ABI_VERSION;
        CopyText(row.scenarioId, source.scenarioId);
        CopyText(row.scenario, source.scenario);
        row.started = source.started;
        row.completed = source.completed;
        row.succeeded = source.succeeded;
        row.failed = source.failed;
        row.dropped = source.dropped;
        if (!source.latencyMs.empty())
        {
            row.minMs = *std::min_element(source.latencyMs.begin(), source.latencyMs.end());
            row.maxMs = *std::max_element(source.latencyMs.begin(), source.latencyMs.end());
            double total = 0.0;
            for (double value : source.latencyMs) total += value;
            row.averageMs = total / source.latencyMs.size();
            row.medianMs = Percentile(source.latencyMs, 0.50);
            row.p90Ms = Percentile(source.latencyMs, 0.90);
            row.p95Ms = Percentile(source.latencyMs, 0.95);
            row.p99Ms = Percentile(source.latencyMs, 0.99);
        }
        row.throughputPerSec = row.completed / seconds;
        rows.push_back(row);
    }
    return rows;
}

// 关键实现：阈值结果在运行终点一次性生成，查询只返回不可变 POD 副本。
std::vector<MTLOAD_THRESHOLD_RESULT> LoadEngine::GetThresholdResults() const
{
    std::lock_guard<std::mutex> guard(m_clMetricsMutex);
    return m_vecThresholdResults;
}

// 关键实现：从同一轮请求、事务和运行快照计算阈值；目标不存在按失败处理，
// 防止配置拼写错误被当作 0 值而意外通过。
bool LoadEngine::EvaluateThresholds(const Workload& workload)
{
    const auto requestRows = GetMetrics();
    const auto scenarioRows = GetScenarioMetrics();
    MTLOAD_RUN_STATE state{};
    GetState(state);
    uint64_t failedRequests = 0;
    for (const auto& row : requestRows) failedRequests += row.failed;
    std::vector<MTLOAD_THRESHOLD_RESULT> results;
    bool allPassed = true;
    for (const Threshold& threshold : workload.thresholds)
    {
        MTLOAD_THRESHOLD_RESULT result{};
        result.structSize = sizeof(result);
        result.abiVersion = MTLOAD_ABI_VERSION;
        CopyText(result.id, threshold.id);
        CopyText(result.name, threshold.name);
        CopyText(result.scope, threshold.scope);
        CopyText(result.metric, threshold.metric);
        CopyText(result.target, threshold.target);
        CopyText(result.comparison, threshold.comparison);
        result.expected = threshold.value;
        bool found = true;
        if (threshold.scope == "run")
        {
            if (threshold.metric == "errorRate")
                result.actual = state.totalSamples == 0 ? 0.0 :
                    static_cast<double>(failedRequests) / state.totalSamples;
            else if (threshold.metric == "throughputPerSec")
                result.actual = state.totalThroughputPerSec;
            else if (threshold.metric == "droppedIterations")
                result.actual = static_cast<double>(state.droppedIterations);
            else found = false;
        }
        else if (threshold.scope == "request")
        {
            found = false;
            for (const auto& row : requestRows)
            {
                if (threshold.target != row.label) continue;
                const double value = threshold.metric == "p95Ms" ? row.p95Ms : row.p99Ms;
                result.actual = found ? (std::max)(result.actual, value) : value;
                found = true;
            }
        }
        else if (threshold.scope == "scenario")
        {
            found = false;
            for (const auto& row : scenarioRows)
            {
                if (threshold.target != row.scenarioId) continue;
                result.actual = threshold.metric == "p95Ms" ? row.p95Ms :
                    (row.completed == 0 ? 0.0 : static_cast<double>(row.failed) / row.completed);
                found = true;
                break;
            }
        }
        const double epsilon = 1e-9;
        const bool passed = found && (threshold.comparison == "lte" ? result.actual <= result.expected :
            threshold.comparison == "gte" ? result.actual >= result.expected :
            std::abs(result.actual - result.expected) <= epsilon);
        result.passed = passed ? 1U : 0U;
        allPassed = allPassed && passed;
        results.push_back(result);
    }
    {
        std::lock_guard<std::mutex> guard(m_clMetricsMutex);
        m_vecThresholdResults = std::move(results);
    }
    return allPassed;
}

// 关键实现：在指标锁内返回错误样本副本，调用方不持有内部引用。
std::vector<MTLOAD_ERROR_SAMPLE> LoadEngine::GetErrors() const
{
    std::lock_guard<std::mutex> guard(m_clMetricsMutex);
    return m_vecErrors;
}

// 关键实现：使用同一轮状态和指标副本生成 JSON，避免跨锁长时间序列化。
std::string LoadEngine::GetSnapshotJson() const
{
    Json::Value root;
    MTLOAD_RUN_STATE state{};
    GetState(state);
    root["AbiVersion"] = MTLOAD_ABI_VERSION;
    root["RunId"] = std::to_string(state.runId);
    root["State"] = state.state;
    root["Result"] = state.lastResult;
    root["TotalSamples"] = std::to_string(state.totalSamples);
    root["PlannedRequests"] = std::to_string(state.plannedRequests);
    root["EffectiveElapsedMs"] = std::to_string(state.effectiveElapsedMs);
    root["TotalThroughputPerSec"] = state.totalThroughputPerSec;
    root["CurrentUsers"] = state.activeUsers;
    root["PeakUsers"] = state.peakActiveUsers;
    root["CurrentInflightRequests"] = state.activeRequests;
    root["PeakInflightRequests"] = state.peakActiveRequests;
    root["ScheduledIterations"] = std::to_string(state.scheduledIterations);
    root["CompletedIterations"] = std::to_string(state.completedIterations);
    root["DroppedIterations"] = std::to_string(state.droppedIterations);
    Json::Value metrics(Json::arrayValue);
    for (const auto& row : GetMetrics())
    {
        Json::Value item;
        item["Scenario"] = row.scenario;
        item["Step"] = row.step;
        item["Label"] = row.label;
        item["Samples"] = std::to_string(row.samples);
        item["Succeeded"] = std::to_string(row.succeeded);
        item["Failed"] = std::to_string(row.failed);
        item["PlannedRequests"] = std::to_string(row.plannedRequests);
        item["Progress"] = row.plannedRequests == 0 ? 0.0 :
            static_cast<double>(row.samples) / row.plannedRequests;
        item["AverageMs"] = row.averageMs;
        item["P95Ms"] = row.p95Ms;
        item["ThroughputPerSec"] = row.throughputPerSec;
        metrics.append(item);
    }
    root["Metrics"] = metrics;
    Json::Value transactions(Json::arrayValue);
    for (const auto& row : GetScenarioMetrics())
    {
        Json::Value item;
        item["ScenarioId"] = row.scenarioId;
        item["Scenario"] = row.scenario;
        item["Started"] = std::to_string(row.started);
        item["Completed"] = std::to_string(row.completed);
        item["Succeeded"] = std::to_string(row.succeeded);
        item["Failed"] = std::to_string(row.failed);
        item["Dropped"] = std::to_string(row.dropped);
        item["AverageMs"] = row.averageMs;
        item["P50Ms"] = row.medianMs;
        item["P90Ms"] = row.p90Ms;
        item["P95Ms"] = row.p95Ms;
        item["P99Ms"] = row.p99Ms;
        item["ThroughputPerSec"] = row.throughputPerSec;
        transactions.append(item);
    }
    root["Transactions"] = transactions;
    Json::Value thresholds(Json::arrayValue);
    for (const auto& row : GetThresholdResults())
    {
        Json::Value item;
        item["Id"] = row.id;
        item["Name"] = row.name;
        item["Scope"] = row.scope;
        item["Metric"] = row.metric;
        item["Target"] = row.target;
        item["Operator"] = row.comparison;
        item["Expected"] = row.expected;
        item["Actual"] = row.actual;
        item["Passed"] = row.passed == 1;
        thresholds.append(item);
    }
    root["Thresholds"] = thresholds;
    Json::StyledWriter writer;
    return writer.write(root);
}

// 关键实现：在状态锁内复制最近错误，返回后不依赖内部字符串生命周期。
std::string LoadEngine::GetLastError() const
{
    std::lock_guard<std::mutex> guard(m_clStateMutex);
    return m_strLastError;
}

// 关键实现：一次取得状态和指标快照，再按位掩码分别写出各格式文件。
int LoadEngine::Export(const std::string& directory, uint32_t formats)
{
    try
    {
        const std::filesystem::path root = std::filesystem::u8path(directory);
        std::filesystem::create_directories(root);
        if ((formats & MTLOAD_REPORT_JSON) != 0)
        {
            std::ofstream stream(root / "MtLoadResult.json", std::ios::binary | std::ios::trunc);
            if (!stream) return MTLOAD_E_REPORT;
            stream << GetSnapshotJson();
        }
        if ((formats & MTLOAD_REPORT_CSV) != 0)
        {
            std::ofstream stream(root / "MtLoadResult.csv", std::ios::binary | std::ios::trunc);
            if (!stream) return MTLOAD_E_REPORT;
            stream << "Scenario,Step,Label,Samples,PlannedRequests,Progress,Succeeded,Failed,ErrorRate,AverageMs,P90Ms,P95Ms,P99Ms,ThroughputPerSec\r\n";
            for (const auto& row : GetMetrics())
            {
                stream << CsvEscape(row.scenario) << ',' << CsvEscape(row.step) << ','
                    << CsvEscape(row.label) << ',' << row.samples << ',' << row.plannedRequests << ','
                    << (row.plannedRequests == 0 ? 0.0 : static_cast<double>(row.samples) / row.plannedRequests) << ','
                    << row.succeeded << ',' << row.failed << ',' << row.errorRate << ',' << row.averageMs << ','
                    << row.p90Ms << ',' << row.p95Ms << ',' << row.p99Ms << ','
                    << row.throughputPerSec << "\r\n";
            }
        }
        if ((formats & MTLOAD_REPORT_XML) != 0)
            WriteXmlReport(root / "MtLoadResult.xml", [&]() { MTLOAD_RUN_STATE value{}; GetState(value); return value; }(),
                GetMetrics(), GetScenarioMetrics(), GetThresholdResults(), GetErrors());
        if ((formats & MTLOAD_REPORT_XLSX) != 0)
            WriteXlsxReport(root / "MtLoadResult.xlsx", [&]() { MTLOAD_RUN_STATE value{}; GetState(value); return value; }(),
                GetMetrics(), GetScenarioMetrics(), GetThresholdResults(), GetErrors());
        return MTLOAD_OK;
    }
    catch (...) { return MTLOAD_E_REPORT; }
}

// 关键实现：根据单一格式选择序列化器，异常统一转换为 MTLOAD_E_REPORT。
int LoadEngine::ExportFile(const std::string& absoluteFilePath, uint32_t format)
{
    if (format != MTLOAD_REPORT_XLSX && format != MTLOAD_REPORT_XML)
        return MTLOAD_E_UNSUPPORTED;
    try
    {
        const std::filesystem::path path = std::filesystem::absolute(
            std::filesystem::u8path(absoluteFilePath));
        if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path());
        MTLOAD_RUN_STATE state{};
        GetState(state);
        const auto metrics = GetMetrics();
        const auto scenarios = GetScenarioMetrics();
        const auto thresholds = GetThresholdResults();
        const auto errors = GetErrors();
        if (format == MTLOAD_REPORT_XLSX) WriteXlsxReport(path, state, metrics, scenarios, thresholds, errors);
        else WriteXmlReport(path, state, metrics, scenarios, thresholds, errors);
        return MTLOAD_OK;
    }
    catch (...) { return MTLOAD_E_REPORT; }
}

// 关键实现：解析计划相对目录并生成固定时间戳文件名，再复用单文件导出。
int LoadEngine::ExportConfigured(uint32_t format, std::string& outputPath)
{
    if (format != MTLOAD_REPORT_XLSX && format != MTLOAD_REPORT_XML)
        return MTLOAD_E_UNSUPPORTED;
    std::filesystem::path planPath;
    std::string configuredDirectory;
    {
        std::lock_guard<std::mutex> guard(m_clStateMutex);
        if (!m_bHasPlan || m_u64TotalSamples.load() == 0 || m_iState == MTLOAD_STATE_STARTING ||
            m_iState == MTLOAD_STATE_RUNNING || m_iState == MTLOAD_STATE_PAUSING ||
            m_iState == MTLOAD_STATE_PAUSED || m_iState == MTLOAD_STATE_STOPPING)
            return MTLOAD_E_INVALID_STATE;
        planPath = std::filesystem::u8path(m_stPlan.sourcePath);
        configuredDirectory = m_stPlan.reportDirectory;
    }
    std::filesystem::path directory = planPath.parent_path();
    if (!configuredDirectory.empty())
    {
        const std::filesystem::path configured = std::filesystem::u8path(configuredDirectory);
        directory = configured.is_absolute() ? configured : directory / configured;
    }
    const std::string extension = format == MTLOAD_REPORT_XLSX ? ".xlsx" : ".xml";
    const std::filesystem::path path = directory /
        std::filesystem::u8path("LoadTest_" + ReportTimestamp() + extension);
    const int result = ExportFile(path.u8string(), format);
    if (result == MTLOAD_OK) outputPath = path.u8string();
    return result;
}
}
