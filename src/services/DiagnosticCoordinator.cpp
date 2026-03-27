#include "services/DiagnosticCoordinator.hpp"

#include <QFileInfo>

namespace ide::services {
using Diagnostic = ide::services::interfaces::Diagnostic;

DiagnosticCoordinator::DiagnosticCoordinator(std::unique_ptr<ide::services::interfaces::IDiagnosticProvider> localProvider,
                                             LanguageServerHub* languageServerHub,
                                             QObject* parent)
    : QObject(parent)
    , m_localProvider(std::move(localProvider))
    , m_languageServerHub(languageServerHub) {
    m_debounceTimer.setSingleShot(true);
    m_debounceTimer.setInterval(350);
    connect(&m_debounceTimer, &QTimer::timeout, this, &DiagnosticCoordinator::analyzeNow);

    if (m_languageServerHub) {
        connect(m_languageServerHub, &LanguageServerHub::diagnosticsReady, this,
                [this](const QString& filePath,
                       const std::vector<Diagnostic>& diagnostics,
                       int version,
                       const QString&) {
            if (QFileInfo(filePath).absoluteFilePath() != QFileInfo(m_snapshot.filePath).absoluteFilePath()) {
                return;
            }
            if (version >= 0 && m_expectedLspVersion >= 0 && version < m_expectedLspVersion) {
                return;
            }
            m_lspDiagnostics = diagnostics;
            emitCombinedDiagnostics();
        });
        connect(m_languageServerHub, &LanguageServerHub::runtimeStatusChanged, this,
                [this](const QString& filePath, const QString& statusLine, const QString& providerLabel) {
            if (QFileInfo(filePath).absoluteFilePath() != QFileInfo(m_snapshot.filePath).absoluteFilePath()) {
                return;
            }
            updateStatus(statusLine, providerLabel);
        });
        connect(m_languageServerHub, &LanguageServerHub::toastRequested, this, &DiagnosticCoordinator::toastRequested);
    }
}

void DiagnosticCoordinator::setDocumentSnapshot(const QString& filePath, const QString& text, bool immediate) {
    const bool fileChanged = QFileInfo(filePath).absoluteFilePath() != QFileInfo(m_snapshot.filePath).absoluteFilePath();
    if (filePath == m_snapshot.filePath && text == m_snapshot.text && !immediate) {
        return;
    }

    m_snapshot.filePath = filePath;
    m_snapshot.text = text;
    m_snapshot.languageId = m_languageServerHub ? m_languageServerHub->languageIdForPath(filePath) : QStringLiteral("cpp");
    ++m_snapshot.revision;
    m_expectedLspVersion = -1;
    if (fileChanged) {
        m_lspDiagnostics.clear();
    }

    if (immediate) {
        analyzeNow();
        return;
    }
    m_debounceTimer.start();
}

void DiagnosticCoordinator::forceRefresh() {
    ++m_snapshot.revision;
    analyzeNow();
}

LanguageServerHub* DiagnosticCoordinator::languageServerHub() const {
    return m_languageServerHub;
}

QString DiagnosticCoordinator::activeProviderLabel() const {
    return m_providerLabel;
}

QString DiagnosticCoordinator::statusLine() const {
    return m_statusLine;
}

void DiagnosticCoordinator::analyzeNow() {
    if (m_snapshot.filePath.trimmed().isEmpty() || !m_localProvider) {
        return;
    }

    m_localDiagnostics = m_localProvider->analyze(m_snapshot.filePath, m_snapshot.text);

    if (!m_languageServerHub) {
        m_lspDiagnostics.clear();
        updateStatus(QStringLiteral("%1: basic mode").arg(m_snapshot.languageId), QStringLiteral("Basic Mode"));
        emitCombinedDiagnostics();
        return;
    }

    m_expectedLspVersion = m_languageServerHub->publishDocument(m_snapshot.filePath, m_snapshot.text);
    if (m_expectedLspVersion < 0) {
        m_lspDiagnostics.clear();
    } else {
        m_lspDiagnostics = m_languageServerHub->latestDiagnostics(m_snapshot.filePath);
    }

    const auto runtime = m_languageServerHub->runtimeStatus(m_snapshot.filePath);
    updateStatus(runtime.statusLine, runtime.providerLabel);
    emitCombinedDiagnostics();
}

void DiagnosticCoordinator::emitCombinedDiagnostics() {
    std::vector<Diagnostic> merged;
    merged.reserve(m_localDiagnostics.size() + m_lspDiagnostics.size());
    merged.insert(merged.end(), m_localDiagnostics.begin(), m_localDiagnostics.end());
    merged.insert(merged.end(), m_lspDiagnostics.begin(), m_lspDiagnostics.end());
    emit diagnosticsUpdated(merged, m_snapshot.revision);
}

void DiagnosticCoordinator::updateStatus(const QString& line, const QString& providerLabel) {
    if (m_statusLine != line) {
        m_statusLine = line;
        emit statusLineChanged(m_statusLine);
    }
    if (m_providerLabel != providerLabel) {
        m_providerLabel = providerLabel;
        emit providerLabelChanged(m_providerLabel);
    }
}

} // namespace ide::services

