#pragma once

#include "services/LanguageServerHub.hpp"
#include "services/interfaces/IDiagnosticProvider.hpp"

#include <QObject>
#include <QTimer>
#include <memory>

namespace ide::services {

class DiagnosticCoordinator final : public QObject {
    Q_OBJECT
public:
    explicit DiagnosticCoordinator(std::unique_ptr<ide::services::interfaces::IDiagnosticProvider> localProvider,
                                   LanguageServerHub* languageServerHub,
                                   QObject* parent = nullptr);

    void setDocumentSnapshot(const QString& filePath, const QString& text, bool immediate);
    void forceRefresh();
    LanguageServerHub* languageServerHub() const;

    QString activeProviderLabel() const;
    QString statusLine() const;

signals:
    void diagnosticsUpdated(const std::vector<ide::services::interfaces::Diagnostic>& diagnostics, int revision);
    void statusLineChanged(const QString& statusLine);
    void providerLabelChanged(const QString& providerLabel);
    void toastRequested(const QString& message);

private:
    struct Snapshot {
        QString filePath;
        QString text;
        QString languageId;
        int revision = 0;
    };

    void analyzeNow();
    void emitCombinedDiagnostics();
    void updateStatus(const QString& line, const QString& providerLabel);

    std::unique_ptr<ide::services::interfaces::IDiagnosticProvider> m_localProvider;
    LanguageServerHub* m_languageServerHub = nullptr;

    QTimer m_debounceTimer;
    Snapshot m_snapshot;

    std::vector<ide::services::interfaces::Diagnostic> m_localDiagnostics;
    std::vector<ide::services::interfaces::Diagnostic> m_lspDiagnostics;

    QString m_providerLabel = QStringLiteral("Local Diagnostics");
    QString m_statusLine = QStringLiteral("diagnostics: idle");

    int m_expectedLspVersion = -1;
};

} // namespace ide::services
