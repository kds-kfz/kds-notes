#include "PlanParser.h"

// 依赖 TinyXML 解析结构化 XML；CSV 与内联 Row 均只在 LoadPlan 阶段转成不可变数据集。

#include <tinyxml.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <limits>
#include <set>
#include <sstream>

#include "../../include/MtLoadTypes.h"

namespace mtload
{
namespace
{
// 读取字符串属性并应用默认值；节点或属性缺失均不返回悬空指针。
std::string Attribute(const TiXmlElement* element, const char* name,
    const std::string& defaultValue = {})
{
    const char* value = element == nullptr ? nullptr : element->Attribute(name);
    return value == nullptr ? defaultValue : value;
}

// 读取非负整数属性；非法或负数使用调用方默认值，范围校验留给 ValidatePlan。
uint32_t UnsignedAttribute(const TiXmlElement* element, const char* name,
    uint32_t defaultValue)
{
    int value = 0;
    if (element != nullptr && element->QueryIntAttribute(name, &value) == TIXML_SUCCESS && value >= 0)
    {
        return static_cast<uint32_t>(value);
    }
    return defaultValue;
}

// 兼容 true/1/yes 的布尔属性写法，其余文本按 false 处理。
bool BoolAttribute(const TiXmlElement* element, const char* name, bool defaultValue)
{
    const std::string value = Attribute(element, name, defaultValue ? "true" : "false");
    return value == "true" || value == "1" || value == "yes";
}

// 以二进制方式读取 UTF-8 文件，失败返回空字符串并由上层补充路径诊断。
std::string ReadUtf8File(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
    {
        return {};
    }
    return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

// 按 RFC 4180 的双引号和双引号转义规则拆分单行 CSV。
std::vector<std::string> ParseCsvLine(const std::string& line)
{
    std::vector<std::string> fields;
    std::string current;
    bool quoted = false;
    for (size_t index = 0; index < line.size(); ++index)
    {
        const char ch = line[index];
        if (ch == '"')
        {
            if (quoted && index + 1 < line.size() && line[index + 1] == '"')
            {
                current.push_back('"');
                ++index;
            }
            else
            {
                quoted = !quoted;
            }
        }
        else if (ch == ',' && !quoted)
        {
            fields.push_back(current);
            current.clear();
        }
        else
        {
            current.push_back(ch);
        }
    }
    fields.push_back(current);
    return fields;
}

// 把 CSV 表头和行转换为统一 DataSet.rows，并在读取阶段拒绝列数漂移。
int LoadCsv(DataSet& dataSet, const std::filesystem::path& base,
    std::string& outError)
{
    const std::filesystem::path path = std::filesystem::weakly_canonical(base / std::filesystem::u8path(dataSet.path));
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
    {
        outError = "无法读取 CSV 数据集: " + path.u8string();
        return MTLOAD_E_CONFIG_IO;
    }

    std::string line;
    if (!std::getline(stream, line))
    {
        outError = "CSV 数据集没有表头: " + path.u8string();
        return MTLOAD_E_CONFIG_SYNTAX;
    }
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line.size() >= 3 && static_cast<unsigned char>(line[0]) == 0xEF &&
        static_cast<unsigned char>(line[1]) == 0xBB && static_cast<unsigned char>(line[2]) == 0xBF)
    {
        line.erase(0, 3);
    }

    const std::vector<std::string> headers = ParseCsvLine(line);
    std::set<std::string> unique;
    for (const auto& header : headers)
    {
        if (header.empty() || !unique.insert(header).second)
        {
            outError = "CSV 表头为空或重复: " + dataSet.id;
            return MTLOAD_E_CONFIG_SEMANTIC;
        }
    }

    while (std::getline(stream, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        const auto values = ParseCsvLine(line);
        if (values.size() != headers.size())
        {
            outError = "CSV 列数与表头不一致: " + dataSet.id;
            return MTLOAD_E_CONFIG_SYNTAX;
        }
        std::map<std::string, std::string> row;
        for (size_t i = 0; i < headers.size(); ++i) row.emplace(headers[i], values[i]);
        dataSet.rows.push_back(std::move(row));
    }
    return MTLOAD_OK;
}

// 把 Inline/Row 的属性和 Value 子节点转换为统一数据行，拒绝空行和重复字段。
int LoadInline(const TiXmlElement* element, DataSet& dataSet, std::string& outError)
{
    for (const TiXmlElement* rowElement = element->FirstChildElement("Row"); rowElement != nullptr;
        rowElement = rowElement->NextSiblingElement("Row"))
    {
        std::map<std::string, std::string> row;
        for (const TiXmlAttribute* attribute = rowElement->FirstAttribute(); attribute != nullptr;
            attribute = attribute->Next())
        {
            if (std::string(attribute->Name()).empty() ||
                !row.emplace(attribute->Name(), attribute->Value()).second)
            {
                outError = "Inline Row 字段名为空或重复: " + dataSet.id;
                return MTLOAD_E_CONFIG_SEMANTIC;
            }
        }
        for (const TiXmlElement* value = rowElement->FirstChildElement("Value"); value != nullptr;
            value = value->NextSiblingElement("Value"))
        {
            const std::string name = Attribute(value, "name");
            const std::string fieldValue = Attribute(value, "value",
                value->GetText() == nullptr ? "" : value->GetText());
            if (name.empty() || !row.emplace(name, fieldValue).second)
            {
                outError = "Inline Row 字段名为空或重复: " + dataSet.id;
                return MTLOAD_E_CONFIG_SEMANTIC;
            }
        }
        if (row.empty())
        {
            outError = "Inline Row 至少需要一个字段: " + dataSet.id;
            return MTLOAD_E_CONFIG_SEMANTIC;
        }
        dataSet.rows.push_back(std::move(row));
    }
    if (dataSet.rows.empty())
    {
        outError = "Inline 数据集至少需要一个 Row: " + dataSet.id;
        return MTLOAD_E_CONFIG_SEMANTIC;
    }
    return MTLOAD_OK;
}

// 按 XML 顺序复制 Header 节点；语义校验在完整计划解析后统一执行。
void ParseHeaders(const TiXmlElement* parent, std::vector<Header>& out)
{
    if (parent == nullptr) return;
    for (const TiXmlElement* header = parent->FirstChildElement("Header"); header != nullptr;
        header = header->NextSiblingElement("Header"))
    {
        out.push_back({ Attribute(header, "name"), Attribute(header, "value") });
    }
}

// 将 Request 子树映射为请求、断言和提取规则，不在此阶段判断跨节点引用。
void ParseRequest(const TiXmlElement* element, Request& request)
{
    request.id = Attribute(element, "id");
    request.label = Attribute(element, "label", request.id);
    request.targetId = Attribute(element, "target");
    request.method = Attribute(element, "method", "GET");
    request.path = Attribute(element, "path", "/");
    request.codec = Attribute(element, "codec", "json");
    request.copies = UnsignedAttribute(element, "copies", 1);
    if (const TiXmlElement* body = element->FirstChildElement("Body"))
    {
        if (body->GetText() != nullptr) request.body = body->GetText();
    }
    ParseHeaders(element->FirstChildElement("Headers"), request.headers);

    if (const TiXmlElement* assertions = element->FirstChildElement("Assertions"))
    {
        for (const TiXmlElement* item = assertions->FirstChildElement(); item != nullptr;
            item = item->NextSiblingElement())
        {
            Assertion assertion;
            if (std::string(item->Value()) == "HttpStatus")
            {
                assertion.kind = Assertion::Kind::HttpStatus;
                assertion.minStatus = static_cast<int>(UnsignedAttribute(item, "min", 200));
                assertion.maxStatus = static_cast<int>(UnsignedAttribute(item, "max", 299));
            }
            else if (std::string(item->Value()) == "Json")
            {
                assertion.kind = Assertion::Kind::Json;
                assertion.path = Attribute(item, "path");
                assertion.op = Attribute(item, "operator", "eq");
                assertion.value = Attribute(item, "value");
            }
            else continue;
            request.assertions.push_back(std::move(assertion));
        }
    }
    if (const TiXmlElement* extractors = element->FirstChildElement("Extractors"))
    {
        for (const TiXmlElement* item = extractors->FirstChildElement("Json"); item != nullptr;
            item = item->NextSiblingElement("Json"))
        {
            request.extractors.push_back({ Attribute(item, "path"), Attribute(item, "variable"),
                Attribute(item, "scope", "session"), BoolAttribute(item, "required", true) });
        }
    }
}
}

// 关键实现：先在局部 Plan 中完成整份 XML/CSV 转换，全部成功后才覆盖 outPlan。
int ParsePlan(const std::string& pathUtf8, Plan& outPlan, std::string& outError)
{
    try
    {
        // 阶段一：读取并验证 XML 文档、根节点和版本，语法错误保留行号。
        const std::filesystem::path path = std::filesystem::absolute(std::filesystem::u8path(pathUtf8));
        const std::string text = ReadUtf8File(path);
        if (text.empty())
        {
            outError = "无法读取计划文件或文件为空: " + pathUtf8;
            return MTLOAD_E_CONFIG_IO;
        }
        TiXmlDocument document;
        document.Parse(text.c_str(), nullptr, TIXML_ENCODING_UTF8);
        if (document.Error())
        {
            outError = "XML 语法错误，第 " + std::to_string(document.ErrorRow()) + " 行: " + document.ErrorDesc();
            return MTLOAD_E_CONFIG_SYNTAX;
        }
        const TiXmlElement* root = document.RootElement();
        if (root == nullptr || std::string(root->Value()) != "MtLoadPlan")
        {
            outError = "根节点必须是 MtLoadPlan";
            return MTLOAD_E_CONFIG_SYNTAX;
        }
        if (Attribute(root, "version") != "1")
        {
            outError = "仅支持 MtLoadPlan version=1";
            return MTLOAD_E_UNSUPPORTED;
        }

        // 阶段二：依次解析环境、用户、目标和全局头部，重复 ID 就地拒绝。
        Plan plan;
        plan.sourcePath = path.u8string();
        plan.name = Attribute(root, "name", path.stem().u8string());
        if (const TiXmlElement* reports = root->FirstChildElement("Reports"))
            plan.reportDirectory = Attribute(reports, "directory");
        if (const TiXmlElement* environments = root->FirstChildElement("Environments"))
        {
            plan.defaultEnvironmentId = Attribute(environments, "default");
            for (const TiXmlElement* item = environments->FirstChildElement("Environment");
                item != nullptr; item = item->NextSiblingElement("Environment"))
            {
                Environment environment;
                environment.id = Attribute(item, "id");
                environment.name = Attribute(item, "name", environment.id);
                environment.scheme = Attribute(item, "scheme", "http");
                environment.host = Attribute(item, "host");
                environment.port = static_cast<uint16_t>(UnsignedAttribute(item, "port", 0));
                environment.basePath = Attribute(item, "basePath");
                environment.tlsCaFile = Attribute(item, "tlsCaFile");
                environment.tlsVerify = BoolAttribute(item, "tlsVerify", true);
                if (!plan.environments.emplace(environment.id, std::move(environment)).second)
                {
                    outError = "Environment id 重复: " + Attribute(item, "id");
                    return MTLOAD_E_CONFIG_SEMANTIC;
                }
            }
            if (plan.defaultEnvironmentId.empty() && !plan.environments.empty())
                plan.defaultEnvironmentId = plan.environments.begin()->first;
        }
        if (const TiXmlElement* users = root->FirstChildElement("Users"))
        {
            for (const TiXmlElement* item = users->FirstChildElement("User"); item != nullptr;
                item = item->NextSiblingElement("User"))
            {
                UserProfile user;
                user.id = Attribute(item, "id");
                user.name = Attribute(item, "name", user.id);
                if (user.name.empty()) user.name = user.id;
                user.variables.emplace("UserId", user.id);
                user.variables.emplace("Version", Attribute(item, "version"));
                user.variables.emplace("No", Attribute(item, "no"));
                user.variables.emplace("Login", Attribute(item, "login"));
                user.variables.emplace("Password", Attribute(item, "password"));
                for (const TiXmlElement* parameter = item->FirstChildElement("Param");
                    parameter != nullptr; parameter = parameter->NextSiblingElement("Param"))
                {
                    const std::string name = Attribute(parameter, "name");
                    const std::string value = Attribute(parameter, "value",
                        parameter->GetText() == nullptr ? "" : parameter->GetText());
                    if (name.empty() || !user.variables.emplace(name, value).second)
                    {
                        outError = "User Param 名称为空或与标准字段重复: " + user.id;
                        return MTLOAD_E_CONFIG_SEMANTIC;
                    }
                }
                if (!plan.users.emplace(user.id, std::move(user)).second)
                {
                    outError = "User id 重复: " + Attribute(item, "id");
                    return MTLOAD_E_CONFIG_SEMANTIC;
                }
            }
        }
        if (const TiXmlElement* targets = root->FirstChildElement("Targets"))
        {
            for (const TiXmlElement* item = targets->FirstChildElement("Target"); item != nullptr;
                item = item->NextSiblingElement("Target"))
            {
                Target target;
                target.id = Attribute(item, "id");
                target.transport = Attribute(item, "transport", "http");
                target.host = Attribute(item, "host");
                target.port = static_cast<uint16_t>(UnsignedAttribute(item, "port", 0));
                target.basePath = Attribute(item, "basePath");
                target.tlsCaFile = Attribute(item, "tlsCaFile");
                target.tlsVerify = BoolAttribute(item, "tlsVerify", true);
                target.connectTimeoutMs = UnsignedAttribute(item, "connectTimeoutMs", 3000);
                target.requestTimeoutMs = UnsignedAttribute(item, "requestTimeoutMs", 5000);
                target.maxConnections = UnsignedAttribute(item, "maxConnections", 100);
                if (!plan.targets.emplace(target.id, std::move(target)).second)
                {
                    outError = "Target id 重复: " + Attribute(item, "id");
                    return MTLOAD_E_CONFIG_SEMANTIC;
                }
            }
        }
        ParseHeaders(root->FirstChildElement("Headers"), plan.headers);

        // 阶段三：把 CSV 和 Inline 数据统一物化，运行线程不再访问外部数据文件。
        const std::filesystem::path base = path.parent_path();
        if (const TiXmlElement* sets = root->FirstChildElement("DataSets"))
        {
            for (const TiXmlElement* item = sets->FirstChildElement("Csv"); item != nullptr;
                item = item->NextSiblingElement("Csv"))
            {
                DataSet dataSet;
                dataSet.id = Attribute(item, "id");
                dataSet.path = Attribute(item, "path");
                dataSet.rowMode = Attribute(item, "rowMode", "circular");
                const int result = LoadCsv(dataSet, base, outError);
                if (result != MTLOAD_OK) return result;
                if (!plan.dataSets.emplace(dataSet.id, std::move(dataSet)).second)
                {
                    outError = "DataSet id 重复: " + Attribute(item, "id");
                    return MTLOAD_E_CONFIG_SEMANTIC;
                }
            }
            for (const TiXmlElement* item = sets->FirstChildElement("Inline"); item != nullptr;
                item = item->NextSiblingElement("Inline"))
            {
                DataSet dataSet;
                dataSet.id = Attribute(item, "id");
                dataSet.rowMode = Attribute(item, "rowMode", "circular");
                if (dataSet.id.empty())
                {
                    outError = "Inline DataSet id 不能为空";
                    return MTLOAD_E_CONFIG_SEMANTIC;
                }
                const int result = LoadInline(item, dataSet, outError);
                if (result != MTLOAD_OK) return result;
                if (!plan.dataSets.emplace(dataSet.id, std::move(dataSet)).second)
                {
                    outError = "DataSet id 重复: " + Attribute(item, "id");
                    return MTLOAD_E_CONFIG_SEMANTIC;
                }
            }
        }

        // 阶段四：保留场景、步骤和请求的 XML 顺序，供运行时严格按顺序调度。
        if (const TiXmlElement* scenarios = root->FirstChildElement("Scenarios"))
        {
            for (const TiXmlElement* scenarioNode = scenarios->FirstChildElement("Scenario"); scenarioNode != nullptr;
                scenarioNode = scenarioNode->NextSiblingElement("Scenario"))
            {
                Scenario scenario;
                scenario.id = Attribute(scenarioNode, "id");
                scenario.name = Attribute(scenarioNode, "name", scenario.id);
                scenario.dataSetId = Attribute(scenarioNode, "dataSet");
                if (const TiXmlElement* userRefs = scenarioNode->FirstChildElement("UserRefs"))
                {
                    for (const TiXmlElement* userRef = userRefs->FirstChildElement("UserRef");
                        userRef != nullptr; userRef = userRef->NextSiblingElement("UserRef"))
                        scenario.userIds.push_back(Attribute(userRef, "id"));
                }
                if (const TiXmlElement* load = scenarioNode->FirstChildElement("Load"))
                {
                    scenario.load.mode = Attribute(load, "mode", "closed");
                    scenario.load.threads = UnsignedAttribute(load, "threads",
                        UnsignedAttribute(load, "users", 1));
                    scenario.load.rampUpMs = UnsignedAttribute(load, "rampUpMs", 0);
                    scenario.load.durationMs = UnsignedAttribute(load, "durationMs", 0);
                    scenario.load.iterations = UnsignedAttribute(load, "iterations", scenario.load.durationMs == 0 ? 1 : 0);
                }
                for (const TiXmlElement* stepNode = scenarioNode->FirstChildElement("Step"); stepNode != nullptr;
                    stepNode = stepNode->NextSiblingElement("Step"))
                {
                    Step step;
                    step.id = Attribute(stepNode, "id");
                    step.name = Attribute(stepNode, "name", step.id);
                    step.onError = Attribute(stepNode, "onError", "stop_iteration");
                    step.alwaysRun = BoolAttribute(stepNode, "alwaysRun", false);
                    for (const TiXmlElement* requestNode = stepNode->FirstChildElement("Request"); requestNode != nullptr;
                        requestNode = requestNode->NextSiblingElement("Request"))
                    {
                        Request request;
                        ParseRequest(requestNode, request);
                        step.requests.push_back(std::move(request));
                    }
                    scenario.steps.push_back(std::move(step));
                }
                plan.scenarios.push_back(std::move(scenario));
            }
        }
        // 只有完整解析成功才提交局部模型，失败不会留下半份计划。
        outPlan = std::move(plan);
        return MTLOAD_OK;
    }
    catch (const std::exception& exception)
    {
        outError = std::string("解析计划失败: ") + exception.what();
        return MTLOAD_E_CONFIG_IO;
    }
}

// 关键实现：在启动前一次性完成范围、引用、并发写冲突和总请求数溢出检查。
int ValidatePlan(const Plan& plan, std::string& outError)
{
    // 先验证计划级必需集合和环境默认引用。
    if (plan.targets.empty() || plan.scenarios.empty())
    {
        outError = "计划至少需要一个 Target 和一个 Scenario";
        return MTLOAD_E_CONFIG_SEMANTIC;
    }
    if (!plan.environments.empty())
    {
        if (plan.environments.find(plan.defaultEnvironmentId) == plan.environments.end())
        {
            outError = "Environments default 引用了不存在的环境: " + plan.defaultEnvironmentId;
            return MTLOAD_E_CONFIG_SEMANTIC;
        }
        for (const auto& pair : plan.environments)
        {
            const Environment& environment = pair.second;
            if (environment.id.empty() || environment.name.empty() ||
                (environment.scheme != "http" && environment.scheme != "https") ||
                environment.host.empty() ||
                (!environment.basePath.empty() && environment.basePath[0] != '/'))
            {
                outError = "Environment 的 id、name、scheme、host 或 basePath 无效: " + environment.id;
                return MTLOAD_E_CONFIG_SEMANTIC;
            }
        }
    }
    // 用户只校验结构引用；具体接口字段是否必填由 XML 模板和服务端决定。
    for (const auto& pair : plan.users)
    {
        const UserProfile& user = pair.second;
        // 压测引擎只校验用户配置的引用结构，不判断业务接口字段是否必填或取值是否合法。
        // 标准属性缺省时仍会生成同名空变量，由请求模板决定是否发送以及如何加 JSON 引号。
        if (user.id.empty())
        {
            outError = "User id 不能为空";
            return MTLOAD_E_CONFIG_SEMANTIC;
        }
    }
    // 目标校验同时执行当前版本协议能力门禁。
    for (const auto& pair : plan.targets)
    {
        const Target& target = pair.second;
        if (target.id.empty() || (plan.environments.empty() && target.host.empty()))
        {
            outError = "Target 的 id 和 host 必须有效";
            return MTLOAD_E_CONFIG_SEMANTIC;
        }
        if (target.transport != "http" && target.transport != "https")
        {
            outError = "当前仅实现 transport=http/https，当前值: " + target.transport;
            return MTLOAD_E_UNSUPPORTED;
        }
    }
    // 场景级校验沿执行层级向下，确保每个 ID 唯一且引用闭合。
    std::set<std::string> scenarioIds;
    uint64_t plannedTotal = 0;
    for (const Scenario& scenario : plan.scenarios)
    {
        if (scenario.id.empty() || !scenarioIds.insert(scenario.id).second || scenario.steps.empty())
        {
            outError = "Scenario id 为空、重复或没有 Step: " + scenario.id;
            return MTLOAD_E_CONFIG_SEMANTIC;
        }
        if (scenario.load.mode != "closed")
        {
            outError = "当前里程碑仅实现 closed 负载模型";
            return MTLOAD_E_UNSUPPORTED;
        }
        if (scenario.load.threads == 0 || scenario.load.threads > 10000 ||
            (scenario.load.durationMs == 0 && scenario.load.iterations == 0))
        {
            outError = "Load threads 或结束条件无效: " + scenario.id;
            return MTLOAD_E_CONFIG_SEMANTIC;
        }
        if (!scenario.userIds.empty() && !scenario.dataSetId.empty())
        {
            outError = "Scenario 不能同时配置 UserRefs 和 DataSet: " + scenario.id;
            return MTLOAD_E_CONFIG_SEMANTIC;
        }
        std::set<std::string> userRefs;
        for (const std::string& userId : scenario.userIds)
        {
            if (userId.empty() || !userRefs.insert(userId).second ||
                plan.users.find(userId) == plan.users.end())
            {
                outError = "Scenario UserRef 为空、重复或不存在: " + userId;
                return MTLOAD_E_CONFIG_SEMANTIC;
            }
        }
        if (!scenario.dataSetId.empty() && plan.dataSets.find(scenario.dataSetId) == plan.dataSets.end())
        {
            outError = "Scenario 引用了不存在的 DataSet: " + scenario.dataSetId;
            return MTLOAD_E_CONFIG_SEMANTIC;
        }
        if (!scenario.dataSetId.empty() && plan.dataSets.at(scenario.dataSetId).rowMode != "circular")
        {
            outError = "当前里程碑仅实现 DataSet rowMode=circular";
            return MTLOAD_E_UNSUPPORTED;
        }
        std::set<std::string> stepIds;
        uint64_t requestsPerIteration = 0;
        for (const Step& step : scenario.steps)
        {
            if (step.id.empty() || !stepIds.insert(step.id).second || step.requests.empty())
            {
                outError = "Step id 为空、重复或没有 Request: " + step.id;
                return MTLOAD_E_CONFIG_SEMANTIC;
            }
            if (step.onError != "stop_iteration" && step.onError != "continue")
            {
                outError = "当前里程碑仅实现 onError=stop_iteration/continue: " + step.id;
                return MTLOAD_E_UNSUPPORTED;
            }
            std::set<std::string> sessionWrites;
            std::set<std::string> requestIds;
            for (const Request& request : step.requests)
            {
                if (request.id.empty() || !requestIds.insert(request.id).second ||
                    request.path.empty() || request.path[0] != '/' ||
                    plan.targets.find(request.targetId) == plan.targets.end() ||
                    request.copies == 0 || request.copies > 10000)
                {
                    outError = "Request 的 id、path、target 或 copies 无效: " + request.id;
                    return MTLOAD_E_CONFIG_SEMANTIC;
                }
                if (request.codec != "json")
                {
                    outError = "首版仅实现 codec=json，当前值: " + request.codec;
                    return MTLOAD_E_UNSUPPORTED;
                }
                if (requestsPerIteration > (std::numeric_limits<uint64_t>::max)() - request.copies)
                {
                    outError = "Scenario 单轮请求数溢出: " + scenario.id;
                    return MTLOAD_E_CONFIG_SEMANTIC;
                }
                requestsPerIteration += request.copies;
                for (const Extractor& extractor : request.extractors)
                {
                    if (extractor.variable.empty())
                    {
                        outError = "Extractor variable 不能为空: " + request.id;
                        return MTLOAD_E_CONFIG_SEMANTIC;
                    }
                    if (extractor.scope == "session" && !sessionWrites.insert(extractor.variable).second)
                    {
                        outError = "同一步并行请求重复写 session 变量: " + extractor.variable;
                        return MTLOAD_E_CONFIG_SEMANTIC;
                    }
                }
            }
        }
        // 有限循环模式预计算总请求数，并在每次乘加前检查 uint64_t 溢出。
        if (scenario.load.durationMs == 0)
        {
            const uint64_t usersAndIterations = static_cast<uint64_t>(scenario.load.threads) *
                scenario.load.iterations;
            if (requestsPerIteration != 0 && usersAndIterations >
                (std::numeric_limits<uint64_t>::max)() / requestsPerIteration)
            {
                outError = "Scenario 计划请求数溢出: " + scenario.id;
                return MTLOAD_E_CONFIG_SEMANTIC;
            }
            const uint64_t scenarioTotal = usersAndIterations * requestsPerIteration;
            if (plannedTotal > (std::numeric_limits<uint64_t>::max)() - scenarioTotal)
            {
                outError = "计划请求总数溢出";
                return MTLOAD_E_CONFIG_SEMANTIC;
            }
            plannedTotal += scenarioTotal;
        }
    }
    outError.clear();
    return MTLOAD_OK;
}
}
