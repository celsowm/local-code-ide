#pragma once

#include "services/interfaces/ICodeIntelProvider.hpp"
#include "services/interfaces/IDiagnosticProvider.hpp"

#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace ide::adapters::diagnostics {

class LspTransport;
class LspProtocol;
class LspDocumentManager;
class LspFeatureProvider;

class LspClient final : public QObject {
    Q_OBJECT
public:
    explicit LspClient(const QString& program,
                       const QStringList& args,
                       std::function<QString(const QString&)> languageIdResolver = {},
                       QObject* parent = nullptr);
    ~LspClient() override;

    bool ensureStarted();
    bool isRunning() const;
    void stop();
    QString statusLine() const;

    int publishDocument(const QString& filePath, const QString& text);
    std::vector<ide::services::interfaces::Diagnostic> latestDiagnostics(const QString& filePath) const;
    int latestDiagnosticsVersion(const QString& filePath) const;
    std::vector<ide::services::interfaces::Diagnostic> publishAndCollect(const QString& filePath,
                                                                         const QString& text,
                                                                         int waitMs = 250);

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

    int requestCompletionsAsync(
        const QString& filePath,
        const QString& text,
        const ide::services::interfaces::EditorPosition& position);
    int requestHoverAsync(
        const QString& filePath,
        const QString& text,
        const ide::services::interfaces::EditorPosition& position);
    int requestDefinitionAsync(
        const QString& filePath,
        const QString& text,
        const ide::services::interfaces::EditorPosition& position);

signals:
    void diagnosticsPublished(const QString& filePath, int version, const QString& source);
    void serverStatusChanged(const QString& statusLine);
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
    void onTransportMessage(const QJsonObject& message);
    void handleInitializeResponse(const QJsonObject& message);
    void handlePublishDiagnostics(const QJsonObject& params);
    static std::vector<ide::services::interfaces::Diagnostic> parseDiagnostics(
        const QJsonArray& diagnostics,
        const QString& filePath,
        const QString& source);
    QString uriForPath(const QString& filePath) const;

    std::unique_ptr<LspTransport> m_transport;
    std::unique_ptr<LspProtocol> m_protocol;
    std::unique_ptr<LspDocumentManager> m_documentManager;
    std::unique_ptr<LspFeatureProvider> m_featureProvider;

    QHash<QString, std::vector<ide::services::interfaces::Diagnostic>> m_diagnosticsByUri;
    QHash<QString, int> m_diagnosticVersionsByUri;
};

} // namespace ide::adapters::diagnostics
