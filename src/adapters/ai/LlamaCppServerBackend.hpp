#pragma once

#include "services/interfaces/IAiBackend.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QString>

namespace ide::adapters::ai {

class LlamaCppServerBackend final : public ide::services::interfaces::IAiBackend {
public:
    LlamaCppServerBackend(QString baseUrl, QString modelName, int timeoutMs = 30000);

    ide::services::interfaces::AiResponse complete(const ide::services::interfaces::AiRequest& request) override;
    QString name() const override;
    QString statusLine() const override;
    bool isAvailable() const override;

private:
    QJsonObject buildRequestPayload(const ide::services::interfaces::AiRequest& request) const;
    QJsonArray serializeMessages(const ide::services::interfaces::AiRequest& request) const;
    QJsonArray serializeTools(const std::vector<ide::services::interfaces::AiToolDefinition>& tools) const;
    QJsonArray serializeToolCalls(const std::vector<ide::services::interfaces::AiToolCall>& toolCalls) const;
    ide::services::interfaces::AiResponse extractResponse(const QJsonDocument& json) const;
    QString errorPrefix() const;

    QString m_baseUrl;
    QString m_modelName;
    int m_timeoutMs = 30000;
    mutable QNetworkAccessManager m_network;
};

} // namespace ide::adapters::ai
