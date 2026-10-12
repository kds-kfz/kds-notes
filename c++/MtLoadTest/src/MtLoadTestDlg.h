#pragma once

// 主对话框仅负责选择计划、发出命令和展示不可变快照。
// 网络、JSON、调度、百分位和报告逻辑不得放入本类。

#include "engine/LoadEngineClient.h"
#include "AppLog.h"

#include <memory>

class CLogDlg;

class CMtLoadTestDlg final : public CDialogEx
{
public:
    // 构造压力测试窗口及空适配器，不加载 DLL。
    CMtLoadTestDlg();
    // 关闭日志子窗口并确保压测适配器完成资源释放。
    ~CMtLoadTestDlg() override;

protected:
    // 初始化控件、日志、DLL 和快照定时器；仅在 UI 线程执行。
    BOOL OnInitDialog() override;
    // 将资源控件绑定到成员包装对象。
    void DoDataExchange(CDataExchange* exchange) override;
    // 复用关闭流程，避免 Esc 绕过引擎停止与日志窗口销毁。
    void OnCancel() override;
    // 处理标题栏关闭并执行完整停机流程。
    afx_msg void OnClose();
    // 选择 XML 计划文件，只更新路径，不隐式启动压测。
    afx_msg void OnBrowsePlan();
    // 重新加载并校验 XML，刷新环境、场景和用户列表。
    afx_msg void OnValidatePlan();
    // 将当前环境下拉项提交给引擎并刷新状态。
    afx_msg void OnEnvironmentChanged();
    // 将当前场景下拉项提交给引擎并刷新计划请求数。
    afx_msg void OnScenarioChanged();
    // 将当前账号选择提交给引擎；密码始终留在 DLL 内部。
    afx_msg void OnUserChanged();
    // 根据当前状态执行开始或停止，停止后的再次开始创建全新运行。
    afx_msg void OnStartStopRun();
    // 根据当前状态执行暂停或继续，不重置场景进度。
    afx_msg void OnPauseResumeRun();
    // 在非运行状态清空当前结果和界面表格。
    afx_msg void OnClearResults();
    // 选择 XML/XLSX 格式并调用配置化导出接口。
    afx_msg void OnExportResults();
    // 创建或激活唯一非模态日志窗口。
    afx_msg void OnShowLog();
    // 定时拉取不可变状态、指标和错误样本。
    afx_msg void OnTimer(UINT_PTR eventId);
    // 窗口缩放时按客户区重排指标列宽。
    afx_msg void OnSize(UINT type, int cx, int cy);
    DECLARE_MESSAGE_MAP()

private:
    static constexpr UINT_PTR kSnapshotTimer = 1; // 运行快照刷新定时器 ID。
    // 返回 EXE 所在目录，用于定位 DLL、配置和日志目录。
    static std::wstring ExecutableDirectory();
    // 删除或替换诊断中的敏感片段后再写日志。
    static std::wstring SanitizeDiagnostic(const std::wstring& value);
    // 将 MTLOAD_STATE 转换为界面状态文本。
    static CString StateText(int state);
    // 将 ABI 固定 UTF-8 文本转换为 CString。
    static CString Utf8Text(const char* value);
    // 拉取一轮快照并增量更新状态、指标、错误和按钮。
    void RefreshSnapshot();
    // 只追加本轮尚未写入应用日志的错误样本。
    void LogEngineErrors();
    // 本轮第一次进入终态时记录一次聚合摘要。
    void LogTerminalSummary(const MTLOAD_RUN_STATE& state,
        const std::vector<MTLOAD_METRIC_ROW>& metrics);
    // 从引擎重新填充环境下拉并保持 selected 标记。
    void RefreshEnvironmentChoices();
    // 从引擎重新填充场景下拉和请求总数摘要。
    void RefreshScenarioChoices();
    // 从引擎重新填充不含密码的用户摘要。
    void RefreshUserChoices();
    // 依据列表客户区宽度分配全部指标列并避免空白尾列。
    void ResizeMetricColumns();
    // 根据状态机统一设置开始、暂停、检测、导出等控件可用性。
    void UpdateControls(int state);
    // 更新底部状态文字，不写入日志。
    void SetStatus(const CString& text);

    CEdit planPathEdit_;                         // XML 计划路径编辑框。
    CComboBox environmentCombo_;                // 环境只读选择框。
    CComboBox scenarioCombo_;                   // 场景只读选择框。
    CComboBox userCombo_;                       // 用户摘要只读选择框。
    CStatic runStateText_;                      // 顶部运行状态和总进度文本。
    CStatic statusText_;                        // 底部操作结果提示。
    CListCtrl metricList_;                      // JMeter 风格聚合指标列表。
    CLoadEngineClient engine_;                  // DLL 的唯一界面适配器和句柄所有者。
    CAppLog appLog_;                            // 应用级线程安全日志存储。
    HICON mainIcon_ = nullptr;                  // 从资源借用的窗口图标句柄。
    std::unique_ptr<CLogDlg> logDialog_;        // 唯一非模态日志窗口所有权。
    std::vector<MTLOAD_ENVIRONMENT> environments_; // 当前环境摘要快照。
    std::vector<MTLOAD_SCENARIO> scenarios_;    // 当前场景摘要快照。
    std::vector<MTLOAD_USER> users_;            // 当前用户摘要快照，不含密码。
    bool engineLoaded_ = false;                 // DLL 与引擎句柄是否可用。
    bool planValidated_ = false;                // 当前路径计划是否已通过校验。
    bool hasExportableResults_ = false;         // 是否至少存在一个已完成样本。
    uint64_t observedRunId_ = 0;                // 界面最后观察到的运行编号。
    uint64_t summarizedRunId_ = 0;              // 已记录终态摘要的运行编号。
    size_t observedErrorCount_ = 0;             // 已写入日志的错误样本数量。
    int lastLoggedState_ = -1;                  // 最近一次写日志的状态，避免定时器重复记录。
    int currentState_ = MTLOAD_STATE_CREATED;   // 当前界面使用的引擎状态快照。
    bool closing_ = false;                      // 防止关闭流程重入。
};

