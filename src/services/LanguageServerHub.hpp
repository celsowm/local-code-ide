#pragma once

#include "adapters/diagnostics/LspClient.hpp"
#include "services/LanguagePackService.hpp"
#include "services/interfaces/ICodeIntelProvider.hpp"
#include "services/interfaces/IDiagnosticProvider.hpp"

#include <QObject>
#include <QHash>
#include <functional>
#include <memory>

namespace ide::services {

class LanguageServerHub final : public QObject {
    Q_OBJECT
public:
    struct RuntimeStatus {
        bool lspReady = false;
        QString languageId;
        QString providerLabel;
        QString statusLine;
    };

    explicit LanguageServerHub(LanguagePackService* languagePackService,
                               QString globalOverrideCommand = {},
                               QStringList globalOverrideArgs = {},
                               bool allowDevInstallerPath = false,
                               QObject* parent = nullptr);

    int publishDocument(const QString& filePath, const QString& text);
    std::vector<ide::services::interfaces::Diagnostic> latestDiagnostics(const QString& filePath) const;
    std::vector<ide::services::interfaces::CompletionItem> completions(
        const QString& filePath,
        const QString& text,
        const ide::services::interfaces::EditorPosition& position
    );
    ide::services::interfaces::HoverInfo hover(
        const QString& filePath,
        const QString& text,
        const ide::services::interfaces::EditorPosition& position
    );
    std::optional<ide::services::interfaces::DefinitionLocation> definition(
        const QString& filePath,
        const QString& text,
        const ide::services::interfaces::EditorPosition& position
    );
    void completionsAsync(
        const QString& filePath,
        const QString& text,
        const ide::services::interfaces::EditorPosition& position,
        std::function<void(std::vector<ide::services::interfaces::CompletionItem>)> onReady
    );
    void hoverAsync(
        const QString& filePath,
        const QString& text,
        const ide::services::interfaces::EditorPosition& position,
        std::function<void(ide::services::interfaces::HoverInfo)> onReady
    );
    void definitionAsync(
        const QString& filePath,
        const QString& text,
        const ide::services::interfaces::EditorPosition& position,
        std::function<void(std::optional<ide::services::interfaces::DefinitionLocation>)> onReady
    );
    RuntimeStatus runtimeStatus(const QString& filePathOrLanguageId) const;
    QString languageIdForPath(const QString& filePath) const;
    void shutdown();

signals:
    void diagnosticsReady(const QString& filePath,
                          const std::vector<ide::services::interfaces::Diagnostic>& diagnostics,
                          int version,
                          const QString& providerLabel);
    void runtimeStatusChanged(const QString& filePath, const QString& statusLine, const QString& providerLabel);
    void toastRequested(const QString& message);

private:
    struct DocumentState {
        QString filePath;
        QString languageId;
        QString clientKey;
        int expectedVersion = -1;
        RuntimeStatus runtime;
    };

    struct ActiveClient {
        std::shared_ptr<ide::adapters::diagnostics::LspClient> client;
        QString key;
        QString providerLabel;
    };

    QString normalizedPath(const QString& filePath) const;
    std::optional<ide::services::LanguagePackService::ServerSpec> resolveServerSpec(const QString& languageId);
    ActiveClient ensureClient(const ide::services::LanguagePackService::ServerSpec& spec);
    void setRuntimeStatus(const QString& filePath, const RuntimeStatus& status);
    const DocumentState* documentStateFor(const QString& filePath) const;
    QString pendingRequestKey(const QString& clientKey, int requestId) const;

    LanguagePackService* m_languagePackService = nullptr;
    QString m_globalOverrideCommand;
    QStringList m_globalOverrideArgs;
    bool m_allowDevInstallerPath = false;

    QHash<QString, DocumentState> m_documentStatesByPath;
    QHash<QString, RuntimeStatus> m_statusByLanguage;
    QHash<QString, std::vector<ide::services::interfaces::Diagnostic>> m_diagnosticsByPath;
    QHash<QString, std::shared_ptr<ide::adapters::diagnostics::LspClient>> m_clientsByKey;
    QHash<QString, std::function<void(std::vector<ide::services::interfaces::CompletionItem>)>> m_pendingCompletionCallbacks;
    QHash<QString, std::function<void(ide::services::interfaces::HoverInfo)>> m_pendingHoverCallbacks;
    QHash<QString, std::function<void(std::optional<ide::services::interfaces::DefinitionLocation>)>> m_pendingDefinitionCallbacks;
};

} // namespace ide::services
