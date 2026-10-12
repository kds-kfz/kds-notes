#include "VariableContext.h"

namespace mtload
{
// 接管当前虚拟用户的数据行副本，保证运行中不依赖共享 DataSet 容器。
VariableContext::VariableContext(std::map<std::string, std::string> dataRow,
    std::map<std::string, std::string> runtimeVariables)
    : runtimeVariables_(std::move(runtimeVariables)), dataRow_(std::move(dataRow))
{
}

// 关键实现：扫描 ${...} 占位符并优先读取会话变量，未知变量立即失败而不发送半成品。
bool VariableContext::Expand(const std::string& text, std::string& out, std::string& error) const
{
    out.clear();
    size_t cursor = 0;
    while (cursor < text.size())
    {
        const size_t begin = text.find("${", cursor);
        if (begin == std::string::npos)
        {
            out.append(text, cursor, std::string::npos);
            return true;
        }
        out.append(text, cursor, begin - cursor);
        const size_t end = text.find('}', begin + 2);
        if (end == std::string::npos)
        {
            error = "变量表达式缺少右花括号";
            return false;
        }
        const std::string name = text.substr(begin + 2, end - begin - 2);
        auto runtime = runtimeVariables_.find(name);
        auto session = session_.find(name);
        auto data = dataRow_.find(name);
        if (runtime != runtimeVariables_.end()) out += runtime->second;
        else if (session != session_.end()) out += session->second;
        else if (data != dataRow_.end()) out += data->second;
        else
        {
            error = "未定义变量: " + name;
            return false;
        }
        cursor = end + 1;
    }
    return true;
}

// 关键实现：仅修改本虚拟用户拥有的 session_，后续步骤即可读取提取结果。
void VariableContext::SetSession(const std::string& name, const std::string& value)
{
    session_[name] = value;
}
}
