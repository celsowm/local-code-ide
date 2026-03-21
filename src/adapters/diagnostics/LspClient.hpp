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

    void publishDocument(const QString& filePath, const QString& text);
    std::vector<ide::services::interfaces::Diagnostic> latestDiagnostics(const QString& filePath) const;
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

private slots:
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();

private:
    int sendRequest(const QString& method, const QJsonObject& params = {});
    void sendNotification(const QString& method, const QJsonObject& params = {});
    void sendMessage(const QJsonObject& message);
    void parseBufferedMessages();
    void handleMessage(const QJsonObject& message);
    void sendInitialize();
    void ensureDocumentOpened(const QString& filePath, const QString& text);
    void waitUntilInitialized(int waitMs = 800);
    QJsonObject waitForResponse(int requestId, int waitMs);
    QJsonObject makeTextDocumentPositionParams(const QString& filePath,
                                              const QString& text,
                                              const ide::services::interfaces::EditorPosition& position);

    QString uriForPath(const QString& filePath) const;
    QString languageIdForPath(const QString& filePath) const;

    static std::vector<ide::services::interfaces::Diagnostic> parseDiagnostics(const QJsonArray& diagnostics);
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
    QHash<int, QJsonObject> m_pendingResponses;
};

} // namespace ide::adapters::diagnostics
