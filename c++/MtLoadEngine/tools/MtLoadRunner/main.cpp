// 无界面 Runner 仅调用公共 C ABI，不依赖 MtLoadEngine 内部源码。

#include <MtLoadEngine.h>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace
{
// 使用 ABI 两次调用约定读取最近错误，避免假设固定缓冲区长度。
std::string LastError(MTLOAD_HANDLE handle)
{
    uint32_t bytes = 0;
    MtLoad_GetLastError(handle, nullptr, &bytes);
    std::vector<char> value(bytes == 0 ? 1 : bytes);
    if (MtLoad_GetLastError(handle, value.data(), &bytes) != MTLOAD_OK) return {};
    return value.data();
}
}

// 关键实现：按创建、加载、校验、启动、等待、导出、销毁的顺序演示完整 C ABI 生命周期。
int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: MtLoadRunner <absolute-plan.xml> [report-directory]\n";
        return 2;
    }
    MTLOAD_ENGINE_OPTIONS options{};
    options.structSize = sizeof(options);
    if (MtLoad_InitOptions(&options) != MTLOAD_OK)
    {
        std::cerr << "MtLoad_InitOptions failed\n";
        return 3;
    }
    MTLOAD_HANDLE handle = nullptr;
    int result = MtLoad_Create(&options, &handle);
    if (result != MTLOAD_OK)
    {
        std::cerr << "MtLoad_Create failed: " << result << '\n';
        return 4;
    }
    auto destroy = [&]() { if (handle != nullptr) { MtLoad_Destroy(handle); handle = nullptr; } };
    result = MtLoad_LoadPlan(handle, argv[1]);
    if (result == MTLOAD_OK) result = MtLoad_ValidatePlan(handle);
    if (result == MTLOAD_OK) result = MtLoad_Start(handle, "runner");
    if (result != MTLOAD_OK)
    {
        std::cerr << "Start failed: " << result << ", " << LastError(handle) << '\n';
        destroy();
        return 5;
    }

    while (MtLoad_Wait(handle, 1000) == MTLOAD_E_TIMEOUT)
    {
        MTLOAD_RUN_STATE state{};
        state.structSize = sizeof(state);
        state.abiVersion = MTLOAD_ABI_VERSION;
        if (MtLoad_GetRunState(handle, &state) == MTLOAD_OK)
        {
            std::cout << "state=" << state.state << " samples=" << state.totalSamples
                << " activeUsers=" << state.activeUsers << " activeRequests="
                << state.activeRequests << '\n';
        }
    }
    MTLOAD_RUN_STATE terminalState{};
    terminalState.structSize = sizeof(terminalState);
    terminalState.abiVersion = MTLOAD_ABI_VERSION;
    if (MtLoad_GetRunState(handle, &terminalState) != MTLOAD_OK)
    {
        std::cerr << "GetRunState failed after completion\n";
        destroy();
        return 6;
    }
    std::cout << "completed result=" << terminalState.lastResult
        << " requests=" << terminalState.totalSamples
        << " requestThroughput=" << terminalState.totalThroughputPerSec
        << " transactions=" << terminalState.completedIterations
        << " dropped=" << terminalState.droppedIterations << '\n';

    // 事务和阈值使用独立查询接口；Runner 输出与 GUI 日志保持相同口径。
    uint32_t transactionCount = 0;
    if (MtLoad_GetScenarioMetricRows(handle, nullptr, &transactionCount) == MTLOAD_E_BUFFER_TOO_SMALL)
    {
        std::vector<MTLOAD_SCENARIO_METRIC_ROW> transactions(transactionCount);
        if (MtLoad_GetScenarioMetricRows(handle, transactions.data(), &transactionCount) == MTLOAD_OK)
        {
            for (uint32_t index = 0; index < transactionCount; ++index)
            {
                const auto& row = transactions[index];
                std::cout << "transaction scenario=" << row.scenarioId
                    << " completed=" << row.completed << " succeeded=" << row.succeeded
                    << " failed=" << row.failed << " dropped=" << row.dropped
                    << " p95Ms=" << row.p95Ms << " throughput=" << row.throughputPerSec << '\n';
            }
        }
    }
    uint32_t thresholdCount = 0;
    if (MtLoad_GetThresholdResults(handle, nullptr, &thresholdCount) == MTLOAD_E_BUFFER_TOO_SMALL)
    {
        std::vector<MTLOAD_THRESHOLD_RESULT> thresholds(thresholdCount);
        if (MtLoad_GetThresholdResults(handle, thresholds.data(), &thresholdCount) == MTLOAD_OK)
        {
            for (uint32_t index = 0; index < thresholdCount; ++index)
            {
                const auto& row = thresholds[index];
                std::cout << "threshold id=" << row.id << " actual=" << row.actual
                    << " expected=" << row.expected << " passed=" << row.passed << '\n';
            }
        }
    }
    // 运行器用于无界面诊断，终态后输出有限错误样本，避免仅凭失败计数猜测传输原因。
    uint32_t errorCount = 0;
    if (MtLoad_GetErrorSamples(handle, nullptr, &errorCount) == MTLOAD_E_BUFFER_TOO_SMALL &&
        errorCount > 0)
    {
        std::vector<MTLOAD_ERROR_SAMPLE> errors(errorCount);
        if (MtLoad_GetErrorSamples(handle, errors.data(), &errorCount) == MTLOAD_OK)
        {
            for (uint32_t index = 0; index < errorCount; ++index)
            {
                const auto& error = errors[index];
                std::cerr << "request failed: result=" << error.result <<
                    " http=" << error.httpStatus << " elapsedMs=" << error.elapsedMs <<
                    " category=" << error.category << " summary=" << error.summary << '\n';
            }
        }
    }
    const std::string reportDirectory = argc >= 3 ? argv[2] :
        (std::filesystem::current_path() / "reports").u8string();
    const int exportResult = MtLoad_ExportReport(handle, reportDirectory.c_str(),
        MTLOAD_REPORT_JSON | MTLOAD_REPORT_CSV);
    if (exportResult != MTLOAD_OK)
        std::cerr << "Export failed: " << exportResult << '\n';
    destroy();
    if (exportResult != MTLOAD_OK) return 7;
    if (terminalState.lastResult == MTLOAD_E_THRESHOLD_FAILED) return 8;
    return terminalState.lastResult == MTLOAD_OK ? 0 : 9;
}
