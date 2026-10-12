#pragma once

// JSON 响应解析、断言和变量提取。解析结果只在当前请求处理期间存在。

#include <map>
#include <string>

#include "../config/LoadPlan.h"

namespace mtload
{
struct JsonEvaluation
{
    bool success = false;                           // 状态断言、JSON 断言和必填提取是否全部通过。
    std::string error;                              // 失败时的 UTF-8 诊断；成功时为空。
    std::map<std::string, std::string> extracted;   // 按变量名返回的提取值，归调用方所有。
};

// 解析响应 JSON、执行 request 中的断言并收集提取变量。
// 函数无共享状态且可并发调用；解析或校验失败通过 JsonEvaluation 返回。
JsonEvaluation EvaluateJson(const Request& request, int httpStatus,
    const std::string& body);
}
