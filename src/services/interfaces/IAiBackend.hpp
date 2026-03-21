#pragma once

#include <QJsonObject>
#include <QString>

#include <vector>

namespace ide::services::interfaces {

struct AiToolCall {
    QString id;
    QString name;
    QJsonObject arguments;
};

struct AiToolDefinition {
    QString name;
    QString description;
    QJsonObject inputSchema;
    bool destructive = false;
};

struct AiMessage {
    QString role;
    QString content;
    QString toolCallId;
    QString toolName;
    std::vector<AiToolCall> toolCalls;
};

struct AiRequest {
    QString prompt;
    QString currentPath;
    QString documentText;
    QString diagnosticsText;
    QString gitSummary;
    QString workspaceContextText;
    QString systemPrompt;
    int maxOutputTokens = 512;
    double temperature = 0.2;
    int documentContextChars = 12000;
    bool includeDocument = true;
    bool includeDiagnostics = true;
    bool includeGitSummary = true;
    int workspaceContextChars = 16000;
    int workspaceContextMaxFiles = 4;
    bool includeWorkspaceContext = true;

    bool enableTools = false;
    int maxToolRounds = 4;
    std::vector<AiToolDefinition> tools;
    std::vector<AiMessage> messages;
};

struct AiResponse {
    QString content;
    std::vector<AiToolCall> toolCalls;
    QString finishReason;
    QString errorText;
    bool ok = true;
};

class IAiBackend {
public:
    virtual ~IAiBackend() = default;
    virtual AiResponse complete(const AiRequest& request) = 0;
    virtual QString name() const = 0;
    virtual QString statusLine() const = 0;
    virtual bool isAvailable() const = 0;
};

} // namespace ide::services::interfaces
