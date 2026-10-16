#include "pch.h"
#include "InterfaceTestModel.h"

#include <Windows.h>
#include <tinyxml.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <limits>
#include <set>
#include <sstream>

namespace interface_test
{
namespace
{
std::string Attribute(const TiXmlElement* element, const char* name,
    const std::string& fallback = {})
{
    if (element == nullptr) return fallback;
    const char* value = element->Attribute(name);
    return value == nullptr ? fallback : value;
}

bool BooleanAttribute(const TiXmlElement* element, const char* name, bool fallback)
{
    const std::string value = Attribute(element, name);
    if (value.empty()) return fallback;
    return value == "true" || value == "1" || value == "yes";
}

template<typename T>
bool UnsignedAttribute(const TiXmlElement* element, const char* name, T fallback,
    T maximum, T& result, std::string& error)
{
    const std::string text = Attribute(element, name);
    if (text.empty()) { result = fallback; return true; }
    try
    {
        size_t used = 0;
        const unsigned long long value = std::stoull(text, &used, 10);
        if (used != text.size() || value > static_cast<unsigned long long>(maximum))
            throw std::out_of_range(name);
        result = static_cast<T>(value);
        return true;
    }
    catch (...)
    {
        error = std::string("配置属性不是合法非负整数: ") + name + "=" + text;
        return false;
    }
}

bool ParseTransport(const std::string& text, TransportKind& value)
{
    if (text == "tcp") value = TransportKind::Tcp;
    else if (text == "http") value = TransportKind::Http;
    else if (text == "https") value = TransportKind::Https;
    else if (text == "web") value = TransportKind::WebSocket;
    else return false;
    return true;
}

bool ParsePayload(const std::string& text, PayloadKind& value)
{
    if (text == "binary") value = PayloadKind::Binary;
    else if (text == "json") value = PayloadKind::Json;
    else if (text == "protobuf") value = PayloadKind::Protobuf;
    else return false;
    return true;
}

const char* TransportValue(TransportKind value)
{
    switch (value)
    {
    case TransportKind::Tcp: return "tcp";
    case TransportKind::Http: return "http";
    case TransportKind::Https: return "https";
    case TransportKind::WebSocket: return "web";
    }
    return "http";
}

const char* PayloadValue(PayloadKind value)
{
    switch (value)
    {
    case PayloadKind::Binary: return "binary";
    case PayloadKind::Json: return "json";
    case PayloadKind::Protobuf: return "protobuf";
    }
    return "json";
}

std::string XmlEscape(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size());
    for (char character : value)
    {
        switch (character)
        {
        case '&': escaped += "&amp;"; break;
        case '<': escaped += "&lt;"; break;
        case '>': escaped += "&gt;"; break;
        case '"': escaped += "&quot;"; break;
        case '\'': escaped += "&apos;"; break;
        default: escaped += character; break;
        }
    }
    return escaped;
}

void AppendAttribute(std::ostringstream& stream, const char* name, const std::string& value)
{
    stream << ' ' << name << "=\"" << XmlEscape(value) << '"';
}

void AppendBoolean(std::ostringstream& stream, const char* name, bool value)
{
    stream << ' ' << name << "=\"" << (value ? "true" : "false") << '"';
}

bool ReadUtf8File(const std::string& path, std::string& text, std::string& error)
{
    std::ifstream file(std::filesystem::u8path(path), std::ios::binary);
    if (!file)
    {
        error = "无法打开接口测试配置: " + path;
        return false;
    }
    text.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    if (text.size() >= 3 && static_cast<unsigned char>(text[0]) == 0xef &&
        static_cast<unsigned char>(text[1]) == 0xbb && static_cast<unsigned char>(text[2]) == 0xbf)
    {
        error = "接口测试配置必须使用 UTF-8 无 BOM";
        return false;
    }
    return true;
}

template<typename Item, typename Selector>
bool HasUniqueValues(const std::vector<Item>& items, Selector selector)
{
    std::set<std::string> values;
    for (const auto& item : items)
        if (selector(item).empty() || !values.insert(selector(item)).second) return false;
    return true;
}

bool Validate(const InterfaceTestConfig& config, std::string& error)
{
    if (config.environments.empty() || config.servers.empty() || config.variableSets.empty() ||
        config.cases.empty())
    {
        error = "接口测试配置的 Environment、Server、VariableSet 和 Case 均不能为空";
        return false;
    }
    if (!HasUniqueValues(config.environments, [](const auto& value) { return value.id; }) ||
        !HasUniqueValues(config.servers, [](const auto& value) { return value.id; }) ||
        !HasUniqueValues(config.variableSets, [](const auto& value) { return value.id; }) ||
        !HasUniqueValues(config.cases, [](const auto& value) { return value.id; }))
    {
        error = "接口测试配置存在空 id 或重复 id";
        return false;
    }
    const auto environmentExists = [&config](const std::string& id) {
        return std::any_of(config.environments.begin(), config.environments.end(),
            [&id](const auto& value) { return value.id == id; });
    };
    if (!environmentExists(config.defaultEnvironmentId))
    {
        error = "默认 Environment 引用不存在: " + config.defaultEnvironmentId;
        return false;
    }
    for (const auto& server : config.servers)
    {
        if (!environmentExists(server.environmentId) || server.host.empty() || server.group.empty())
        {
            error = "Server 的环境、group 或 host 无效: " + server.id;
            return false;
        }
    }
    if (config.FindVariableSet(config.defaultVariableSetId) == nullptr)
    {
        error = "默认 VariableSet 引用不存在: " + config.defaultVariableSetId;
        return false;
    }
    for (const auto& item : config.cases)
    {
        const bool groupExists = std::any_of(config.servers.begin(), config.servers.end(),
            [&item](const auto& server) { return server.group == item.serverGroup; });
        if (!groupExists || item.method.empty())
        {
            error = "Case 的 Server group 或 method 无效: " + item.id;
            return false;
        }
        std::set<std::string> pointers;
        for (const auto& field : item.fields)
        {
            if (field.pointer.empty() || field.pointer.front() != '/' ||
                !pointers.insert(field.pointer).second)
            {
                error = "Case 存在无效或重复 JSON Pointer: " + item.id;
                return false;
            }
        }
    }
    return true;
}
} // namespace

bool InterfaceTestConfig::Load(const std::string& path, std::string& error)
{
    error.clear();
    InterfaceTestConfig loaded;
    std::string xml;
    if (!ReadUtf8File(path, xml, error)) return false;
    TiXmlDocument document;
    document.Parse(xml.c_str(), nullptr, TIXML_ENCODING_UTF8);
    if (document.Error())
    {
        error = std::string("接口测试 XML 解析失败: ") + document.ErrorDesc();
        return false;
    }
    const TiXmlElement* root = document.RootElement();
    if (root == nullptr || std::string(root->Value()) != "InterfaceTest")
    {
        error = "接口测试配置根节点必须是 InterfaceTest";
        return false;
    }

    const TiXmlElement* environmentsNode = root->FirstChildElement("Environments");
    loaded.defaultEnvironmentId = Attribute(environmentsNode, "default");
    for (const TiXmlElement* node = environmentsNode == nullptr ? nullptr :
        environmentsNode->FirstChildElement("Environment"); node != nullptr;
        node = node->NextSiblingElement("Environment"))
        loaded.environments.push_back({Attribute(node, "id"), Attribute(node, "name")});

    const TiXmlElement* serversNode = root->FirstChildElement("Servers");
    for (const TiXmlElement* node = serversNode == nullptr ? nullptr :
        serversNode->FirstChildElement("Server"); node != nullptr;
        node = node->NextSiblingElement("Server"))
    {
        ServerProfile server;
        server.id = Attribute(node, "id");
        server.environmentId = Attribute(node, "environmentRef");
        server.group = Attribute(node, "group");
        server.name = Attribute(node, "name");
        if (!ParseTransport(Attribute(node, "transport"), server.transport))
        {
            error = "未知 transport: " + Attribute(node, "transport");
            return false;
        }
        server.scheme = Attribute(node, "scheme");
        server.host = Attribute(node, "host");
        server.hasExplicitPort = node->Attribute("port") != nullptr;
        const unsigned short defaultPort = server.scheme == "https" || server.scheme == "wss" ? 443 :
            (server.scheme == "tcp" ? 0 : 80);
        if (!UnsignedAttribute(node, "port", defaultPort,
            (std::numeric_limits<unsigned short>::max)(), server.port, error) ||
            !UnsignedAttribute(node, "connectTimeoutMs", 5000U, 3600000U,
                server.connectTimeoutMs, error) ||
            !UnsignedAttribute(node, "requestTimeoutMs", 15000U, 3600000U,
                server.requestTimeoutMs, error) ||
            !UnsignedAttribute(node, "maxResponseBytes", 8U * 1024U * 1024U,
                1024U * 1024U * 1024U, server.maxResponseBytes, error)) return false;
        server.basePath = Attribute(node, "basePath");
        server.acceptEncoding = Attribute(node, "acceptEncoding");
        server.tlsVerify = BooleanAttribute(node, "tlsVerify", true);
        server.tlsCaFile = Attribute(node, "tlsCaFile");
        if (const TiXmlElement* payloads = node->FirstChildElement("Payloads"))
            for (const TiXmlElement* payload = payloads->FirstChildElement("Payload");
                payload != nullptr; payload = payload->NextSiblingElement("Payload"))
            {
                PayloadKind value;
                if (!ParsePayload(Attribute(payload, "type"), value))
                {
                    error = "未知 payload: " + Attribute(payload, "type");
                    return false;
                }
                server.payloads.push_back(value);
            }
        if (const TiXmlElement* web = node->FirstChildElement("WebSocket"))
        {
            server.webSocketPath = Attribute(web, "path", "/");
            if (!UnsignedAttribute(web, "heartbeatMs", 30000U, 3600000U,
                server.heartbeatMs, error) || !UnsignedAttribute(web, "maxMessages",
                static_cast<size_t>(5000), static_cast<size_t>(1000000), server.maxMessages, error))
                return false;
        }
        loaded.servers.push_back(std::move(server));
    }

    const TiXmlElement* variableSetsNode = root->FirstChildElement("VariableSets");
    loaded.defaultVariableSetId = Attribute(variableSetsNode, "default");
    for (const TiXmlElement* node = variableSetsNode == nullptr ? nullptr :
        variableSetsNode->FirstChildElement("VariableSet"); node != nullptr;
        node = node->NextSiblingElement("VariableSet"))
    {
        VariableSetDefinition set;
        set.id = Attribute(node, "id");
        set.name = Attribute(node, "name");
        for (const TiXmlElement* variable = node->FirstChildElement("Variable");
            variable != nullptr; variable = variable->NextSiblingElement("Variable"))
            set.variables.push_back({Attribute(variable, "name"), Attribute(variable, "value"),
                BooleanAttribute(variable, "sensitive", false)});
        loaded.variableSets.push_back(std::move(set));
    }

    const TiXmlElement* casesNode = root->FirstChildElement("Cases");
    for (const TiXmlElement* node = casesNode == nullptr ? nullptr :
        casesNode->FirstChildElement("Case"); node != nullptr;
        node = node->NextSiblingElement("Case"))
    {
        InterfaceCase item;
        item.id = Attribute(node, "id"); item.name = Attribute(node, "name");
        item.category = Attribute(node, "category"); item.serverGroup = Attribute(node, "serverRef");
        if (!ParsePayload(Attribute(node, "payload", "json"), item.payload))
        {
            error = "Case 的 payload 无效: " + item.id;
            return false;
        }
        item.method = Attribute(node, "method"); item.path = Attribute(node, "path");
        item.risk = Attribute(node, "risk", "read"); item.rootType = Attribute(node, "rootType", "object");
        item.receiveOnly = BooleanAttribute(node, "receiveOnly", false);
        item.functionDescription = Attribute(node, "function");
        item.usageDescription = Attribute(node, "usage"); item.authDescription = Attribute(node, "auth");
        item.sideEffectDescription = Attribute(node, "sideEffect");
        if (const TiXmlElement* headers = node->FirstChildElement("Headers"))
            for (const TiXmlElement* header = headers->FirstChildElement("Header");
                header != nullptr; header = header->NextSiblingElement("Header"))
                item.headers.push_back({Attribute(header, "name"), Attribute(header, "value"),
                    BooleanAttribute(header, "required", false),
                    BooleanAttribute(header, "sensitive", false), Attribute(header, "description")});
        if (const TiXmlElement* fields = node->FirstChildElement("Fields"))
            for (const TiXmlElement* field = fields->FirstChildElement("Field");
                field != nullptr; field = field->NextSiblingElement("Field"))
            {
                FieldDefinition value;
                value.pointer = Attribute(field, "pointer"); value.type = Attribute(field, "type", "string");
                value.required = BooleanAttribute(field, "required", false);
                value.enabled = value.required || BooleanAttribute(field, "enabled", true);
                value.sensitive = BooleanAttribute(field, "sensitive", false);
                value.value = Attribute(field, "value"); value.description = Attribute(field, "description");
                item.fields.push_back(std::move(value));
            }
        if (const TiXmlElement* extracts = node->FirstChildElement("Extracts"))
            for (const TiXmlElement* extract = extracts->FirstChildElement("Extract");
                extract != nullptr; extract = extract->NextSiblingElement("Extract"))
                item.extracts.push_back({Attribute(extract, "variable"), Attribute(extract, "pointer"),
                    BooleanAttribute(extract, "sensitive", false)});
        loaded.cases.push_back(std::move(item));
    }
    if (!Validate(loaded, error)) return false;
    *this = std::move(loaded);
    return true;
}

bool InterfaceTestConfig::SaveAtomic(const std::string& path, std::string& error) const
{
    error.clear();
    if (!Validate(*this, error)) return false;
    std::ostringstream output;
    output << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n<InterfaceTest version=\"1\">\r\n";
    output << "  <Environments"; AppendAttribute(output, "default", defaultEnvironmentId); output << ">\r\n";
    for (const auto& value : environments)
    {
        output << "    <Environment"; AppendAttribute(output, "id", value.id);
        AppendAttribute(output, "name", value.name); output << " />\r\n";
    }
    output << "  </Environments>\r\n  <Servers>\r\n";
    for (const auto& value : servers)
    {
        output << "    <Server"; AppendAttribute(output, "id", value.id);
        AppendAttribute(output, "environmentRef", value.environmentId); AppendAttribute(output, "group", value.group);
        AppendAttribute(output, "name", value.name); AppendAttribute(output, "transport", TransportValue(value.transport));
        AppendAttribute(output, "scheme", value.scheme); AppendAttribute(output, "host", value.host);
        if (value.hasExplicitPort) output << " port=\"" << value.port << '"';
        AppendAttribute(output, "basePath", value.basePath); output << " connectTimeoutMs=\"" << value.connectTimeoutMs
            << "\" requestTimeoutMs=\"" << value.requestTimeoutMs << "\" maxResponseBytes=\"" << value.maxResponseBytes << '"';
        if (!value.acceptEncoding.empty()) AppendAttribute(output, "acceptEncoding", value.acceptEncoding);
        AppendBoolean(output, "tlsVerify", value.tlsVerify);
        if (!value.tlsCaFile.empty()) AppendAttribute(output, "tlsCaFile", value.tlsCaFile);
        output << ">\r\n      <Payloads>";
        for (auto payload : value.payloads)
        {
            output << "<Payload"; AppendAttribute(output, "type", PayloadValue(payload)); output << " />";
        }
        output << "</Payloads>\r\n";
        if (value.transport == TransportKind::WebSocket)
        {
            output << "      <WebSocket"; AppendAttribute(output, "path", value.webSocketPath);
            output << " heartbeatMs=\"" << value.heartbeatMs << "\" maxMessages=\"" << value.maxMessages << "\" />\r\n";
        }
        output << "    </Server>\r\n";
    }
    output << "  </Servers>\r\n  <VariableSets"; AppendAttribute(output, "default", defaultVariableSetId); output << ">\r\n";
    for (const auto& set : variableSets)
    {
        output << "    <VariableSet"; AppendAttribute(output, "id", set.id); AppendAttribute(output, "name", set.name); output << ">\r\n";
        for (const auto& value : set.variables)
        {
            output << "      <Variable"; AppendAttribute(output, "name", value.name); AppendAttribute(output, "value", value.value);
            AppendBoolean(output, "sensitive", value.sensitive); output << " />\r\n";
        }
        output << "    </VariableSet>\r\n";
    }
    output << "  </VariableSets>\r\n  <Cases>\r\n";
    for (const auto& item : cases)
    {
        output << "    <Case"; AppendAttribute(output, "id", item.id); AppendAttribute(output, "name", item.name);
        AppendAttribute(output, "category", item.category); AppendAttribute(output, "serverRef", item.serverGroup);
        AppendAttribute(output, "payload", PayloadValue(item.payload)); AppendAttribute(output, "method", item.method);
        AppendAttribute(output, "path", item.path); AppendAttribute(output, "risk", item.risk);
        if (item.rootType != "object") AppendAttribute(output, "rootType", item.rootType);
        if (item.receiveOnly) AppendBoolean(output, "receiveOnly", true);
        AppendAttribute(output, "function", item.functionDescription); AppendAttribute(output, "usage", item.usageDescription);
        AppendAttribute(output, "auth", item.authDescription); AppendAttribute(output, "sideEffect", item.sideEffectDescription);
        output << ">\r\n      <Headers>";
        for (const auto& value : item.headers)
        {
            output << "<Header"; AppendAttribute(output, "name", value.name); AppendAttribute(output, "value", value.value);
            if (value.required) AppendBoolean(output, "required", true); if (value.sensitive) AppendBoolean(output, "sensitive", true);
            AppendAttribute(output, "description", value.description); output << " />";
        }
        output << "</Headers>\r\n      <Fields>";
        for (const auto& value : item.fields)
        {
            output << "<Field"; AppendAttribute(output, "pointer", value.pointer); AppendAttribute(output, "type", value.type);
            if (value.required) AppendBoolean(output, "required", true); if (!value.required || !value.enabled) AppendBoolean(output, "enabled", value.enabled);
            if (value.sensitive) AppendBoolean(output, "sensitive", true); AppendAttribute(output, "value", value.value);
            AppendAttribute(output, "description", value.description); output << " />";
        }
        output << "</Fields>\r\n      <Extracts>";
        for (const auto& value : item.extracts)
        {
            output << "<Extract"; AppendAttribute(output, "variable", value.variable); AppendAttribute(output, "pointer", value.pointer);
            if (value.sensitive) AppendBoolean(output, "sensitive", true); output << " />";
        }
        output << "</Extracts>\r\n    </Case>\r\n";
    }
    output << "  </Cases>\r\n</InterfaceTest>\r\n";

    const std::filesystem::path target = std::filesystem::u8path(path);
    const auto suffix = std::to_string(::GetCurrentProcessId()) + "." + std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    const std::filesystem::path temporary = target.parent_path() /
        (target.filename().u8string() + ".tmp." + suffix);
    {
        std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
        const std::string text = output.str();
        if (!file || !file.write(text.data(), static_cast<std::streamsize>(text.size())) || !file.flush())
        {
            error = "无法写入接口测试临时配置";
            std::error_code ignored; std::filesystem::remove(temporary, ignored);
            return false;
        }
    }
    InterfaceTestConfig verified;
    if (!verified.Load(temporary.u8string(), error))
    {
        std::error_code ignored; std::filesystem::remove(temporary, ignored);
        error = "接口测试配置复检失败: " + error;
        return false;
    }
    if (!::MoveFileExW(temporary.wstring().c_str(), target.wstring().c_str(),
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        error = "原子替换接口测试配置失败，Win32=" + std::to_string(::GetLastError());
        std::error_code ignored; std::filesystem::remove(temporary, ignored);
        return false;
    }
    return true;
}

const ServerProfile* InterfaceTestConfig::FindServer(const std::string& id) const
{ auto it = std::find_if(servers.begin(), servers.end(), [&id](const auto& value) { return value.id == id; }); return it == servers.end() ? nullptr : &*it; }
ServerProfile* InterfaceTestConfig::FindServer(const std::string& id)
{ return const_cast<ServerProfile*>(static_cast<const InterfaceTestConfig*>(this)->FindServer(id)); }
const VariableSetDefinition* InterfaceTestConfig::FindVariableSet(const std::string& id) const
{ auto it = std::find_if(variableSets.begin(), variableSets.end(), [&id](const auto& value) { return value.id == id; }); return it == variableSets.end() ? nullptr : &*it; }
const InterfaceCase* InterfaceTestConfig::FindCase(const std::string& id) const
{ auto it = std::find_if(cases.begin(), cases.end(), [&id](const auto& value) { return value.id == id; }); return it == cases.end() ? nullptr : &*it; }
InterfaceCase* InterfaceTestConfig::FindCase(const std::string& id)
{ return const_cast<InterfaceCase*>(static_cast<const InterfaceTestConfig*>(this)->FindCase(id)); }

TransportPayloadSupport QueryTransportPayloadSupport(TransportKind transport, PayloadKind payload)
{
    TransportPayloadSupport result;
    result.allowed = !((transport == TransportKind::Http || transport == TransportKind::Https) && payload == PayloadKind::Binary);
    result.implemented = result.allowed && payload == PayloadKind::Json && transport != TransportKind::Tcp;
    result.label = ToDisplayName(payload) + (result.implemented ? "" : "（待开发）");
    return result;
}

std::string ToDisplayName(TransportKind value)
{
    switch (value) { case TransportKind::Tcp: return "TCP"; case TransportKind::Http: return "HTTP";
    case TransportKind::Https: return "HTTPS"; case TransportKind::WebSocket: return "WebSocket/WSS"; }
    return "未知";
}

std::string ToDisplayName(PayloadKind value)
{
    switch (value) { case PayloadKind::Binary: return "Binary"; case PayloadKind::Json: return "JSON";
    case PayloadKind::Protobuf: return "Protobuf"; }
    return "未知";
}

std::string JoinUrl(const ServerProfile& server, const std::string& path)
{
    std::string url = server.scheme + "://" + server.host;
    if (server.hasExplicitPort) url += ":" + std::to_string(server.port);
    auto appendPath = [&url](const std::string& value) {
        if (value.empty()) return;
        if (url.back() == '/' && value.front() == '/') url += value.substr(1);
        else if (url.back() != '/' && value.front() != '/') url += "/" + value;
        else url += value;
    };
    appendPath(server.basePath); appendPath(path);
    return url;
}
} // namespace interface_test
