#pragma once

// JSON 消息层统一处理变量展开、类型转换、JSON Pointer 同步、脱敏和响应提取。
// 所有字符串均为 UTF-8；敏感值不得进入 redactedBody 或普通日志。

#include "InterfaceTestModel.h"

#include <string>
#include <vector>

namespace interface_test
{
std::string ExpandVariables(const std::string& source, SessionVariables& session,
    std::string& error);
bool BuildJsonFromFields(const std::vector<FieldDefinition>& fields,
    const std::string& rootType, SessionVariables& session, std::string& body,
    std::string& redactedBody, std::string& error);
bool SynchronizeFieldsFromJson(const std::string& body,
    std::vector<FieldDefinition>& fields, std::string& rootType, std::string& error);
bool FormatJson(const std::string& body, std::string& formatted, std::string& error);
bool ApplyExtractRules(const std::string& responseBody,
    const std::vector<ExtractDefinition>& extracts, SessionVariables& session,
    std::vector<std::string>& extractedNames, std::string& error);
} // namespace interface_test
