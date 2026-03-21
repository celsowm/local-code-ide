#include "adapters/ai/LlamaCppServerBackend.hpp"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

namespace ide::adapters::ai {

namespace {
QString normalizeBaseUrl(QString url) {
    url = url.trimmed();
    if (url.endsWith('/')) {
        url.chop(1);
    }
    return url;
}
} // namespace

LlamaCppServerBackend::LlamaCppServerBackend(QString baseUrl, QString modelName, int timeoutMs)
    : m_baseUrl(normalizeBaseUrl(std::move(baseUrl)))
    , m_modelName(std::move(modelName))
    , m_timeoutMs(timeoutMs) {}

ide::services::interfaces::AiResponse LlamaCppServerBackend::complete(const ide::services::interfaces::AiRequest& requestData) {
    QNetworkRequest request(QUrl(m_baseUrl + "/v1/chat/completions"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    const QJsonDocument payload(buildRequestPayload(requestData));
    QNetworkReply* reply = m_network.post(request, payload.toJson(QJsonDocument::Compact));

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, reply, [reply, &loop]() {
        if (reply->isRunning()) {
            reply->abort();
        }
        loop.quit();
    });

    timer.start(m_timeoutMs);
    loop.exec();

    const QByteArray body = reply->readAll();
    const auto error = reply->error();
    const QString errorText = reply->errorString();
    reply->deleteLater();

    if (error != QNetworkReply::NoError) {
        return {
            QStringLiteral("%1 Falha na chamada HTTP: %2").arg(errorPrefix(), errorText),
            {},
            {},
            QStringLiteral("%1 Falha na chamada HTTP: %2").arg(errorPrefix(), errorText),
            false
        };
    }

    QJsonParseError parseError;
    const QJsonDocument json = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !json.isObject()) {
        return {
            QStringLiteral("%1 Resposta JSON inválida: %2").arg(errorPrefix(), parseError.errorString()),
            {},
            {},
            QStringLiteral("%1 Resposta JSON inválida: %2").arg(errorPrefix(), parseError.errorString()),
            false
        };
    }

    return extractResponse(json);
}

QString LlamaCppServerBackend::name() const {
    return "llama.cpp Server Backend";
}

QString LlamaCppServerBackend::statusLine() const {
    return QString("%1 • model=%2").arg(m_baseUrl, m_modelName.isEmpty() ? "auto" : m_modelName);
}

bool LlamaCppServerBackend::isAvailable() const {
    return !m_baseUrl.isEmpty();
}

QJsonObject LlamaCppServerBackend::buildRequestPayload(const ide::services::interfaces::AiRequest& request) const {
    QJsonObject root{
        {QStringLiteral("messages"), serializeMessages(request)},
        {QStringLiteral("temperature"), request.temperature},
        {QStringLiteral("stream"), false},
        {QStringLiteral("max_tokens"), request.maxOutputTokens}
    };

    if (!m_modelName.isEmpty()) {
        root.insert(QStringLiteral("model"), m_modelName);
    }

    if (request.enableTools && !request.tools.empty()) {
        root.insert(QStringLiteral("tools"), serializeTools(request.tools));
        root.insert(QStringLiteral("tool_choice"), QStringLiteral("auto"));
    }

    return root;
}

QJsonArray LlamaCppServerBackend::serializeMessages(const ide::services::interfaces::AiRequest& request) const {
    QJsonArray messages;
    if (!request.messages.empty()) {
        for (const auto& message : request.messages) {
            QJsonObject obj;
            obj.insert(QStringLiteral("role"), message.role);

            if (message.role == QStringLiteral("assistant")) {
                obj.insert(
                    QStringLiteral("content"),
                    (message.content.isEmpty() && !message.toolCalls.empty())
                        ? QJsonValue(QJsonValue::Null)
                        : QJsonValue(message.content)
                );
                if (!message.toolCalls.empty()) {
                    obj.insert(QStringLiteral("tool_calls"), serializeToolCalls(message.toolCalls));
                }
            } else if (message.role == QStringLiteral("tool")) {
                obj.insert(QStringLiteral("content"), message.content);
                obj.insert(QStringLiteral("tool_call_id"), message.toolCallId);
                if (!message.toolName.isEmpty()) {
                    obj.insert(QStringLiteral("name"), message.toolName);
                }
            } else {
                obj.insert(QStringLiteral("content"), message.content);
            }

            messages.append(obj);
        }
        return messages;
    }

    messages.append(QJsonObject{
        {QStringLiteral("role"), QStringLiteral("system")},
        {QStringLiteral("content"), request.systemPrompt}
    });

    QString userMessage = QStringLiteral("Current file: %1\n")
        .arg(request.currentPath.isEmpty() ? QStringLiteral("untitled") : request.currentPath);
    if (request.includeDocument && !request.documentText.isEmpty()) {
        userMessage += QStringLiteral("\nCurrent document:\n```\n%1\n```\n").arg(request.documentText);
    }
    if (request.includeDiagnostics && !request.diagnosticsText.isEmpty()) {
        userMessage += QStringLiteral("\nDiagnostics:\n%1\n").arg(request.diagnosticsText);
    }
    if (request.includeGitSummary && !request.gitSummary.isEmpty()) {
        userMessage += QStringLiteral("\nGit summary:\n%1\n").arg(request.gitSummary);
    }
    if (request.includeWorkspaceContext && !request.workspaceContextText.isEmpty()) {
        userMessage += QStringLiteral("\nRelevant workspace context:\n%1\n").arg(request.workspaceContextText);
    }
    userMessage += QStringLiteral("\nUser request: %1").arg(request.prompt);

    messages.append(QJsonObject{
        {QStringLiteral("role"), QStringLiteral("user")},
        {QStringLiteral("content"), userMessage}
    });

    return messages;
}

QJsonArray LlamaCppServerBackend::serializeTools(const std::vector<ide::services::interfaces::AiToolDefinition>& tools) const {
    QJsonArray arr;
    for (const auto& tool : tools) {
        arr.append(QJsonObject{
            {QStringLiteral("type"), QStringLiteral("function")},
            {QStringLiteral("function"), QJsonObject{
                {QStringLiteral("name"), tool.name},
                {QStringLiteral("description"), tool.description},
                {QStringLiteral("parameters"), tool.inputSchema}
            }}
        });
    }
    return arr;
}

QJsonArray LlamaCppServerBackend::serializeToolCalls(const std::vector<ide::services::interfaces::AiToolCall>& toolCalls) const {
    QJsonArray arr;
    int idx = 0;
    for (const auto& call : toolCalls) {
        arr.append(QJsonObject{
            {QStringLiteral("id"), call.id.isEmpty() ? QStringLiteral("call_%1").arg(QString::number(++idx)) : call.id},
            {QStringLiteral("type"), QStringLiteral("function")},
            {QStringLiteral("function"), QJsonObject{
                {QStringLiteral("name"), call.name},
                {QStringLiteral("arguments"), QString::fromUtf8(QJsonDocument(call.arguments).toJson(QJsonDocument::Compact))}
            }}
        });
    }
    return arr;
}

ide::services::interfaces::AiResponse LlamaCppServerBackend::extractResponse(const QJsonDocument& json) const {
    ide::services::interfaces::AiResponse result;

    const QJsonObject root = json.object();
    const QJsonArray choices = root.value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty()) {
        result.ok = false;
        result.errorText = QStringLiteral("%1 Resposta sem `choices`.").arg(errorPrefix());
        result.content = result.errorText;
        return result;
    }

    const QJsonObject first = choices.first().toObject();
    result.finishReason = first.value(QStringLiteral("finish_reason")).toString();

    const QJsonObject message = first.value(QStringLiteral("message")).toObject();
    result.content = message.value(QStringLiteral("content")).toString();
    if (result.content.isEmpty()) {
        result.content = first.value(QStringLiteral("text")).toString();
    }

    const QJsonArray toolCalls = message.value(QStringLiteral("tool_calls")).toArray();
    for (const auto& entry : toolCalls) {
        const QJsonObject toolObject = entry.toObject();
        const QJsonObject functionObject = toolObject.value(QStringLiteral("function")).toObject();

        ide::services::interfaces::AiToolCall toolCall;
        toolCall.id = toolObject.value(QStringLiteral("id")).toString();
        toolCall.name = functionObject.value(QStringLiteral("name")).toString();

        const QJsonValue argumentsValue = functionObject.value(QStringLiteral("arguments"));
        if (argumentsValue.isObject()) {
            toolCall.arguments = argumentsValue.toObject();
        } else {
            QJsonParseError parseError;
            const QJsonDocument argsDoc = QJsonDocument::fromJson(argumentsValue.toString().toUtf8(), &parseError);
            if (parseError.error == QJsonParseError::NoError && argsDoc.isObject()) {
                toolCall.arguments = argsDoc.object();
            }
        }

        result.toolCalls.push_back(toolCall);
    }

    if (result.content.isEmpty() && result.toolCalls.empty()) {
        result.ok = false;
        result.errorText = QStringLiteral("%1 Não foi possível extrair conteúdo nem tool_calls.").arg(errorPrefix());
        result.content = result.errorText;
    }

    return result;
}

QString LlamaCppServerBackend::errorPrefix() const {
    return "[llama.cpp]";
}

} // namespace ide::adapters::ai
