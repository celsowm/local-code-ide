#pragma once

#include "adapters/diagnostics/LspClient.hpp"
#include "services/LanguagePackService.hpp"
#include "services/interfaces/ICodeIntelProvider.hpp"
#include "services/interfaces/IDiagnosticProvider.hpp"

#include <QObject>
#include <QHash>
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
    RuntimeStatus runtimeStatus(const QString& filePathOrLanguageId) const;
    QString languageIdForPath(const QString& filePath) const;

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

    LanguagePackService* m_languagePackService = nullptr;
    QString m_globalOverrideCommand;
    QStringList m_globalOverrideArgs;
    bool m_allowDevInstallerPath = false;

    QHash<QString, DocumentState> m_documentStatesByPath;
    QHash<QString, RuntimeStatus> m_statusByLanguage;
    QHash<QString, std::vector<ide::services::interfaces::Diagnostic>> m_diagnosticsByPath;
    QHash<QString, std::shared_ptr<ide::adapters::diagnostics::LspClient>> m_clientsByKey;
};

} // namespace ide::services
