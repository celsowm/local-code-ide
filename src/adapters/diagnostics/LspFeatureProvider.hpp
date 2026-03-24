#pragma once

#include "services/interfaces/ICodeIntelProvider.hpp"

#include <QHash>
#include <QObject>
#include <QJsonObject>
#include <optional>

namespace ide::adapters::diagnostics {

class LspTransport;
class LspDocumentManager;

class LspFeatureProvider final : public QObject {
    Q_OBJECT
public:
    explicit LspFeatureProvider(LspTransport& transport,
                                LspDocumentManager& documentManager,
                                QObject* parent = nullptr);

    // Sync (blocks using QEventLoop, not msleep)
    std::vector<ide::services::interfaces::CompletionItem> requestCompletions(
        const QString& filePath,
        const QString& text,
        const ide::services::interfaces::EditorPosition& position,
        int waitMs = 700);

    ide::services::interfaces::HoverInfo requestHover(
        const QString& filePath,
        const QString& text,
        const ide::services::interfaces::EditorPosition& position,
        int waitMs = 700);

    std::optional<ide::services::interfaces::DefinitionLocation> requestDefinition(
        const QString& filePath,
        const QString& text,
        const ide::services::interfaces::EditorPosition& position,
        int waitMs = 700);

    // Async (returns request ID, results via signals)
    int requestCompletionsAsync(const QString& filePath,
                                const QString& text,
                                const ide::services::interfaces::EditorPosition& position);

    int requestHoverAsync(const QString& filePath,
                          const QString& text,
                          const ide::services::interfaces::EditorPosition& position);

    int requestDefinitionAsync(const QString& filePath,
                               const QString& text,
                               const ide::services::interfaces::EditorPosition& position);

    void handleResponse(int requestId, const QJsonObject& response);

signals:
    void completionsReady(int requestId,
                          const QString& filePath,
                          const std::vector<ide::services::interfaces::CompletionItem>& items);
    void hoverReady(int requestId,
                    const QString& filePath,
                    const ide::services::interfaces::HoverInfo& info);
    void definitionReady(int requestId,
                         const QString& filePath,
                         bool found,
                         const ide::services::interfaces::DefinitionLocation& location);

private:
    int sendRequest(const QString& method, const QJsonObject& params);
    QJsonObject waitForResponse(int requestId, int waitMs);
    QJsonObject makeTextDocumentPositionParams(const QString& filePath,
                                               const QString& text,
                                               const ide::services::interfaces::EditorPosition& position);

    static std::vector<ide::services::interfaces::CompletionItem> parseCompletionItems(const QJsonValue& value);
    static ide::services::interfaces::HoverInfo parseHover(const QJsonValue& value);
    static std::optional<ide::services::interfaces::DefinitionLocation> parseDefinition(const QJsonValue& value);

    LspTransport& m_transport;
    LspDocumentManager& m_documentManager;
    int m_nextId = 1;
    QHash<int, QJsonObject> m_pendingResponses;
    QHash<int, QString> m_pendingCompletionByRequestId;
    QHash<int, QString> m_pendingHoverByRequestId;
    QHash<int, QString> m_pendingDefinitionByRequestId;
};

} // namespace ide::adapters::diagnostics
