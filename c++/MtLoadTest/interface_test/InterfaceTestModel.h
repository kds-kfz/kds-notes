#pragma once

// 接口测试配置模型。该层只描述配置、协议能力和 URL 规则，不依赖 MFC 控件，
// 因而可同时被桌面程序和控制台契约测试复用。

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace interface_test
{
enum class TransportKind { Tcp, Http, Https, WebSocket };
enum class PayloadKind { Binary, Json, Protobuf };

struct TransportPayloadSupport
{
    bool allowed = false;
    bool implemented = false;
    std::string label;
};

struct EnvironmentDefinition { std::string id; std::string name; };

struct ServerProfile
{
    std::string id;
    std::string environmentId;
    std::string group;
    std::string name;
    TransportKind transport = TransportKind::Http;
    std::string scheme;
    std::string host;
    unsigned short port = 80;
    bool hasExplicitPort = false;
    std::string basePath;
    uint32_t connectTimeoutMs = 5000;
    uint32_t requestTimeoutMs = 15000;
    uint32_t maxResponseBytes = 8U * 1024U * 1024U;
    std::string acceptEncoding;
    bool tlsVerify = true;
    std::string tlsCaFile;
    std::vector<PayloadKind> payloads;
    std::string webSocketPath = "/";
    uint32_t heartbeatMs = 30000;
    size_t maxMessages = 5000;
};

struct VariableDefinition { std::string name; std::string value; bool sensitive = false; };
struct VariableSetDefinition
{
    std::string id;
    std::string name;
    std::vector<VariableDefinition> variables;
};

struct HeaderDefinition
{
    std::string name;
    std::string value;
    bool required = false;
    bool sensitive = false;
    std::string description;
};

struct FieldDefinition
{
    std::string pointer;
    std::string type = "string";
    bool required = false;
    bool enabled = true;
    bool sensitive = false;
    std::string value;
    std::string description;
};

struct ExtractDefinition { std::string variable; std::string pointer; bool sensitive = false; };

struct InterfaceCase
{
    std::string id;
    std::string name;
    std::string category;
    std::string serverGroup;
    PayloadKind payload = PayloadKind::Json;
    std::string method;
    std::string path;
    std::string risk = "read";
    std::string rootType = "object";
    bool receiveOnly = false;
    std::string functionDescription;
    std::string usageDescription;
    std::string authDescription;
    std::string sideEffectDescription;
    std::vector<HeaderDefinition> headers;
    std::vector<FieldDefinition> fields;
    std::vector<ExtractDefinition> extracts;
};

struct SessionVariables
{
    std::map<std::string, std::string> values;
    std::set<std::string> sensitiveNames;
    uint64_t sequence = 0;
};

class InterfaceTestConfig
{
public:
    bool Load(const std::string& path, std::string& error);
    bool SaveAtomic(const std::string& path, std::string& error) const;
    const ServerProfile* FindServer(const std::string& id) const;
    ServerProfile* FindServer(const std::string& id);
    const VariableSetDefinition* FindVariableSet(const std::string& id) const;
    const InterfaceCase* FindCase(const std::string& id) const;
    InterfaceCase* FindCase(const std::string& id);

    std::string defaultEnvironmentId;
    std::string defaultVariableSetId;
    std::vector<EnvironmentDefinition> environments;
    std::vector<ServerProfile> servers;
    std::vector<VariableSetDefinition> variableSets;
    std::vector<InterfaceCase> cases;
};

TransportPayloadSupport QueryTransportPayloadSupport(TransportKind transport, PayloadKind payload);
std::string ToDisplayName(TransportKind transport);
std::string ToDisplayName(PayloadKind payload);
std::string JoinUrl(const ServerProfile& server, const std::string& path);
} // namespace interface_test
