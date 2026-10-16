#include "PlanParser.h"

// 依赖 TinyXML 解析结构化 XML；CSV 与内联 Row 均只在 LoadPlan 阶段转成不可变数据集。

#include <tinyxml.h>

#include <algorithm>
#include <cmath>
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

// 读取浮点属性并保留负数等已成功解析的原值；具体取值范围由后续语义校验决定，
// 避免解析阶段把错误配置静默替换成可通过校验的默认值。
double DoubleAttribute(const TiXmlElement* element, const char* name, double defaultValue)
{
    double value = 0.0;
    if (element != nullptr && element->QueryDoubleAttribute(name, &value) == TIXML_SUCCESS)
    {
        return value;
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

// 将 Scenario 或 Workload 下的 Load 映射为统一模型。阶段顺序即执行顺序，
// 每段目标速率由上一段目标线性过渡，解析阶段不做上限判断。
void ParseLoad(const TiXmlElement* element, Load& load)
{
    if (element == nullptr) return;
    load.mode = Attribute(element, "mode", "closed");
    load.threads = UnsignedAttribute(element, "threads",
        UnsignedAttribute(element, "users", 1));
    load.rampUpMs = UnsignedAttribute(element, "rampUpMs", 0);
    load.durationMs = UnsignedAttribute(element, "durationMs", 0);
    load.iterations = UnsignedAttribute(element, "iterations", load.durationMs == 0 ? 1 : 0);
    load.ratePerSec = DoubleAttribute(element, "ratePerSec", 0.0);
    load.preAllocatedUsers = UnsignedAttribute(element, "preAllocatedUsers", 1);
    load.maxUsers = UnsignedAttribute(element, "maxUsers", load.preAllocatedUsers);
    for (const TiXmlElement* stage = element->FirstChildElement("Stage"); stage != nullptr;
        stage = stage->NextSiblingElement("Stage"))
    {
        load.stages.push_back({ UnsignedAttribute(stage, "durationMs", 0),
            DoubleAttribute(stage, "targetRatePerSec", 0.0) });
    }
}

// 计算有限 closed 工作负载中某场景按确定性权重周期获得的事务数。
// 校验阶段只使用整数运算，结果与运行时调度器保持一致。
uint64_t WeightedScenarioIterations(const Workload& workload, const std::string& scenarioId)
{
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
                ParseLoad(scenarioNode->FirstChildElement("Load"), scenario.load);
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

        // 阶段五：独立工作负载复用场景定义。没有 Workloads 时生成兼容项，保证
        // 旧 Scenario.Load、单场景选择和“全部场景”执行语义保持不变。
        if (const TiXmlElement* workloads = root->FirstChildElement("Workloads"))
        {
            plan.defaultWorkloadId = Attribute(workloads, "default");
            for (const TiXmlElement* node = workloads->FirstChildElement("Workload"); node != nullptr;
                node = node->NextSiblingElement("Workload"))
            {
                Workload workload;
                workload.id = Attribute(node, "id");
                workload.name = Attribute(node, "name", workload.id);
                workload.testType = Attribute(node, "type", "average");
                ParseLoad(node->FirstChildElement("Load"), workload.load);
                for (const TiXmlElement* reference = node->FirstChildElement("ScenarioRef");
                    reference != nullptr; reference = reference->NextSiblingElement("ScenarioRef"))
                {
                    workload.scenarios.push_back({ Attribute(reference, "id"),
                        UnsignedAttribute(reference, "weight", 1) });
                }
                if (const TiXmlElement* thresholds = node->FirstChildElement("Thresholds"))
                {
                    for (const TiXmlElement* item = thresholds->FirstChildElement("Threshold");
                        item != nullptr; item = item->NextSiblingElement("Threshold"))
                    {
                        Threshold threshold;
                        threshold.id = Attribute(item, "id");
                        threshold.name = Attribute(item, "name", threshold.id);
                        threshold.scope = Attribute(item, "scope", "run");
                        threshold.metric = Attribute(item, "metric");
                        threshold.target = Attribute(item, "target");
                        threshold.comparison = Attribute(item, "operator", "lte");
                        threshold.value = DoubleAttribute(item, "value", 0.0);
                        workload.thresholds.push_back(std::move(threshold));
                    }
                }
                plan.workloads.push_back(std::move(workload));
            }
        }
        const bool hadExplicitWorkloads = !plan.workloads.empty();
        if (!hadExplicitWorkloads)
        {
            plan.defaultWorkloadId = "legacy-all";
        }
        else if (plan.defaultWorkloadId.empty())
        {
            plan.defaultWorkloadId = plan.workloads.front().id;
        }
        Workload all;
        all.id = "legacy-all";
        all.name = "全部场景（兼容模式）";
        all.implicit = true;
        all.useScenarioLoads = true;
        for (const Scenario& scenario : plan.scenarios)
            all.scenarios.push_back({ scenario.id, 1 });
        plan.workloads.push_back(std::move(all));
        for (const Scenario& scenario : plan.scenarios)
        {
            Workload single;
            single.id = "legacy-" + scenario.id;
            single.name = scenario.name + "（兼容模式）";
            single.load = scenario.load;
            single.scenarios.push_back({ scenario.id, 1 });
            single.implicit = true;
            plan.workloads.push_back(std::move(single));
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
    std::set<std::string> requestLabels;
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
            outError = "Scenario.Load 兼容入口只允许 closed；arrival 必须放在 Workload.Load: " + scenario.id;
            return MTLOAD_E_CONFIG_SEMANTIC;
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
                requestLabels.insert(request.label);
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

    // 工作负载在场景语义校验完成后检查，确保引用、阶段曲线和阈值目标闭合。
    if (plan.workloads.empty())
    {
        outError = "计划至少需要一个 Workload 或可转换的 Scenario.Load";
        return MTLOAD_E_CONFIG_SEMANTIC;
    }
    std::set<std::string> workloadIds;
    bool defaultFound = false;
    for (const Workload& workload : plan.workloads)
    {
        if (workload.id.empty() || !workloadIds.insert(workload.id).second || workload.scenarios.empty())
        {
            outError = "Workload id 为空、重复或没有 ScenarioRef: " + workload.id;
            return MTLOAD_E_CONFIG_SEMANTIC;
        }
        if (workload.id == plan.defaultWorkloadId) defaultFound = true;
        uint64_t totalWeight = 0;
        std::set<std::string> workloadScenarioIds;
        for (const WorkloadScenarioRef& reference : workload.scenarios)
        {
            if (reference.weight == 0 || scenarioIds.find(reference.scenarioId) == scenarioIds.end() ||
                !workloadScenarioIds.insert(reference.scenarioId).second)
            {
                outError = "Workload ScenarioRef 不存在、重复或权重为 0: " + reference.scenarioId;
                return MTLOAD_E_CONFIG_SEMANTIC;
            }
            totalWeight += reference.weight;
        }
        if (totalWeight > 100000)
        {
            outError = "Workload 场景权重总和超过 100000: " + workload.id;
            return MTLOAD_E_CONFIG_SEMANTIC;
        }
        if (!workload.useScenarioLoads)
        {
            const Load& load = workload.load;
            if (load.mode == "closed")
            {
                if (load.threads == 0 || load.threads > 10000 ||
                    (load.durationMs == 0 && load.iterations == 0))
                {
                    outError = "closed Workload 的 users 或结束条件无效: " + workload.id;
                    return MTLOAD_E_CONFIG_SEMANTIC;
                }
                // 有限 closed 计划必须能用 uint64_t 精确表示，否则进度、吞吐和报告总数会失真。
                if (load.durationMs == 0)
                {
                    uint64_t workloadPlanned = 0;
                    for (const WorkloadScenarioRef& reference : workload.scenarios)
                    {
                        const Scenario* scenario = nullptr;
                        for (const Scenario& candidate : plan.scenarios)
                            if (candidate.id == reference.scenarioId) { scenario = &candidate; break; }
                        if (scenario == nullptr) continue;
                        uint64_t requestsPerIteration = 0;
                        for (const Step& step : scenario->steps)
                            for (const Request& request : step.requests)
                                requestsPerIteration += request.copies;
                        const uint64_t scenarioIterations = WeightedScenarioIterations(workload,
                            reference.scenarioId);
                        if (requestsPerIteration != 0 && scenarioIterations >
                            (std::numeric_limits<uint64_t>::max)() / requestsPerIteration)
                        {
                            outError = "closed Workload 计划请求数溢出: " + workload.id;
                            return MTLOAD_E_CONFIG_SEMANTIC;
                        }
                        const uint64_t scenarioRequests = scenarioIterations * requestsPerIteration;
                        if (workloadPlanned > (std::numeric_limits<uint64_t>::max)() - scenarioRequests)
                        {
                            outError = "closed Workload 计划请求总数溢出: " + workload.id;
                            return MTLOAD_E_CONFIG_SEMANTIC;
                        }
                        workloadPlanned += scenarioRequests;
                    }
                }
            }
            else if (load.mode == "arrival")
            {
                if (load.preAllocatedUsers == 0 || load.maxUsers < load.preAllocatedUsers ||
                    load.maxUsers > 10000)
                {
                    outError = "arrival Workload 的 preAllocatedUsers/maxUsers 无效: " + workload.id;
                    return MTLOAD_E_CONFIG_SEMANTIC;
                }
                double maximumRate = load.ratePerSec;
                uint64_t stageDuration = 0;
                for (const LoadStage& stage : load.stages)
                {
                    if (stage.durationMs == 0 || !std::isfinite(stage.targetRatePerSec) ||
                        stage.targetRatePerSec < 0.0)
                    {
                        outError = "arrival Stage durationMs 或 targetRatePerSec 无效: " + workload.id;
                        return MTLOAD_E_CONFIG_SEMANTIC;
                    }
                    stageDuration += stage.durationMs;
                    maximumRate = (std::max)(maximumRate, stage.targetRatePerSec);
                }
                if (!std::isfinite(load.ratePerSec) || load.ratePerSec < 0.0 || maximumRate <= 0.0 ||
                    (load.stages.empty() && load.durationMs == 0))
                {
                    outError = "arrival Workload 需要正到达率和 durationMs 或 Stage: " + workload.id;
                    return MTLOAD_E_CONFIG_SEMANTIC;
                }
                if (!load.stages.empty() && load.durationMs != 0 && load.durationMs != stageDuration)
                {
                    outError = "arrival Workload 的 durationMs 必须等于 Stage 总时长或省略: " + workload.id;
                    return MTLOAD_E_CONFIG_SEMANTIC;
                }
            }
            else
            {
                outError = "Workload.Load mode 只支持 closed/arrival: " + workload.id;
                return MTLOAD_E_UNSUPPORTED;
            }
        }

        std::set<std::string> thresholdIds;
        for (const Threshold& threshold : workload.thresholds)
        {
            if (threshold.id.empty() || !thresholdIds.insert(threshold.id).second ||
                !std::isfinite(threshold.value) || threshold.value < 0.0 ||
                (threshold.comparison != "lte" && threshold.comparison != "gte" && threshold.comparison != "eq"))
            {
                outError = "Threshold id 或 operator 无效: " + threshold.id;
                return MTLOAD_E_CONFIG_SEMANTIC;
            }
            if (threshold.scope == "run")
            {
                if (threshold.metric != "errorRate" && threshold.metric != "throughputPerSec" &&
                    threshold.metric != "droppedIterations")
                {
                    outError = "run Threshold metric 无效: " + threshold.metric;
                    return MTLOAD_E_CONFIG_SEMANTIC;
                }
            }
            else if (threshold.scope == "request")
            {
                if ((threshold.metric != "p95Ms" && threshold.metric != "p99Ms") ||
                    requestLabels.find(threshold.target) == requestLabels.end())
                {
                    outError = "request Threshold metric 或 target 无效: " + threshold.id;
                    return MTLOAD_E_CONFIG_SEMANTIC;
                }
            }
            else if (threshold.scope == "scenario")
            {
                if ((threshold.metric != "p95Ms" && threshold.metric != "errorRate") ||
                    scenarioIds.find(threshold.target) == scenarioIds.end())
                {
                    outError = "scenario Threshold metric 或 target 无效: " + threshold.id;
                    return MTLOAD_E_CONFIG_SEMANTIC;
                }
            }
            else
            {
                outError = "Threshold scope 只支持 run/request/scenario: " + threshold.id;
                return MTLOAD_E_CONFIG_SEMANTIC;
            }
        }
    }
    if (!defaultFound)
    {
        outError = "Workloads default 引用了不存在的工作负载: " + plan.defaultWorkloadId;
        return MTLOAD_E_CONFIG_SEMANTIC;
    }
    outError.clear();
    return MTLOAD_OK;
}
}
