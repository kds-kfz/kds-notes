#include "pch.h"
#include "JsonMessage.h"

#include <nlohmann/json.hpp>

#include <chrono>
#include <iomanip>
#include <limits>
#include <sstream>

namespace interface_test
{
namespace
{
using Json = nlohmann::ordered_json;

bool ParseSigned(const std::string& text, long long& value)
{
    try { size_t used = 0; value = std::stoll(text, &used, 10); return used == text.size(); }
    catch (...) { return false; }
}

bool ParseUnsigned(const std::string& text, unsigned long long& value)
{
    if (!text.empty() && text.front() == '-') return false;
    try { size_t used = 0; value = std::stoull(text, &used, 10); return used == text.size(); }
    catch (...) { return false; }
}

bool ParseNumber(const std::string& text, double& value)
{
    try { size_t used = 0; value = std::stod(text, &used); return used == text.size() && std::isfinite(value); }
    catch (...) { return false; }
}

bool BuildValue(const FieldDefinition& field, const std::string& expanded,
    Json& value, std::string& error)
{
    if (field.type == "string") value = expanded;
    else if (field.type == "integer")
    {
        long long number = 0;
        if (!ParseSigned(expanded, number)) { error = "字段不是合法 integer: " + field.pointer; return false; }
        value = number;
    }
    else if (field.type == "unsigned")
    {
        unsigned long long number = 0;
        if (!ParseUnsigned(expanded, number)) { error = "字段不是合法 unsigned: " + field.pointer; return false; }
        value = number;
    }
    else if (field.type == "number")
    {
        double number = 0.0;
        if (!ParseNumber(expanded, number)) { error = "字段不是合法 number: " + field.pointer; return false; }
        value = number;
    }
    else if (field.type == "boolean")
    {
        if (expanded == "true" || expanded == "1") value = true;
        else if (expanded == "false" || expanded == "0") value = false;
        else { error = "字段不是合法 boolean: " + field.pointer; return false; }
    }
    else if (field.type == "null") value = nullptr;
    else if (field.type == "object" || field.type == "array")
    {
        try { value = Json::parse(expanded); }
        catch (const std::exception& exception)
        {
            error = "字段 JSON 解析失败 " + field.pointer + ": " + exception.what();
            return false;
        }
        if ((field.type == "object" && !value.is_object()) ||
            (field.type == "array" && !value.is_array()))
        {
            error = "字段 JSON 类型与声明不一致: " + field.pointer;
            return false;
        }
    }
    else
    {
        error = "不支持的字段类型 " + field.type + ": " + field.pointer;
        return false;
    }
    return true;
}

std::string TypeName(const Json& value)
{
    if (value.is_string()) return "string";
    if (value.is_number_unsigned()) return "unsigned";
    if (value.is_number_integer()) return "integer";
    if (value.is_number_float()) return "number";
    if (value.is_boolean()) return "boolean";
    if (value.is_object()) return "object";
    if (value.is_array()) return "array";
    return "null";
}

std::string EditableValue(const Json& value)
{
    return value.is_string() ? value.get<std::string>() : value.dump();
}

std::string ScalarText(const Json& value)
{
    if (value.is_string()) return value.get<std::string>();
    if (value.is_null()) return {};
    return value.dump();
}
} // namespace

std::string ExpandVariables(const std::string& source, SessionVariables& session,
    std::string& error)
{
    error.clear();
    std::string expanded;
    size_t position = 0;
    while (position < source.size())
    {
        const size_t begin = source.find("${", position);
        if (begin == std::string::npos) { expanded.append(source, position, std::string::npos); break; }
        expanded.append(source, position, begin - position);
        const size_t end = source.find('}', begin + 2);
        if (end == std::string::npos)
        {
            error = "变量表达式缺少右花括号";
            return {};
        }
        const std::string name = source.substr(begin + 2, end - begin - 2);
        if (name == "nowUnixMs")
        {
            const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            expanded += std::to_string(now);
        }
        else if (name == "sequence")
            expanded += std::to_string(++session.sequence);
        else
        {
            const auto value = session.values.find(name);
            if (value == session.values.end())
            {
                error = "未定义变量: " + name;
                return {};
            }
            expanded += value->second;
        }
        position = end + 1;
    }
    return expanded;
}

bool BuildJsonFromFields(const std::vector<FieldDefinition>& fields,
    const std::string& rootType, SessionVariables& session, std::string& body,
    std::string& redactedBody, std::string& error)
{
    error.clear();
    Json actual = rootType == "array" ? Json::array() : Json::object();
    Json redacted = actual;
    for (const auto& field : fields)
    {
        if (!field.required && !field.enabled) continue;
        if (field.pointer.empty() || field.pointer.front() != '/')
        {
            error = "JSON Pointer 必须以 / 开头: " + field.pointer;
            return false;
        }
        std::string expandError;
        const std::string expanded = ExpandVariables(field.value, session, expandError);
        if (!expandError.empty()) { error = field.pointer + ": " + expandError; return false; }
        Json value;
        if (!BuildValue(field, expanded, value, error)) return false;
        try
        {
            const nlohmann::json_pointer<std::string> pointer(field.pointer);
            actual[pointer] = value;
            redacted[pointer] = field.sensitive ? Json("******") : value;
        }
        catch (const std::exception& exception)
        {
            error = "JSON Pointer 写入失败 " + field.pointer + ": " + exception.what();
            return false;
        }
    }
    body = actual.dump(2);
    redactedBody = redacted.dump(2);
    return true;
}

bool SynchronizeFieldsFromJson(const std::string& body,
    std::vector<FieldDefinition>& fields, std::string& rootType, std::string& error)
{
    error.clear();
    try
    {
        const Json document = Json::parse(body);
        if (!document.is_object() && !document.is_array())
        {
            error = "请求 JSON 根节点必须是 object 或 array";
            return false;
        }
        rootType = document.is_array() ? "array" : "object";
        const Json flattened = document.flatten();
        std::vector<FieldDefinition> synchronized;
        synchronized.reserve(flattened.size());
        for (auto iterator = flattened.begin(); iterator != flattened.end(); ++iterator)
        {
            FieldDefinition field;
            field.pointer = iterator.key();
            if (field.pointer.empty()) continue;
            field.type = TypeName(iterator.value());
            field.value = EditableValue(iterator.value());
            field.enabled = true;
            const auto old = std::find_if(fields.begin(), fields.end(), [&field](const auto& value) {
                return value.pointer == field.pointer;
            });
            if (old != fields.end())
            {
                field.required = old->required;
                field.sensitive = old->sensitive;
                field.description = old->description;
            }
            else field.description = "从原始 JSON 同步";
            synchronized.push_back(std::move(field));
        }
        fields = std::move(synchronized);
        return true;
    }
    catch (const std::exception& exception)
    {
        error = std::string("请求 JSON 解析失败: ") + exception.what();
        return false;
    }
}

bool FormatJson(const std::string& body, std::string& formatted, std::string& error)
{
    error.clear();
    try { formatted = Json::parse(body).dump(2); return true; }
    catch (const std::exception& exception)
    {
        error = std::string("JSON 格式化失败: ") + exception.what();
        return false;
    }
}

bool ApplyExtractRules(const std::string& responseBody,
    const std::vector<ExtractDefinition>& extracts, SessionVariables& session,
    std::vector<std::string>& extractedNames, std::string& error)
{
    error.clear();
    extractedNames.clear();
    if (extracts.empty()) return true;
    Json response;
    try { response = Json::parse(responseBody); }
    catch (const std::exception& exception)
    {
        error = std::string("响应不是合法 JSON: ") + exception.what();
        return false;
    }
    for (const auto& rule : extracts)
    {
        try
        {
            const nlohmann::json_pointer<std::string> pointer(rule.pointer);
            if (!response.contains(pointer))
            {
                error = "响应中不存在提取路径: " + rule.pointer;
                return false;
            }
            session.values[rule.variable] = ScalarText(response.at(pointer));
            if (rule.sensitive) session.sensitiveNames.insert(rule.variable);
            else session.sensitiveNames.erase(rule.variable);
            extractedNames.push_back(rule.variable);
        }
        catch (const std::exception& exception)
        {
            error = "响应变量提取失败 " + rule.pointer + ": " + exception.what();
            return false;
        }
    }
    return true;
}
} // namespace interface_test
