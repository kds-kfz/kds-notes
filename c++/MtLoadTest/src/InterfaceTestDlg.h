#pragma once

// 通用接口测试对话框，只负责编排配置选择、编辑状态和异步传输结果展示。
// 配置解析、JSON 构造及网络请求分别委托给 interface_test 下的独立模块。

#include "interface_test/InterfaceTestModel.h"
#include "interface_test/InterfaceTransport.h"
#include "AppLog.h"

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>

class CLogDlg;

// 接口测试窗口只承担交互和展示；配置、JSON 和网络处理分别由 interface_test
// 目录中的模型、编解码和传输对象完成。
class CInterfaceTestDlg final : public CDialogEx
{
public:
    // 构造窗口及 HTTP/WebSocket 传输对象，不在构造阶段访问控件。
    CInterfaceTestDlg();
    // 停止请求、心跳和接收线程后释放传输对象。
    ~CInterfaceTestDlg() override;

protected:
    // 初始化控件、列表和默认配置路径。
    BOOL OnInitDialog() override;
    // 将资源控件绑定到成员包装对象。
    void DoDataExchange(CDataExchange* exchange) override;
    // 在确认未保存内容并停止后台任务后关闭窗口。
    void OnCancel() override;

    // 窗口销毁阶段执行最后一次线程和连接收敛。
    afx_msg void OnDestroy();
    // 窗口缩放时重新计算字段表和 WebSocket 列宽。
    afx_msg void OnSize(UINT type, int width, int height);
    // 浏览选择独立的接口测试 XML 配置。
    afx_msg void OnBrowse();
    // 加载路径框中的配置并刷新全部下拉项。
    afx_msg void OnLoad();
    // 将工作副本写回配置模型并执行原子保存。
    afx_msg void OnSave();
    // 环境变化后过滤服务器及后续接口选择。
    afx_msg void OnEnvironmentChanged();
    // 服务类型变化后过滤服务器和合法报文协议。
    afx_msg void OnTransportChanged();
    // 服务器变化后刷新协议、分类和接口列表。
    afx_msg void OnServerChanged();
    // 报文协议变化后重新筛选可执行用例。
    afx_msg void OnPayloadChanged();
    // 分类变化后刷新接口名称列表。
    afx_msg void OnCategoryChanged();
    // 切换用例前处理未保存编辑，再加载新的工作副本。
    afx_msg void OnCaseChanged();
    // 切换变量组并重置仅驻留内存的会话变量。
    afx_msg void OnVariableSetChanged();
    // 标记文本编辑区已有用户修改。
    afx_msg void OnEditorChanged();
    // 处理字段启用复选框变化并执行必填字段门禁。
    afx_msg void OnFieldChanged(NMHDR* header, LRESULT* result);
    // 把选中字段载入 Pointer、类型和值编辑控件。
    afx_msg void OnFieldSelectionChanged(NMHDR* header, LRESULT* result);
    // 在右侧显示选中 WebSocket 记录的完整 JSON。
    afx_msg void OnWebSelectionChanged(NMHDR* header, LRESULT* result);
    // 校验并应用当前字段编辑控件内容。
    afx_msg void OnApplyField();
    // 新增一个默认可选字段并进入编辑状态。
    afx_msg void OnAddField();
    // 删除当前可选字段；必填字段拒绝删除。
    afx_msg void OnDeleteField();
    // 解析原始 JSON 并同步字段表和根节点类型。
    afx_msg void OnApplyJson();
    // 校验并在后台执行当前 HTTP/HTTPS/JSON 用例。
    afx_msg void OnSend();
    // 取消当前 HTTP 请求，不阻塞 UI 线程。
    afx_msg void OnCancelRequest();
    // 校验配置、展开鉴权信息并建立 WebSocket 连接。
    afx_msg void OnConnectWeb();
    // 主动断开 WebSocket 并等待传输线程退出。
    afx_msg void OnDisconnectWeb();
    // 清空内存中的 WebSocket 消息和详情显示。
    afx_msg void OnClearWeb();
    // 创建或激活与压力测试共用实现的非模态日志窗口。
    afx_msg void OnShowLog();
    // 在 UI 线程接收后台 HTTP 结果所有权并更新响应区。
    afx_msg LRESULT OnHttpCompleted(WPARAM, LPARAM parameter);
    // 在 UI 线程接收一条深拷贝 WebSocket 消息并执行条数上限淘汰。
    afx_msg LRESULT OnWebMessage(WPARAM, LPARAM parameter);
    // 在 UI 线程更新 WebSocket 连接状态和按钮。
    afx_msg LRESULT OnWebState(WPARAM, LPARAM parameter);
    DECLARE_MESSAGE_MAP()

private:
    // 提交当前编辑内容后才能切换用例或发送，保证字段表与原始 JSON 不分叉。
    // 成功时替换 config_ 和工作副本；失败时保留当前界面内容供修正。
    bool LoadConfiguration(const std::wstring& path);
    // 对脏工作副本提示保存、放弃或取消；返回 false 表示终止当前切换。
    bool ConfirmPendingChanges();
    // 将控件值提交到 workingCase_ 并校验 JSON/字段表一致性。
    bool CommitEditors(std::string& error);
    // 使用指定会话变量从当前字段表生成真实和脱敏 JSON。
    bool BuildCurrentBody(interface_test::SessionVariables& session, std::string& body,
        std::string& redactedBody, std::string& error) const;
    // 展开当前请求头，并单独生成用于界面的脱敏文本。
    bool BuildCurrentHeaders(interface_test::SessionVariables& session,
        std::vector<std::pair<std::string, std::string>>& headers,
        std::string& redactedHeaders, std::string& error) const;
    // 写操作发送前展示服务器、URL 和脱敏正文并请求用户确认。
    bool ConfirmWriteRequest(const std::string& url, const std::string& redactedBody) const;
    // 建立字段和 WebSocket 列表列定义。
    void InitializeLists();
    // 按客户区宽度动态铺满字段表各列，优先保证 Pointer、值和说明可读。
    void ResizeFieldColumns();
    // 按客户区宽度动态铺满 WebSocket 各列，窄窗口保留水平滚动。
    void ResizeWebColumns();
    // 填充环境下拉并维护 environmentIds_ 索引映射。
    void PopulateEnvironments();
    // 填充四种服务类型及实现状态。
    void PopulateTransports();
    // 根据环境和传输类型筛选服务器。
    void PopulateServers();
    // 根据能力矩阵过滤报文协议并标记待开发组合。
    void PopulatePayloads();
    // 填充账号及通用变量组。
    void PopulateVariableSets();
    // 汇总当前服务器/协议下的接口分类。
    void PopulateCategories();
    // 根据服务器组、协议和分类填充接口用例。
    void PopulateCases();
    // 从 config_ 复制选中用例形成可编辑工作副本。
    void LoadSelectedCase();
    // 刷新说明、方法、路径、字段、JSON 和请求预览。
    void RefreshCaseDisplay();
    // 从 workingCase_.fields 重建字段列表。
    void RefreshFieldList();
    // 使用脱敏会话副本生成实际请求预览，不消耗正式 sequence。
    void RefreshRequestPreview();
    // 根据实现状态、连接状态、脏标记和后台任务统一更新按钮。
    void RefreshControlState();
    // 从选中变量组重建内存会话并登记敏感变量名。
    void ResetSessionVariables();
    // 更新底部状态文本。
    void SetStatus(const std::wstring& text);
    // 按稳定 ID 在并行索引数组中选择下拉项。
    void SelectComboId(CComboBox& combo, const std::vector<std::string>& ids, const std::string& id);
    // 返回当前选中服务器；指针在配置未重新加载期间有效。
    const interface_test::ServerProfile* SelectedServer() const;
    // 返回当前服务类型；没有有效项时使用安全默认值。
    interface_test::TransportKind SelectedTransport() const;
    // 返回当前报文类型；没有有效项时使用安全默认值。
    interface_test::PayloadKind SelectedPayload() const;
    // 后台线程不得直接访问 MFC 控件，完成结果统一通过窗口消息回到 UI 线程。
    void StartWorker(std::function<void()> work);
    // 取消活动请求、断开 WebSocket，并等待本窗口创建的工作线程全部退出。
    void StopWorkers();

    CComboBox environmentCombo_;       // 环境选择框。
    CComboBox transportCombo_;         // TCP/HTTP/HTTPS/Web 服务类型选择框。
    CComboBox serverCombo_;            // 当前环境和类型下的服务器选择框。
    CComboBox payloadCombo_;           // Binary/JSON/Protobuf 报文类型选择框。
    CComboBox variableSetCombo_;       // 账号及通用变量组选择框。
    CComboBox categoryCombo_;          // 接口分类选择框。
    CComboBox caseCombo_;              // 接口用例选择框。
    CComboBox fieldTypeCombo_;         // 当前字段 JSON 类型编辑框。
    CListCtrl fieldList_;              // 字段启用、Pointer、类型、值和说明列表。
    CListCtrl webList_;                // WebSocket 收发消息列表。
    CAppLog appLog_;                   // 复用的线程安全应用日志存储，不记录敏感报文。
    std::unique_ptr<CLogDlg> logDialog_; // 复用的唯一非模态日志窗口所有权。

    interface_test::InterfaceTestConfig config_; // 当前已加载且可保存的完整配置模型。
    // workingCase_ 是可编辑副本；只有保存成功后才覆盖 config_ 中的正式用例。
    interface_test::InterfaceCase workingCase_; // 当前用例可编辑副本。
    interface_test::SessionVariables session_; // 当前变量组和响应提取的内存会话。
    std::unique_ptr<interface_test::SocketHttpTransport> httpTransport_; // SocketClient HTTP/HTTPS 请求所有者。
    std::unique_ptr<interface_test::SocketWebTransport> webTransport_; // SocketClient WebSocket/WSS 连接所有者。
    std::thread worker_;                  // 单个 HTTP 后台任务线程，由窗口联结。
    std::atomic<bool> workerRunning_{false}; // 防止并发启动两个 HTTP 请求。
    std::atomic<bool> closing_{false};    // 阻止关闭后继续投递窗口消息。

    std::wstring configPath_;              // 当前配置文件绝对路径。
    std::string selectedCaseId_;           // 工作副本来源用例 ID。
    std::vector<std::string> environmentIds_; // 环境下拉索引到稳定 ID 的映射。
    std::vector<interface_test::TransportKind> transportValues_; // 服务类型下拉枚举映射。
    std::vector<std::string> serverIds_;    // 服务器下拉索引到稳定 ID 的映射。
    std::vector<interface_test::PayloadKind> payloadValues_; // 报文下拉枚举映射。
    std::vector<std::string> variableSetIds_; // 变量组下拉索引到稳定 ID 的映射。
    std::vector<std::string> categoryValues_; // 分类下拉索引到 UTF-8 分类名的映射。
    std::vector<std::string> caseIds_;      // 用例下拉索引到稳定 ID 的映射。
    std::vector<interface_test::WebMessage> webMessages_; // 与 Web 列表行同步的消息副本。
    // refreshing_ 屏蔽程序刷新控件产生的通知；dirty_ 只记录用户修改。
    bool refreshing_ = false;       // 程序刷新控件期间屏蔽变更通知。
    bool dirty_ = false;            // 工作副本是否包含尚未保存的修改。
    bool rawJsonDirty_ = false;     // 原始 JSON 是否领先于字段表，提交前需同步。
};
