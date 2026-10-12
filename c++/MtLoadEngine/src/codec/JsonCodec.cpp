#include "JsonCodec.h"

#include <json/json.h>

#include <cctype>
#include <limits>
#include <sstream>

namespace mtload
{
namespace
{
// 逐层解析对象成员和零基数组索引；非法语法、越界或缺失成员统一返回空指针。
const Json::Value* Resolve(const Json::Value& root, const std::string& path)
{
    const Json::Value* current = &root;
    size_t cursor = path.rfind("$.", 0) == 0 ? 2 : (path.rfind('$', 0) == 0 ? 1 : 0);
    while (cursor < path.size())
    {
        if (path[cursor] == '.') ++cursor;
        const size_t memberBegin = cursor;
        while (cursor < path.size() && path[cursor] != '.' && path[cursor] != '[') ++cursor;
        if (cursor > memberBegin)
        {
            const std::string member = path.substr(memberBegin, cursor - memberBegin);
            if (!current->isObject() || !current->isMember(member)) return nullptr;
            current = &(*current)[member];
        }
        while (cursor < path.size() && path[cursor] == '[')
        {
            const size_t indexBegin = ++cursor;
            unsigned int index = 0;
            while (cursor < path.size() && std::isdigit(static_cast<unsigned char>(path[cursor])))
            {
                const unsigned int digit = static_cast<unsigned int>(path[cursor] - '0');
                if (index > ((std::numeric_limits<unsigned int>::max)() - digit) / 10U) return nullptr;
                index = index * 10U + digit;
                ++cursor;
            }
            if (cursor == indexBegin || cursor >= path.size() || path[cursor] != ']' ||
                !current->isArray() || index >= current->size()) return nullptr;
            current = &(*current)[index];
            ++cursor;
        }
        if (cursor < path.size() && path[cursor] != '.') return nullptr;
    }
    return current;
}

// 把 JSON 标量转换为变量字符串；复合值保持紧凑 JSON 表示。
std::string ToString(const Json::Value& value)
{
    if (value.isString()) return value.asString();
    if (value.isBool()) return value.asBool() ? "true" : "false";
    if (value.isInt()) return std::to_string(value.asInt());
    if (value.isUInt()) return std::to_string(value.asUInt());
    if (value.isDouble())
    {
        std::ostringstream stream;
        stream << value.asDouble();
        return stream.str();
    }
    Json::FastWriter writer;
    std::string text = writer.write(value);
    if (!text.empty() && text.back() == '\n') text.pop_back();
    return text;
}

// 根据断言类型执行状态范围或 JSON 路径比较，并返回可定位的失败原因。
bool EvaluateAssertion(const Assertion& assertion, const Json::Value& root,
    std::string& error)
{
    const Json::Value* value = Resolve(root, assertion.path);
    if (assertion.op == "exists") return value != nullptr;
    if (assertion.op == "not_exists") return value == nullptr;
    if (value == nullptr)
    {
        error = "JSON 断言路径不存在: " + assertion.path;
        return false;
    }
    const std::string actual = ToString(*value);
    if (assertion.op == "eq" && actual == assertion.value) return true;
    if (assertion.op == "ne" && actual != assertion.value) return true;
    if (assertion.op == "gt" || assertion.op == "ge" || assertion.op == "lt" || assertion.op == "le")
    {
        try
        {
            const double left = std::stod(actual);
            const double right = std::stod(assertion.value);
            if (assertion.op == "gt" && left > right) return true;
            if (assertion.op == "ge" && left >= right) return true;
            if (assertion.op == "lt" && left < right) return true;
            if (assertion.op == "le" && left <= right) return true;
        }
        catch (...) {}
    }
    error = "JSON 断言失败: " + assertion.path + " actual=" + actual;
    return false;
}
}

// 先解析一次响应，再按配置顺序执行全部断言和提取，避免重复解析 JSON。
JsonEvaluation EvaluateJson(const Request& request, int httpStatus,
    const std::string& body)
{
    JsonEvaluation result;
    for (const Assertion& assertion : request.assertions)
    {
        if (assertion.kind == Assertion::Kind::HttpStatus &&
            (httpStatus < assertion.minStatus || httpStatus > assertion.maxStatus))
        {
            result.error = "HTTP 状态断言失败: " + std::to_string(httpStatus);
            return result;
        }
    }

    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(body, root, false))
    {
        result.error = "响应不是合法 JSON: " + reader.getFormatedErrorMessages();
        return result;
    }
    for (const Assertion& assertion : request.assertions)
    {
        if (assertion.kind == Assertion::Kind::Json && !EvaluateAssertion(assertion, root, result.error))
            return result;
    }
    for (const Extractor& extractor : request.extractors)
    {
        const Json::Value* value = Resolve(root, extractor.path);
        if (value == nullptr)
        {
            if (extractor.required)
            {
                result.error = "JSON 提取路径不存在: " + extractor.path;
                return result;
            }
            continue;
        }
        result.extracted[extractor.variable] = ToString(*value);
    }
    result.success = true;
    return result;
}
}
