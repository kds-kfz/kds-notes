#pragma once

// 每个虚拟用户独占变量上下文；运行中无需跨用户加锁。

#include <map>
#include <string>

namespace mtload
{
// 单个虚拟用户的变量容器。
// 对象由该用户线程独占，不提供跨线程同步，也不保存到计划或报告中。
class VariableContext
{
public:
    // 接管本线程的数据行与运行时变量副本；运行时变量只读且不能被提取结果覆盖。
    VariableContext(std::map<std::string, std::string> dataRow,
        std::map<std::string, std::string> runtimeVariables = {});

    // 展开 ${Name} 占位符；先查询会话变量，再查询数据行。
    // 未知变量返回 false 并设置 error，out 仅在成功时可作为请求内容使用。
    bool Expand(const std::string& text, std::string& out, std::string& error) const;

    // 设置当前虚拟用户的会话变量，供后续步骤展开；调用线程必须拥有本对象。
    void SetSession(const std::string& name, const std::string& value);

private:
    // 引擎生成的运行标识优先级最高，避免用户参数或响应提取破坏并发隔离。
    std::map<std::string, std::string> m_mapRuntimeVariables;
    std::map<std::string, std::string> m_mapDataRow;     // 当前用户分配的数据集行，生命周期等同本对象。
    std::map<std::string, std::string> m_mapSession;     // 响应提取产生的运行时变量，不跨用户共享。
};
}
