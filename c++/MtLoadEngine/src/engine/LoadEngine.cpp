#include "LoadEngine.h"

#include <json/json.h>
#include <xlnt/xlnt.hpp>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <ctime>
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
    const std::vector<MTLOAD_ERROR_SAMPLE>& errors)
{
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) throw std::runtime_error("无法创建 XML 报告");
    stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n"
        << "<压测结果 运行编号=\"" << state.runId << "\" 状态=\"" << state.state
        << "\" 已请求=\"" << state.totalSamples << "\" 总请求=\""
        << state.plannedRequests << "\">\r\n  <指标列表>\r\n";
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
    stream << "  </指标列表>\r\n  <错误列表>\r\n";
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
    const std::vector<MTLOAD_ERROR_SAMPLE>& errors)
{
    xlnt::workbook workbook;
    xlnt::worksheet summary = workbook.active_sheet();
    summary.title(u8"汇总");
    const char* summaryNames[] = { u8"运行编号", u8"状态", u8"已请求", u8"总请求" };
    const std::string summaryValues[] = { std::to_string(state.runId), std::to_string(state.state),
        std::to_string(state.totalSamples), std::to_string(state.plannedRequests) };
    for (int index = 0; index < 4; ++index)
    {
        summary.cell(xlnt::cell_reference(1, index + 1)).value(summaryNames[index]);
        summary.cell(xlnt::cell_reference(2, index + 1)).value(summaryValues[index]);
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

// 汇总所选场景的有限计划请求数；选择为空时覆盖全部场景。
uint64_t PlannedRequests(const Plan& plan, const std::string& selectedScenarioId)
{
    uint64_t total = 0;
    for (const Scenario& scenario : plan.scenarios)
    {
        if (!selectedScenarioId.empty() && scenario.id != selectedScenarioId) continue;
        if (scenario.load.durationMs != 0 || scenario.load.iterations == 0) return 0;
        for (const Step& step : scenario.steps)
            for (const Request& request : step.requests)
                total += PlannedRequests(scenario, request);
    }
    return total;
}
}

// 关键实现：容量至少为 1，防止错误配置导致所有工作线程永久等待。
PermitPool::PermitPool(uint32_t capacity) : available_((std::max)(1U, capacity)) {}

// 关键实现：在条件变量上同时等待许可或停止，成功时在锁内扣减许可。
bool PermitPool::Acquire(const std::atomic<bool>& stopping)
{
    std::unique_lock<std::mutex> lock(mutex_);
    changed_.wait(lock, [&]() { return available_ > 0 || stopping.load(); });
    if (stopping.load()) return false;
    --available_;
    return true;
}

// 关键实现：在锁内归还许可并只唤醒一个竞争线程。
void PermitPool::Release()
{
    std::lock_guard<std::mutex> guard(mutex_);
    ++available_;
    changed_.notify_one();
}

// 关键实现：停机时唤醒全部等待者，让其重新检查 stopping。
void PermitPool::WakeAll()
{
    changed_.notify_all();
}

// 关键实现：复制固定选项并按 maxConnections 建立全实例许可池。
LoadEngine::LoadEngine(const MTLOAD_ENGINE_OPTIONS& options)
    : options_(options), permits_(options.maxConnections)
{
}

// 关键实现：复用 Stop 并联结顶层线程，析构返回时不再有线程访问成员。
LoadEngine::~LoadEngine()
{
    Stop();
    Wait(UINT32_MAX);
    if (worker_.joinable()) worker_.join();
}

// 关键实现：在状态锁内同步保存结果码和诊断，供 ABI 随后查询。
void LoadEngine::SetError(int result, const std::string& error)
{
    std::lock_guard<std::mutex> guard(stateMutex_);
    lastResult_ = result;
    lastError_ = error;
}

// 关键实现：先在锁外解析局部计划，再在状态锁内一次性替换当前模型。
int LoadEngine::LoadPlanFile(const std::string& path)
{
    bool reapCompletedWorker = false;
    {
        std::lock_guard<std::mutex> guard(stateMutex_);
        if (state_ == MTLOAD_STATE_STARTING || state_ == MTLOAD_STATE_RUNNING ||
            state_ == MTLOAD_STATE_PAUSING || state_ == MTLOAD_STATE_PAUSED ||
            state_ == MTLOAD_STATE_STOPPING)
            return MTLOAD_E_INVALID_STATE;
        // 终态只表示工作已结束；std::thread 仍需由控制线程 join 后才能启动下一轮。
        reapCompletedWorker = worker_.joinable();
    }
    if (reapCompletedWorker) worker_.join();

    Plan parsed;
    std::string error;
    const int result = ParsePlan(path, parsed, error);
    if (result != MTLOAD_OK)
    {
        SetError(result, error);
        return result;
    }
    std::lock_guard<std::mutex> guard(stateMutex_);
    plan_ = std::move(parsed);
    selectedEnvironmentId_ = plan_.defaultEnvironmentId;
    selectedScenarioId_.clear();
    selectedUserId_.clear();
    hasPlan_ = true;
    validated_ = false;
    state_ = MTLOAD_STATE_PLAN_LOADED;
    lastResult_ = MTLOAD_OK;
    lastError_.clear();
    startUnixMs_ = 0;
    endUnixMs_ = 0;
    plannedRequests_ = 0;
    pausedDurationMs_ = 0;
    pauseRequested_.store(false);
    totalSamples_.store(0);
    {
        std::lock_guard<std::mutex> metricGuard(metricsMutex_);
        metrics_.clear();
        errors_.clear();
    }
    return MTLOAD_OK;
}

// 关键实现：在锁外执行语义校验，成功后初始化默认选择和零值指标行。
int LoadEngine::Validate()
{
    Plan copy;
    {
        std::lock_guard<std::mutex> guard(stateMutex_);
        if (!hasPlan_ || state_ != MTLOAD_STATE_PLAN_LOADED) return MTLOAD_E_INVALID_STATE;
        copy = plan_;
    }
    std::string error;
    const int result = ValidatePlan(copy, error);
    {
        std::lock_guard<std::mutex> guard(stateMutex_);
        if (result == MTLOAD_OK)
        {
            validated_ = true;
            state_ = MTLOAD_STATE_VALIDATED;
            plannedRequests_ = PlannedRequests(copy, selectedScenarioId_);
            lastError_.clear();
        }
        else
        {
            validated_ = false;
            plannedRequests_ = 0;
            lastError_ = error;
        }
        lastResult_ = result;
    }
    if (result == MTLOAD_OK) ResetMetricDefinitions(copy, selectedScenarioId_);
    return result;
}

// 关键实现：在状态锁内把环境模型转换为不含业务敏感值的 ABI POD。
std::vector<MTLOAD_ENVIRONMENT> LoadEngine::GetEnvironments() const
{
    std::lock_guard<std::mutex> guard(stateMutex_);
    std::vector<MTLOAD_ENVIRONMENT> rows;
    rows.reserve(plan_.environments.size());
    for (const auto& pair : plan_.environments)
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
        row.selected = source.id == selectedEnvironmentId_ ? 1U : 0U;
        rows.push_back(row);
    }
    return rows;
}

// 关键实现：仅在非活动状态接受存在的环境 ID，并保持计划内容不变。
int LoadEngine::SelectEnvironment(const std::string& environmentId)
{
    std::lock_guard<std::mutex> guard(stateMutex_);
    if (!hasPlan_ || environmentId.empty()) return MTLOAD_E_INVALID_ARGUMENT;
    if (state_ == MTLOAD_STATE_STARTING || state_ == MTLOAD_STATE_RUNNING ||
        state_ == MTLOAD_STATE_PAUSING || state_ == MTLOAD_STATE_PAUSED ||
        state_ == MTLOAD_STATE_STOPPING)
        return MTLOAD_E_INVALID_STATE;
    if (plan_.environments.find(environmentId) == plan_.environments.end())
    {
        lastResult_ = MTLOAD_E_NOT_FOUND;
        lastError_ = "环境不存在: " + environmentId;
        return lastResult_;
    }
    selectedEnvironmentId_ = environmentId;
    lastResult_ = MTLOAD_OK;
    lastError_.clear();
    return MTLOAD_OK;
}

// 关键实现：复制场景负载参数并预计算单循环和总请求数。
std::vector<MTLOAD_SCENARIO> LoadEngine::GetScenarios() const
{
    std::lock_guard<std::mutex> guard(stateMutex_);
    std::vector<MTLOAD_SCENARIO> rows;
    rows.reserve(plan_.scenarios.size());
    for (const Scenario& source : plan_.scenarios)
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
        row.selected = source.id == selectedScenarioId_ ? 1U : 0U;
        rows.push_back(row);
    }
    return rows;
}

// 关键实现：切换场景后重建零值指标定义，界面可立即看到完整步骤列表。
int LoadEngine::SelectScenario(const std::string& scenarioId)
{
    Plan planCopy;
    {
        std::lock_guard<std::mutex> guard(stateMutex_);
        if (!validated_) return MTLOAD_E_INVALID_STATE;
        if (state_ == MTLOAD_STATE_STARTING || state_ == MTLOAD_STATE_RUNNING ||
            state_ == MTLOAD_STATE_PAUSING || state_ == MTLOAD_STATE_PAUSED ||
            state_ == MTLOAD_STATE_STOPPING)
            return MTLOAD_E_INVALID_STATE;
        if (!scenarioId.empty() && std::none_of(plan_.scenarios.begin(), plan_.scenarios.end(),
            [&](const Scenario& scenario) { return scenario.id == scenarioId; }))
        {
            lastResult_ = MTLOAD_E_NOT_FOUND;
            lastError_ = "场景不存在: " + scenarioId;
            return lastResult_;
        }
        selectedScenarioId_ = scenarioId;
        plannedRequests_ = PlannedRequests(plan_, selectedScenarioId_);
        lastResult_ = MTLOAD_OK;
        lastError_.clear();
        planCopy = plan_;
    }
    ResetMetricDefinitions(planCopy, scenarioId);
    return MTLOAD_OK;
}

// 关键实现：只复制 Version/No/Login 等摘要，密码和 Param 永不跨 ABI。
std::vector<MTLOAD_USER> LoadEngine::GetUsers() const
{
    std::lock_guard<std::mutex> guard(stateMutex_);
    std::vector<MTLOAD_USER> rows;
    rows.reserve(plan_.users.size());
    for (const auto& pair : plan_.users)
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
        row.selected = source.id == selectedUserId_ ? 1U : 0U;
        rows.push_back(row);
    }
    return rows;
}

// 关键实现：保存统一用户覆盖 ID；空字符串恢复各场景 UserRefs。
int LoadEngine::SelectUser(const std::string& userId)
{
    std::lock_guard<std::mutex> guard(stateMutex_);
    if (!validated_) return MTLOAD_E_INVALID_STATE;
    if (state_ == MTLOAD_STATE_STARTING || state_ == MTLOAD_STATE_RUNNING ||
        state_ == MTLOAD_STATE_PAUSING || state_ == MTLOAD_STATE_PAUSED ||
        state_ == MTLOAD_STATE_STOPPING)
        return MTLOAD_E_INVALID_STATE;
    if (!userId.empty() && plan_.users.find(userId) == plan_.users.end())
    {
        lastResult_ = MTLOAD_E_NOT_FOUND;
        lastError_ = "用户不存在: " + userId;
        return lastResult_;
    }
    selectedUserId_ = userId;
    lastResult_ = MTLOAD_OK;
    lastError_.clear();
    return MTLOAD_OK;
}

// 关键实现：清理上一轮、建立新 runId 和指标定义后再发布 STARTING 并创建线程。
int LoadEngine::Start(const std::string& runName)
{
    // 先在锁内判断上一轮是否可回收，再在锁外 join，避免工作线程结束时反向等待状态锁。
    bool reapCompletedWorker = false;
    {
        std::lock_guard<std::mutex> guard(stateMutex_);
        const bool restartable = state_ == MTLOAD_STATE_VALIDATED ||
            state_ == MTLOAD_STATE_COMPLETED || state_ == MTLOAD_STATE_FAILED;
        if (!validated_ || !restartable) return MTLOAD_E_INVALID_STATE;
        reapCompletedWorker = worker_.joinable();
    }
    if (reapCompletedWorker) worker_.join();

    // 重新取得锁后二次确认状态，并原子建立完整的新一轮运行上下文。
    std::lock_guard<std::mutex> guard(stateMutex_);
    const bool restartable = state_ == MTLOAD_STATE_VALIDATED ||
        state_ == MTLOAD_STATE_COMPLETED || state_ == MTLOAD_STATE_FAILED;
    if (!validated_ || !restartable || worker_.joinable()) return MTLOAD_E_INVALID_STATE;
    runName_ = runName.empty() ? plan_.name : runName;
    ++runId_;
    startUnixMs_ = UnixMs();
    endUnixMs_ = 0;
    lastResult_ = MTLOAD_OK;
    lastError_.clear();
    state_ = MTLOAD_STATE_STARTING;
    stopping_.store(false);
    pauseRequested_.store(false);
    pausedDurationMs_ = 0;
    totalSamples_.store(0);
    ResetMetricDefinitions(plan_, selectedScenarioId_);
    try
    {
        worker_ = std::thread(&LoadEngine::Run, this);
    }
    catch (const std::exception& exception)
    {
        state_ = MTLOAD_STATE_FAILED;
        lastResult_ = MTLOAD_E_START_FAILED;
        lastError_ = std::string("创建运行线程失败: ") + exception.what();
        endUnixMs_ = UnixMs();
        return lastResult_;
    }
    return MTLOAD_OK;
}

// 关键实现：只设置暂停标记和 PAUSING，具体 PAUSED 在调度线程到达边界时确认。
int LoadEngine::Pause()
{
    std::lock_guard<std::mutex> pauseGuard(pauseMutex_);
    {
        std::lock_guard<std::mutex> guard(stateMutex_);
        if (state_ == MTLOAD_STATE_PAUSING || state_ == MTLOAD_STATE_PAUSED) return MTLOAD_OK;
        if (state_ != MTLOAD_STATE_STARTING && state_ != MTLOAD_STATE_RUNNING)
            return MTLOAD_E_INVALID_STATE;
        pauseRequested_.store(true);
        pauseStarted_ = std::chrono::steady_clock::now();
        state_ = activeRequests_.load() == 0 ? MTLOAD_STATE_PAUSED : MTLOAD_STATE_PAUSING;
    }
    pauseChanged_.notify_all();
    stateChanged_.notify_all();
    return MTLOAD_OK;
}

// 关键实现：累计本次暂停时长、清除标记并唤醒所有虚拟用户。
int LoadEngine::Resume()
{
    {
        std::lock_guard<std::mutex> guard(stateMutex_);
        if (state_ == MTLOAD_STATE_RUNNING || state_ == MTLOAD_STATE_STARTING) return MTLOAD_OK;
        if (state_ != MTLOAD_STATE_PAUSING && state_ != MTLOAD_STATE_PAUSED)
            return MTLOAD_E_INVALID_STATE;
        if (pauseRequested_.exchange(false))
        {
            pausedDurationMs_ += static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - pauseStarted_).count());
        }
        state_ = MTLOAD_STATE_RUNNING;
    }
    pauseChanged_.notify_all();
    stateChanged_.notify_all();
    return MTLOAD_OK;
}

// 关键实现：幂等发布 STOPPING，依次唤醒暂停门禁和并发许可等待者。
int LoadEngine::Stop()
{
    {
        std::lock_guard<std::mutex> guard(stateMutex_);
        if (state_ == MTLOAD_STATE_STARTING || state_ == MTLOAD_STATE_RUNNING ||
            state_ == MTLOAD_STATE_PAUSING || state_ == MTLOAD_STATE_PAUSED)
            state_ = MTLOAD_STATE_STOPPING;
        else if (state_ != MTLOAD_STATE_STOPPING) return MTLOAD_OK;
        if (pauseRequested_.exchange(false))
        {
            pausedDurationMs_ += static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - pauseStarted_).count());
        }
    }
    stopping_.store(true);
    permits_.WakeAll();
    pauseChanged_.notify_all();
    stateChanged_.notify_all();
    return MTLOAD_OK;
}

// 关键实现：只在稳定非活动状态清空结果，随后恢复与计划校验状态一致的状态值。
int LoadEngine::ClearResults()
{
    std::lock_guard<std::mutex> guard(stateMutex_);
    if (state_ == MTLOAD_STATE_STARTING || state_ == MTLOAD_STATE_RUNNING ||
        state_ == MTLOAD_STATE_PAUSING || state_ == MTLOAD_STATE_PAUSED ||
        state_ == MTLOAD_STATE_STOPPING)
        return MTLOAD_E_INVALID_STATE;
    startUnixMs_ = 0;
    endUnixMs_ = 0;
    pausedDurationMs_ = 0;
    totalSamples_.store(0);
    lastResult_ = MTLOAD_OK;
    lastError_.clear();
    std::lock_guard<std::mutex> metricGuard(metricsMutex_);
    metrics_.clear();
    errors_.clear();
    return MTLOAD_OK;
}

// 关键实现：在 stateChanged_ 上等待终态谓词，有限等待超时不改变运行状态。
int LoadEngine::Wait(uint32_t timeoutMs)
{
    std::unique_lock<std::mutex> lock(stateMutex_);
    // 未启动的 CREATED/PLAN_LOADED/VALIDATED 实例没有工作线程，应立即返回。
    auto terminal = [&]() { return !worker_.joinable() ||
        state_ == MTLOAD_STATE_COMPLETED || state_ == MTLOAD_STATE_FAILED; };
    bool done = true;
    if (!terminal())
    {
        if (timeoutMs == UINT32_MAX) stateChanged_.wait(lock, terminal);
        else done = stateChanged_.wait_for(lock, std::chrono::milliseconds(timeoutMs), terminal);
    }
    lock.unlock();
    if (!done) return MTLOAD_E_TIMEOUT;
    if (worker_.joinable()) worker_.join();
    return MTLOAD_OK;
}

// 关键实现：在状态锁内写入终态和结束时间，再广播唤醒所有等待者。
void LoadEngine::SetTerminalState(int state, int result)
{
    {
        std::lock_guard<std::mutex> guard(stateMutex_);
        if (pauseRequested_.exchange(false))
        {
            pausedDurationMs_ += static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - pauseStarted_).count());
        }
        state_ = state;
        lastResult_ = result;
        endUnixMs_ = UnixMs();
    }
    pauseChanged_.notify_all();
    stateChanged_.notify_all();
}

// 关键实现：复制所选场景后逐一执行，捕获所有异常并确保最终只设置一次终态。
void LoadEngine::Run()
{
    try
    {
        // 进入工作线程后才发布 RUNNING，保证 STARTING 表示线程尚未真正接管执行。
        {
            std::lock_guard<std::mutex> guard(stateMutex_);
            if (state_ == MTLOAD_STATE_STARTING) state_ = MTLOAD_STATE_RUNNING;
        }
        stateChanged_.notify_all();
        // 每个选中场景独立调度，顶层线程统一 join 后再发布终态。
        std::vector<std::thread> scenarios;
        for (const Scenario& scenario : plan_.scenarios)
        {
            if (!selectedScenarioId_.empty() && scenario.id != selectedScenarioId_) continue;
            scenarios.emplace_back(&LoadEngine::RunScenario, this, std::cref(scenario));
        }
        for (auto& thread : scenarios) thread.join();
        SetTerminalState(MTLOAD_STATE_COMPLETED, MTLOAD_OK);
    }
    catch (const std::exception& exception)
    {
        {
            std::lock_guard<std::mutex> guard(stateMutex_);
            lastError_ = std::string("运行线程异常: ") + exception.what();
        }
        SetTerminalState(MTLOAD_STATE_FAILED, MTLOAD_E_INTERNAL);
    }
}

// 关键实现：一个线程对应一个虚拟用户，ramp-up 仅延迟启动，不共享变量上下文。
void LoadEngine::RunScenario(const Scenario& scenario)
{
    activeScenarios_.fetch_add(1);
    // 每个用户线程拥有独立迭代位置和变量上下文，相同账号也不共享可变会话值。
    std::vector<std::thread> users;
    for (uint32_t index = 0; index < scenario.load.threads && !stopping_.load(); ++index)
    {
        users.emplace_back([&, index]()
        {
            activeUsers_.fetch_add(1);
            // 栈守卫保证提前停止、延迟取消和异常路径均能归还活动用户计数。
            struct UserGuard
            {
                std::atomic<uint32_t>& count; // 借用 LoadEngine 活动用户计数器。
                // 用户线程离开作用域时只执行一次计数回收。
                ~UserGuard() { count.fetch_sub(1); }
            } userGuard{ activeUsers_ };
            if (scenario.load.rampUpMs > 0 && scenario.load.threads > 1)
            {
                const uint64_t delayMs = static_cast<uint64_t>(scenario.load.rampUpMs) *
                    index / scenario.load.threads;
                if (!WaitForActiveDelay(delayMs)) return;
            }
            if (!WaitUntilRunnable()) return;
            // 持续时间模式使用单调时钟，并扣除暂停区间。
            const auto started = std::chrono::steady_clock::now();
            const uint64_t pausedAtStart = PausedDurationMs();
            uint64_t iteration = 0;
            while (!stopping_.load())
            {
                if (scenario.load.iterations > 0 && iteration >= scenario.load.iterations) break;
                if (scenario.load.durationMs > 0)
                {
                    const uint64_t wallMs = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - started).count());
                    const uint64_t pausedMs = PausedDurationMs() - pausedAtStart;
                    if (wallMs >= pausedMs + scenario.load.durationMs) break;
                }
                if (!WaitUntilRunnable()) break;
                RunIteration(scenario, index, iteration++);
            }
        });
    }
    for (auto& user : users) user.join();
    activeScenarios_.fetch_sub(1);
}

// 关键实现：为当前用户选择账号/数据行，步骤串行、步骤内请求副本并行并等待屏障。
bool LoadEngine::RunIteration(const Scenario& scenario, uint32_t userIndex, uint64_t iteration)
{
    // 为本轮选择统一用户、UserRefs 或数据行，复制后交给该用户独占。
    std::map<std::string, std::string> row;
    if (!selectedUserId_.empty())
    {
        row = plan_.users.at(selectedUserId_).variables;
    }
    else if (!scenario.userIds.empty())
    {
        const std::string& userId = scenario.userIds[userIndex % scenario.userIds.size()];
        row = plan_.users.at(userId).variables;
    }
    else if (!scenario.dataSetId.empty())
    {
        const DataSet& dataSet = plan_.dataSets.at(scenario.dataSetId);
        if (!dataSet.rows.empty()) row = dataSet.rows[(static_cast<uint64_t>(userIndex) + iteration) % dataSet.rows.size()];
    }
    // 运行时标识由引擎生成并放入只读作用域，保证同账号并发线程和循环的数据键互不覆盖。
    std::map<std::string, std::string> runtimeVariables;
    runtimeVariables["runId"] = std::to_string(runId_);
    runtimeVariables["threadIndex"] = std::to_string(static_cast<uint64_t>(userIndex) + 1ULL);
    runtimeVariables["iterationIndex"] = std::to_string(iteration + 1ULL);
    VariableContext variables(row, std::move(runtimeVariables));
    bool iterationFailed = false;
    // 步骤严格串行；业务失败后跳过普通步骤，但仍扫描并执行 alwaysRun 清理步骤。
    for (const Step& step : scenario.steps)
    {
        if (stopping_.load()) return false;
        if (iterationFailed && !step.alwaysRun) continue;
        if (!WaitUntilRunnable()) return false;
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
        if (!stepOk && step.onError != "continue") iterationFailed = true;
    }
    return !iterationFailed;
}

// 关键实现：展开路径/正文/请求头，取得并发许可后执行 HTTP，再统一断言、提取和统计。
LoadEngine::RequestOutcome LoadEngine::RunRequest(const Scenario& scenario,
    const Step& step, const Request& request,
    const std::map<std::string, std::string>&, const VariableContext& variables)
{
    RequestOutcome outcome;
    // 先取得全实例并发许可；栈守卫覆盖全部早退路径并自动归还。
    if (!permits_.Acquire(stopping_))
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
    } guard{ permits_ };
    if (!BeginRequest())
    {
        outcome.result = MTLOAD_E_INVALID_STATE;
        outcome.error = "运行已停止";
        return outcome;
    }
    // 在途计数采用独立作用域守卫，避免变量展开失败时遗留 activeRequests_。
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
    const Target target = ResolveTarget(plan_.targets.at(request.targetId));
    const std::string targetPath = JoinPath(target.basePath, path);
    HttpResponse response = ExecuteHttp(target, expandedRequest, plan_.headers, targetPath, body,
        options_.maxResponseBytes, stopping_);
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
    std::lock_guard<std::mutex> guard(stateMutex_);
    if (plan_.environments.empty())
    {
        if (resolved.port == 0) resolved.port = resolved.transport == "https" ? 443 : 80;
        return resolved;
    }
    const Environment& environment = plan_.environments.at(selectedEnvironmentId_);
    resolved.transport = environment.scheme;
    resolved.host = environment.host;
    resolved.port = environment.port != 0 ? environment.port :
        (resolved.transport == "https" ? 443 : 80);
    resolved.basePath = environment.basePath;
    resolved.tlsCaFile = environment.tlsCaFile;
    resolved.tlsVerify = environment.tlsVerify;
    return resolved;
}

// 关键实现：预创建所有选中请求的累加器，并设置各自计划请求数。
void LoadEngine::ResetMetricDefinitions(const Plan& plan, const std::string& selectedScenarioId)
{
    std::lock_guard<std::mutex> guard(metricsMutex_);
    metrics_.clear();
    errors_.clear();
    for (const Scenario& scenario : plan.scenarios)
    {
        if (!selectedScenarioId.empty() && scenario.id != selectedScenarioId) continue;
        for (const Step& step : scenario.steps)
        {
            for (const Request& request : step.requests)
            {
                MetricAccumulator& metric = metrics_[MetricKey(scenario, step, request)];
                metric.scenario = scenario.name;
                metric.step = step.name;
                metric.label = request.label;
                metric.method = request.method;
                metric.path = request.path;
                metric.plannedRequests = PlannedRequests(scenario, request);
            }
        }
    }
}

// 关键实现：暂停时在条件变量上等待继续或停止，并由到达线程确认 PAUSED 状态。
bool LoadEngine::WaitUntilRunnable()
{
    std::unique_lock<std::mutex> lock(pauseMutex_);
    pauseChanged_.wait(lock, [this]() { return !pauseRequested_.load() || stopping_.load(); });
    return !stopping_.load();
}

// 关键实现：在暂停门禁内登记在途请求，防止 Pause 漏掉刚放行的请求。
bool LoadEngine::BeginRequest()
{
    std::unique_lock<std::mutex> lock(pauseMutex_);
    pauseChanged_.wait(lock, [this]() { return !pauseRequested_.load() || stopping_.load(); });
    if (stopping_.load()) return false;
    // 与 Pause 共用 pauseMutex_，保证暂停观察到的 activeRequests_ 不漏掉刚放行的请求。
    activeRequests_.fetch_add(1);
    return true;
}

// 关键实现：用单调时钟累计“活动时间”，暂停区间不消耗 ramp-up 延迟。
bool LoadEngine::WaitForActiveDelay(uint64_t delayMs)
{
    uint64_t remainingMs = delayMs;
    while (remainingMs > 0 && !stopping_.load())
    {
        if (!WaitUntilRunnable()) return false;
        const auto started = std::chrono::steady_clock::now();
        std::unique_lock<std::mutex> lock(pauseMutex_);
        pauseChanged_.wait_for(lock, std::chrono::milliseconds(remainingMs),
            [this]() { return pauseRequested_.load() || stopping_.load(); });
        const uint64_t elapsedMs = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started).count());
        remainingMs = elapsedMs >= remainingMs ? 0 : remainingMs - elapsedMs;
    }
    return !stopping_.load();
}

// 关键实现：减少在途计数；最后一个在途请求结束时把 PAUSING 确认为 PAUSED。
void LoadEngine::FinishRequest()
{
    const uint32_t remaining = activeRequests_.fetch_sub(1) - 1;
    if (remaining != 0 || !pauseRequested_.load()) return;
    {
        std::lock_guard<std::mutex> guard(stateMutex_);
        if (state_ == MTLOAD_STATE_PAUSING) state_ = MTLOAD_STATE_PAUSED;
    }
    stateChanged_.notify_all();
}

// 关键实现：在暂停锁内合并历史暂停时长和当前未结束区间。
uint64_t LoadEngine::PausedDurationMs() const
{
    std::lock_guard<std::mutex> guard(stateMutex_);
    uint64_t result = pausedDurationMs_;
    if (pauseRequested_.load())
    {
        result += static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - pauseStarted_).count());
    }
    return result;
}

// 关键实现：所有样本字段在同一 metricsMutex_ 临界区更新，保持统计口径一致。
void LoadEngine::RecordMetric(const Scenario& scenario, const Step& step,
    const Request& request, bool success, double elapsedMs,
    uint64_t sentBytes, uint64_t receivedBytes)
{
    std::lock_guard<std::mutex> guard(metricsMutex_);
    MetricAccumulator& metric = metrics_[MetricKey(scenario, step, request)];
    metric.scenario = scenario.name;
    metric.step = step.name;
    metric.label = request.label;
    metric.method = request.method;
    metric.path = request.path;
    success ? ++metric.succeeded : ++metric.failed;
    metric.sentBytes += sentBytes;
    metric.receivedBytes += receivedBytes;
    metric.latencyMs.push_back(elapsedMs);
    totalSamples_.fetch_add(1);
}

// 关键实现：构造固定布局脱敏样本并限制保留数量，避免错误洪峰耗尽内存。
void LoadEngine::RecordError(const Scenario& scenario, const Step& step,
    const Request& request, const RequestOutcome& outcome)
{
    std::lock_guard<std::mutex> guard(metricsMutex_);
    if (errors_.size() >= 1000) return;
    MTLOAD_ERROR_SAMPLE sample{};
    sample.structSize = sizeof(sample);
    sample.abiVersion = MTLOAD_ABI_VERSION;
    sample.timestampUnixMs = UnixMs();
    sample.runId = runId_;
    sample.result = outcome.result;
    sample.httpStatus = outcome.httpStatus;
    sample.elapsedMs = outcome.elapsedMs;
    CopyText(sample.scenario, scenario.name);
    CopyText(sample.step, step.name);
    CopyText(sample.label, request.label);
    CopyText(sample.category, outcome.httpStatus == 0 ? "transport" : "assertion");
    CopyText(sample.summary, outcome.error);
    errors_.push_back(sample);
}

// 关键实现：状态字段在 stateMutex_ 下复制，热计数通过原子读取补齐同一快照。
int LoadEngine::GetState(MTLOAD_RUN_STATE& out) const
{
    std::lock_guard<std::mutex> guard(stateMutex_);
    out = {};
    out.structSize = sizeof(out);
    out.abiVersion = MTLOAD_ABI_VERSION;
    out.runId = runId_;
    out.state = state_;
    out.lastResult = lastResult_;
    out.startUnixMs = startUnixMs_;
    out.endUnixMs = endUnixMs_;
    out.activeScenarios = activeScenarios_.load();
    out.activeUsers = activeUsers_.load();
    out.activeRequests = activeRequests_.load();
    out.activeConnections = activeRequests_.load();
    out.totalSamples = totalSamples_.load();
    out.plannedRequests = plannedRequests_;
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
        std::lock_guard<std::mutex> stateGuard(stateMutex_);
        start = startUnixMs_;
        end = endUnixMs_;
        pausedMs = pausedDurationMs_;
        if (pauseRequested_.load())
        {
            pausedMs += static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - pauseStarted_).count());
        }
    }
    std::lock_guard<std::mutex> guard(metricsMutex_);
    const int64_t wallMs = start == 0 ? 0 : (end == 0 ? UnixMs() : end) - start;
    const double seconds = (std::max)(0.001,
        static_cast<double>((std::max)(int64_t{ 0 }, wallMs - static_cast<int64_t>(pausedMs))) / 1000.0);
    for (const auto& item : metrics_)
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

// 关键实现：在指标锁内返回错误样本副本，调用方不持有内部引用。
std::vector<MTLOAD_ERROR_SAMPLE> LoadEngine::GetErrors() const
{
    std::lock_guard<std::mutex> guard(metricsMutex_);
    return errors_;
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
    root["TotalSamples"] = std::to_string(state.totalSamples);
    root["PlannedRequests"] = std::to_string(state.plannedRequests);
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
    Json::StyledWriter writer;
    return writer.write(root);
}

// 关键实现：在状态锁内复制最近错误，返回后不依赖内部字符串生命周期。
std::string LoadEngine::GetLastError() const
{
    std::lock_guard<std::mutex> guard(stateMutex_);
    return lastError_;
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
                GetMetrics(), GetErrors());
        if ((formats & MTLOAD_REPORT_XLSX) != 0)
            WriteXlsxReport(root / "MtLoadResult.xlsx", [&]() { MTLOAD_RUN_STATE value{}; GetState(value); return value; }(),
                GetMetrics(), GetErrors());
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
        const auto errors = GetErrors();
        if (format == MTLOAD_REPORT_XLSX) WriteXlsxReport(path, state, metrics, errors);
        else WriteXmlReport(path, state, metrics, errors);
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
        std::lock_guard<std::mutex> guard(stateMutex_);
        if (!hasPlan_ || totalSamples_.load() == 0 || state_ == MTLOAD_STATE_STARTING ||
            state_ == MTLOAD_STATE_RUNNING || state_ == MTLOAD_STATE_PAUSING ||
            state_ == MTLOAD_STATE_PAUSED || state_ == MTLOAD_STATE_STOPPING)
            return MTLOAD_E_INVALID_STATE;
        planPath = std::filesystem::u8path(plan_.sourcePath);
        configuredDirectory = plan_.reportDirectory;
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
