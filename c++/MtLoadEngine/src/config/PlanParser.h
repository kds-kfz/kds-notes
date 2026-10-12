#pragma once

// XML、内联 Row 和 CSV 解析入口。函数只在调用线程执行，不发起网络请求。

#include "LoadPlan.h"

namespace mtload
{
// 读取并解析 UTF-8 XML 计划及其 CSV 依赖。
// 成功时覆盖 outPlan 并返回 MTLOAD_OK；失败时保持可诊断的 outError。
int ParsePlan(const std::string& pathUtf8, Plan& outPlan, std::string& outError);

// 对已解析模型执行 ID、交叉引用、范围、协议支持和请求总数溢出校验。
// 函数只读访问 plan，可在非运行线程调用；失败返回具体 MTLOAD_RESULT。
int ValidatePlan(const Plan& plan, std::string& outError);
}

