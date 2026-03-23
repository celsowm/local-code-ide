#pragma once

#include "services/interfaces/ICodeIntelProvider.hpp"
#include "services/interfaces/IDiagnosticProvider.hpp"

#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QProcess>
#include <QSet>
#include <QString>
#include <QStringList>
#include <optional>
#include <vector>

namespace ide::adapters::diagnostics {

class LspClient final : public QObject {
    Q_OBJECT
public:
    explicit LspClient(QString program, QStringList args, QObject* parent = nullptr);

    bool ensureStarted();
    bool isRunning() const;
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
        int waitMs = 700
    );

    ide::services::interfaces::HoverInfo requestHover(
        const QString& filePath,
        const QString& text,
        const ide::services::interfaces::EditorPosition& position,
        int waitMs = 700
    );

    std::optional<ide::services::interfaces::DefinitionLocation> requestDefinition(
        const QString& filePath,
        const QString& text,
        const ide::services::interfaces::EditorPosition& position,
        int waitMs = 700
    );
    int requestCompletionsAsync(
        const QString& filePath,
        const QString& text,
        const ide::services::interfaces::EditorPosition& position
    );
    int requestHoverAsync(
        const QString& filePath,
        const QString& text,
        const ide::services::interfaces::EditorPosition& position
    );
    int requestDefinitionAsync(
        const QString& filePath,
        const QString& text,
        const ide::services::interfaces::EditorPosition& position
    );

private slots:
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();

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
    struct PendingDocumentOpen {
        QString filePath;
        QString text;
    };

    int sendRequest(const QString& method, const QJsonObject& params = {});
    void sendNotification(const QString& method, const QJsonObject& params = {});
    void sendMessage(const QJsonObject& message);
    void parseBufferedMessages();
    void handleMessage(const QJsonObject& message);
    void sendInitialize();
    bool ensureDocumentOpened(const QString& filePath, const QString& text, int version);
    void waitUntilInitialized(int waitMs = 800);
    QJsonObject waitForResponse(int requestId, int waitMs);
    QJsonObject makeTextDocumentPositionParams(const QString& filePath,
                                              const QString& text,
                                              const ide::services::interfaces::EditorPosition& position);
    void flushPendingDocumentOpens();

    QString uriForPath(const QString& filePath) const;
    QString languageIdForPath(const QString& filePath) const;

    static std::vector<ide::services::interfaces::Diagnostic> parseDiagnostics(const QJsonArray& diagnostics,
                                                                              const QString& filePath,
                                                                              const QString& source);
    static std::vector<ide::services::interfaces::CompletionItem> parseCompletionItems(const QJsonValue& value);
    static ide::services::interfaces::HoverInfo parseHover(const QJsonValue& value);
    static std::optional<ide::services::interfaces::DefinitionLocation> parseDefinition(const QJsonValue& value);

    QString m_program;
    QStringList m_args;
    QProcess m_process;
    QByteArray m_stdoutBuffer;
    QByteArray m_stderrBuffer;
    bool m_initialized = false;
    int m_nextId = 1;
    QSet<QString> m_openDocuments;
    QHash<QString, int> m_documentVersions;
    QHash<QString, std::vector<ide::services::interfaces::Diagnostic>> m_diagnosticsByUri;
    QHash<QString, int> m_diagnosticVersionsByUri;
    QHash<int, QJsonObject> m_pendingResponses;
    QHash<QString, PendingDocumentOpen> m_pendingDocumentOpensByUri;
    QHash<int, QString> m_pendingCompletionByRequestId;
    QHash<int, QString> m_pendingHoverByRequestId;
    QHash<int, QString> m_pendingDefinitionByRequestId;
};

} // namespace ide::adapters::diagnostics
