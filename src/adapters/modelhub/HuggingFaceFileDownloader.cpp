#include "adapters/modelhub/HuggingFaceFileDownloader.hpp"

#include "adapters/modelhub/HfCliDownloadBackend.hpp"
#include "adapters/modelhub/QtHttpDownloadBackend.hpp"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QTextStream>
#include <QtGlobal>

namespace ide::adapters::modelhub {

namespace {
QString repoCacheDirName(const QString& repoId) {
    QString normalized = repoId.trimmed();
    normalized.replace('/', QStringLiteral("--"));
    return QStringLiteral("models--") + normalized;
}
}

HuggingFaceFileDownloader::HuggingFaceFileDownloader(QString token, QObject* parent)
    : QObject(parent)
    , m_token(std::move(token))
    , m_strategy(parseStrategy())
    , m_qtBackend(std::make_unique<QtHttpDownloadBackend>(this))
    , m_hfBackend(std::make_unique<HfCliDownloadBackend>(this)) {
    bindBackendSignals(m_qtBackend.get());
    bindBackendSignals(m_hfBackend.get());
}

bool HuggingFaceFileDownloader::isActive() const {
    return m_activeBackend && m_activeBackend->isActive();
}

QString HuggingFaceFileDownloader::localPath() const { return m_localPath; }
QString HuggingFaceFileDownloader::statusLine() const { return m_statusLine; }
qint64 HuggingFaceFileDownloader::receivedBytes() const { return m_receivedBytes; }
qint64 HuggingFaceFileDownloader::totalBytes() const { return m_totalBytes; }
double HuggingFaceFileDownloader::speedBytesPerSec() const { return m_speedBytesPerSec; }
qint64 HuggingFaceFileDownloader::etaSeconds() const { return m_etaSeconds; }
QString HuggingFaceFileDownloader::phase() const { return m_phase; }
QString HuggingFaceFileDownloader::progressDetails() const { return m_progressDetails; }
QString HuggingFaceFileDownloader::diagnosticsText() const { return m_diagnosticsText; }
QString HuggingFaceFileDownloader::backendName() const { return m_backendName; }
bool HuggingFaceFileDownloader::hfCliAvailable() const { return m_hfBackend && m_hfBackend->isAvailable(); }

HuggingFaceFileDownloader::Strategy HuggingFaceFileDownloader::parseStrategy() {
    const QString value = QProcessEnvironment::systemEnvironment()
                              .value(QStringLiteral("LOCALCODEIDE_HF_DOWNLOADER"))
                              .trimmed()
                              .toLower();
    if (value == QStringLiteral("qt")) {
        return Strategy::QtOnly;
    }
    if (value == QStringLiteral("hf")) {
        return Strategy::HfOnly;
    }
    return Strategy::Auto;
}

QString HuggingFaceFileDownloader::appLogPath() {
    QString root = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (root.trimmed().isEmpty()) {
        root = QDir::homePath();
    }
    const QString dir = QDir(root).filePath(QStringLiteral("logs"));
    QDir().mkpath(dir);
    return QDir(dir).filePath(QStringLiteral("modelhub-download.log"));
}

QString HuggingFaceFileDownloader::formatBytes(qint64 bytes) {
    if (bytes < 0) {
        return QStringLiteral("?");
    }
    static const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    double value = static_cast<double>(bytes);
    int idx = 0;
    while (value >= 1024.0 && idx < 4) {
        value /= 1024.0;
        ++idx;
    }
    return QStringLiteral("%1 %2").arg(QString::number(value, 'f', value >= 100.0 ? 0 : (value >= 10.0 ? 1 : 2)), units[idx]);
}

QString HuggingFaceFileDownloader::formatRate(double bytesPerSec) {
    if (bytesPerSec <= 0.0) {
        return QStringLiteral("—");
    }
    return QStringLiteral("%1/s").arg(formatBytes(static_cast<qint64>(bytesPerSec)));
}

QString HuggingFaceFileDownloader::formatEta(qint64 seconds) {
    if (seconds < 0) {
        return QStringLiteral("—");
    }
    const qint64 mins = seconds / 60;
    const qint64 secs = seconds % 60;
    if (mins > 0) {
        return QStringLiteral("%1m%2s").arg(QString::number(mins), QString::number(secs).rightJustified(2, '0'));
    }
    return QStringLiteral("%1s").arg(QString::number(secs));
}

QString HuggingFaceFileDownloader::locateDownloadedFile(const QString& cacheDir, const QString& repoId, const QString& filename) {
    const QString repoRoot = QDir(cacheDir).filePath(repoCacheDirName(repoId));
    const QString snapshotsRoot = QDir(repoRoot).filePath(QStringLiteral("snapshots"));
    const QString preferredPath = QDir(snapshotsRoot).filePath(QStringLiteral("main/%1").arg(filename));
    if (QFileInfo::exists(preferredPath)) {
        return preferredPath;
    }
    QDir snapshotsDir(snapshotsRoot);
    if (snapshotsDir.exists()) {
        const QFileInfoList snapshotDirs = snapshotsDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time);
        for (const QFileInfo& snapshotInfo : snapshotDirs) {
            const QString candidate = QDir(snapshotInfo.absoluteFilePath()).filePath(filename);
            if (QFileInfo::exists(candidate)) {
                return candidate;
            }
        }
    }
    return {};
}

void HuggingFaceFileDownloader::resetState() {
    m_receivedBytes = 0;
    m_totalBytes = 0;
    m_speedBytesPerSec = 0.0;
    m_etaSeconds = -1;
    m_progressDetails.clear();
    m_phase = QStringLiteral("idle");
    m_backendName.clear();
}

void HuggingFaceFileDownloader::setStatusLine(const QString& message) {
    if (m_statusLine == message) {
        return;
    }
    m_statusLine = message;
    emit statusChanged();
}

void HuggingFaceFileDownloader::setPhase(const QString& value) {
    if (m_phase == value) {
        return;
    }
    m_phase = value;
    emit statusChanged();
}

void HuggingFaceFileDownloader::setProgressDetails() {
    const QString transferred = formatBytes(m_receivedBytes);
    const QString total = m_totalBytes > 0 ? formatBytes(m_totalBytes) : QStringLiteral("unknown");
    m_progressDetails = QStringLiteral("%1 / %2 • %3 • ETA %4")
                            .arg(transferred, total, formatRate(m_speedBytesPerSec), formatEta(m_etaSeconds));
}

void HuggingFaceFileDownloader::appendRuntimeLog(const QString& level, const QString& message) const {
    const QString line = QStringLiteral("[%1] [%2] %3")
                             .arg(QDateTime::currentDateTime().toString(Qt::ISODate), level, message);
    if (level == QStringLiteral("WARN")) {
        qWarning().noquote() << line;
    } else {
        qInfo().noquote() << line;
    }
    QFile file(appLogPath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << line << '\n';
    }
}

void HuggingFaceFileDownloader::appendDiagnostic(const QString& message) {
    if (message.trimmed().isEmpty()) {
        return;
    }
    QStringList rows = m_diagnosticsText.split('\n', Qt::SkipEmptyParts);
    rows.push_back(QStringLiteral("[%1][%2] %3")
                       .arg(QDateTime::currentDateTime().toString(Qt::ISODate), m_backendName, message.trimmed()));
    while (rows.size() > 80) {
        rows.removeFirst();
    }
    m_diagnosticsText = rows.join('\n');
    emit diagnosticsChanged();
}

void HuggingFaceFileDownloader::bindBackendSignals(IFileDownloadBackend* backend) {
    connect(backend, &IFileDownloadBackend::progressChanged, this, [this, backend]() {
        if (backend != m_activeBackend) {
            return;
        }

        if (auto* qtBackend = qobject_cast<QtHttpDownloadBackend*>(backend)) {
            m_receivedBytes = qtBackend->receivedBytes();
            m_totalBytes = qtBackend->totalBytes();
            m_speedBytesPerSec = qtBackend->speedBytesPerSec();
            m_etaSeconds = qtBackend->etaSeconds();
            setPhase(qtBackend->phase());
        } else {
            m_receivedBytes = 0;
            m_totalBytes = 0;
            m_speedBytesPerSec = 0.0;
            m_etaSeconds = -1;
        }
        setProgressDetails();
        emit progressChanged();
    });

    connect(backend, &IFileDownloadBackend::statusChanged, this, [this, backend]() {
        if (backend != m_activeBackend) {
            return;
        }

        QString backendStatus;
        QString backendPhase;
        if (auto* qtBackend = qobject_cast<QtHttpDownloadBackend*>(backend)) {
            backendStatus = qtBackend->statusLine();
            backendPhase = qtBackend->phase();
        } else if (auto* hfBackend = qobject_cast<HfCliDownloadBackend*>(backend)) {
            backendStatus = hfBackend->statusLine();
            backendPhase = hfBackend->phase();
        }

        if (!backendPhase.isEmpty()) {
            setPhase(backendPhase);
        }

        if (!backendStatus.isEmpty()) {
            if (m_totalBytes > 0 || m_receivedBytes > 0 || m_speedBytesPerSec > 0.0) {
                setStatusLine(QStringLiteral("%1 • %2").arg(backendStatus, m_progressDetails));
            } else {
                setStatusLine(backendStatus);
            }
        }
    });

    connect(backend, &IFileDownloadBackend::diagnosticsChanged, this, [this, backend]() {
        if (backend != m_activeBackend) {
            return;
        }
        QString text;
        if (auto* qtBackend = qobject_cast<QtHttpDownloadBackend*>(backend)) {
            text = qtBackend->diagnosticsText();
        } else if (auto* hfBackend = qobject_cast<HfCliDownloadBackend*>(backend)) {
            text = hfBackend->diagnosticsText();
        }
        const QString lastLine = text.section('\n', -1).trimmed();
        if (!lastLine.isEmpty() && lastLine != m_lastBackendDiagnosticLine) {
            m_lastBackendDiagnosticLine = lastLine;
            appendDiagnostic(lastLine);
        }
    });

    connect(backend, &IFileDownloadBackend::failed, this, [this, backend](const QString& message) {
        if (backend != m_activeBackend) {
            return;
        }
        handleBackendFailure(message);
    });

    connect(backend, &IFileDownloadBackend::finishedSuccessfully, this, [this, backend](const QString& localPath) {
        if (backend != m_activeBackend) {
            return;
        }
        handleBackendSuccess(localPath);
    });
}

void HuggingFaceFileDownloader::beginBackendAttempt(std::size_t index) {
    if (index >= m_attemptOrder.size()) {
        handleBackendFailure(QStringLiteral("No downloader backend available."));
        return;
    }

    m_activeAttempt = index;
    m_activeBackend = m_attemptOrder[index];
    m_backendName = m_activeBackend->backendName();
    emit activeChanged();
    appendRuntimeLog(QStringLiteral("INFO"), QStringLiteral("Download backend selected: %1").arg(m_backendName));
    appendDiagnostic(QStringLiteral("backend selected: %1").arg(m_backendName));
    setPhase(QStringLiteral("starting"));

    DownloadRequest request;
    request.repoId = m_repoId;
    request.filename = m_filename;
    request.cacheDir = m_cacheDir;
    request.token = m_token;
    m_activeBackend->start(request);
}

void HuggingFaceFileDownloader::handleBackendFailure(const QString& message) {
    appendRuntimeLog(QStringLiteral("WARN"), QStringLiteral("%1 failed: %2").arg(m_backendName, message));
    appendDiagnostic(QStringLiteral("failure: %1").arg(message));

    if (m_cancelRequested) {
        m_cancelRequested = false;
        setPhase(QStringLiteral("canceled"));
        setStatusLine(QStringLiteral("Download canceled."));
        emit failed(m_statusLine);
        return;
    }

    const std::size_t nextIndex = m_activeAttempt + 1;
    if (nextIndex < m_attemptOrder.size()) {
        appendRuntimeLog(QStringLiteral("INFO"), QStringLiteral("Falling back to next backend."));
        beginBackendAttempt(nextIndex);
        return;
    }

    m_activeBackend = nullptr;
    setPhase(QStringLiteral("failed"));
    setStatusLine(message.isEmpty() ? QStringLiteral("Download failed.") : message);
    emit activeChanged();
    emit failed(m_statusLine);
}

void HuggingFaceFileDownloader::handleBackendSuccess(const QString& localPath) {
    QString resolved = localPath;
    if (resolved.isEmpty() || !QFileInfo::exists(resolved)) {
        resolved = locateDownloadedFile(m_cacheDir, m_repoId, m_filename);
    }
    if (resolved.isEmpty()) {
        handleBackendFailure(QStringLiteral("Download completed but local file was not found."));
        return;
    }

    m_localPath = resolved;
    m_receivedBytes = QFileInfo(m_localPath).size();
    m_totalBytes = m_receivedBytes;
    m_speedBytesPerSec = 0.0;
    m_etaSeconds = 0;
    setProgressDetails();
    setPhase(QStringLiteral("completed"));
    setStatusLine(QStringLiteral("Download completed: %1").arg(m_localPath));
    appendRuntimeLog(QStringLiteral("INFO"), QStringLiteral("Download success backend=%1 path=%2").arg(m_backendName, m_localPath));
    appendDiagnostic(QStringLiteral("success path=%1").arg(m_localPath));
    m_activeBackend = nullptr;
    emit progressChanged();
    emit activeChanged();
    emit finishedSuccessfully(m_localPath);
}

void HuggingFaceFileDownloader::startDownload(const QString& repoId, const QString& filename, const QString& localDir) {
    if (repoId.trimmed().isEmpty() || filename.trimmed().isEmpty() || localDir.trimmed().isEmpty()) {
        setStatusLine(QStringLiteral("Missing repository, file name, or destination folder."));
        emit failed(m_statusLine);
        return;
    }

    if (isActive()) {
        cancel();
    }

    resetState();
    m_cancelRequested = false;
    m_diagnosticsText.clear();
    m_lastBackendDiagnosticLine.clear();
    emit diagnosticsChanged();
    m_repoId = repoId.trimmed();
    m_filename = filename.trimmed();
    m_cacheDir = localDir.trimmed();
    m_localPath = QDir(m_cacheDir).filePath(QStringLiteral("%1/snapshots/main/%2").arg(repoCacheDirName(m_repoId), m_filename));
    QDir().mkpath(QFileInfo(m_localPath).dir().absolutePath());

    m_attemptOrder.clear();
    const bool hfAvailable = m_hfBackend && m_hfBackend->isAvailable();
    if (m_strategy == Strategy::QtOnly) {
        m_attemptOrder.push_back(m_qtBackend.get());
    } else if (m_strategy == Strategy::HfOnly) {
        if (hfAvailable) {
            m_attemptOrder.push_back(m_hfBackend.get());
        }
    } else {
        m_attemptOrder.push_back(m_qtBackend.get());
        if (hfAvailable) {
            m_attemptOrder.push_back(m_hfBackend.get());
        }
    }

    if (m_attemptOrder.empty()) {
        setPhase(QStringLiteral("failed"));
        setStatusLine(QStringLiteral("No available download backend. Install `hf` or use Qt backend."));
        appendRuntimeLog(QStringLiteral("WARN"), m_statusLine);
        emit failed(m_statusLine);
        return;
    }

    appendRuntimeLog(QStringLiteral("INFO"), QStringLiteral("Download start repo=%1 file=%2 strategy=%3 hfAvailable=%4")
                                            .arg(m_repoId,
                                                 m_filename,
                                                 m_strategy == Strategy::QtOnly ? QStringLiteral("qt") : (m_strategy == Strategy::HfOnly ? QStringLiteral("hf") : QStringLiteral("auto")),
                                                 hfAvailable ? QStringLiteral("true") : QStringLiteral("false")));

    beginBackendAttempt(0);
}

void HuggingFaceFileDownloader::cancel() {
    if (!isActive() || !m_activeBackend) {
        return;
    }

    m_cancelRequested = true;
    appendRuntimeLog(QStringLiteral("INFO"), QStringLiteral("Cancel requested backend=%1").arg(m_backendName));
    m_activeBackend->cancel();
}

} // namespace ide::adapters::modelhub
