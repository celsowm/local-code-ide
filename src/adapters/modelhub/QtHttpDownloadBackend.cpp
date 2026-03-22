#include "adapters/modelhub/QtHttpDownloadBackend.hpp"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QUrlQuery>

namespace ide::adapters::modelhub {

QString QtHttpDownloadBackend::repoCacheDirName(const QString& repoId) {
    QString normalized = repoId.trimmed();
    normalized.replace('/', QStringLiteral("--"));
    return QStringLiteral("models--") + normalized;
}

QString QtHttpDownloadBackend::snapshotTargetPath(const QString& hubCacheDir, const QString& repoId, const QString& filename) {
    const QString repoCacheDir = QDir(hubCacheDir).filePath(repoCacheDirName(repoId));
    return QDir(repoCacheDir).filePath(QStringLiteral("snapshots/main/%1").arg(filename));
}

QtHttpDownloadBackend::QtHttpDownloadBackend(QObject* parent)
    : IFileDownloadBackend(parent) {}

QString QtHttpDownloadBackend::backendName() const { return QStringLiteral("qt-http"); }
bool QtHttpDownloadBackend::isAvailable() const { return true; }
bool QtHttpDownloadBackend::isActive() const { return !m_reply.isNull(); }
qint64 QtHttpDownloadBackend::receivedBytes() const { return m_receivedBytes; }
qint64 QtHttpDownloadBackend::totalBytes() const { return m_totalBytes; }
double QtHttpDownloadBackend::speedBytesPerSec() const { return m_speedBytesPerSec; }
qint64 QtHttpDownloadBackend::etaSeconds() const { return m_etaSeconds; }
QString QtHttpDownloadBackend::phase() const { return m_phase; }
QString QtHttpDownloadBackend::statusLine() const { return m_statusLine; }
QString QtHttpDownloadBackend::diagnosticsText() const { return m_diagnosticsText; }

void QtHttpDownloadBackend::setPhase(const QString& value) {
    if (m_phase == value) {
        return;
    }
    m_phase = value;
    emit statusChanged();
}

void QtHttpDownloadBackend::setStatusLine(const QString& value) {
    if (m_statusLine == value) {
        return;
    }
    m_statusLine = value;
    emit statusChanged();
}

void QtHttpDownloadBackend::appendDiagnostic(const QString& line) {
    if (line.trimmed().isEmpty()) {
        return;
    }
    QStringList rows = m_diagnosticsText.split('\n', Qt::SkipEmptyParts);
    rows.push_back(QStringLiteral("[%1][%2] %3")
                       .arg(QDateTime::currentDateTime().toString(Qt::ISODate), backendName(), line.trimmed()));
    while (rows.size() > 40) {
        rows.removeFirst();
    }
    m_diagnosticsText = rows.join('\n');
    emit diagnosticsChanged();
}

void QtHttpDownloadBackend::resetState() {
    m_statusLine.clear();
    m_phase = QStringLiteral("idle");
    m_receivedBytes = 0;
    m_totalBytes = 0;
    m_resumeOffset = 0;
    m_etaSeconds = -1;
    m_speedBytesPerSec = 0.0;
    m_speedSamples.clear();
}

void QtHttpDownloadBackend::updateSpeedSample() {
    if (!m_elapsed.isValid()) {
        return;
    }
    m_speedSamples.push_back({m_receivedBytes, m_elapsed.elapsed()});
    while (!m_speedSamples.isEmpty() && (m_speedSamples.back().millis - m_speedSamples.front().millis) > 6000) {
        m_speedSamples.pop_front();
    }
    if (m_speedSamples.size() < 2) {
        m_speedBytesPerSec = 0.0;
        return;
    }
    const auto& first = m_speedSamples.front();
    const auto& last = m_speedSamples.back();
    const qint64 deltaBytes = last.receivedBytes - first.receivedBytes;
    const qint64 deltaMs = last.millis - first.millis;
    if (deltaBytes <= 0 || deltaMs <= 0) {
        m_speedBytesPerSec = 0.0;
        return;
    }
    m_speedBytesPerSec = static_cast<double>(deltaBytes) * 1000.0 / static_cast<double>(deltaMs);
}

void QtHttpDownloadBackend::updateEta() {
    if (m_totalBytes <= 0 || m_receivedBytes <= 0 || m_speedBytesPerSec < 1024.0) {
        m_etaSeconds = -1;
        return;
    }
    const qint64 remaining = m_totalBytes - m_receivedBytes;
    if (remaining <= 0) {
        m_etaSeconds = 0;
        return;
    }
    m_etaSeconds = qMax<qint64>(0, static_cast<qint64>(remaining / m_speedBytesPerSec));
}

void QtHttpDownloadBackend::start(const DownloadRequest& request) {
    if (request.repoId.trimmed().isEmpty() || request.filename.trimmed().isEmpty() || request.cacheDir.trimmed().isEmpty()) {
        setStatusLine(QStringLiteral("Missing repository, file name, or cache directory."));
        appendDiagnostic(QStringLiteral("invalid start request"));
        emit failed(m_statusLine);
        return;
    }

    if (isActive()) {
        cancel();
    }

    m_cancelRequested = false;
    resetState();
    m_repoId = request.repoId.trimmed();
    m_filename = request.filename.trimmed();
    m_localPath = snapshotTargetPath(request.cacheDir.trimmed(), m_repoId, m_filename);
    const QFileInfo targetInfo(m_localPath);
    QDir().mkpath(targetInfo.dir().absolutePath());
    m_partialPath = m_localPath + QStringLiteral(".part");
    m_resumeOffset = QFileInfo::exists(m_partialPath) ? QFileInfo(m_partialPath).size() : 0;
    m_receivedBytes = m_resumeOffset;

    m_file.setFileName(m_partialPath);
    if (!m_file.open(m_resumeOffset > 0 ? (QIODevice::WriteOnly | QIODevice::Append) : QIODevice::WriteOnly)) {
        setStatusLine(QStringLiteral("Could not open temporary file for writing."));
        appendDiagnostic(QStringLiteral("file open failed: %1").arg(m_partialPath));
        emit failed(m_statusLine);
        return;
    }

    const QString encodedFilename = QString::fromUtf8(QUrl::toPercentEncoding(m_filename, "/"));
    QUrl url(QStringLiteral("https://huggingface.co/%1/resolve/main/%2").arg(m_repoId, encodedFilename));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("download"), QStringLiteral("true"));
    url.setQuery(query);

    QNetworkRequest networkRequest(url);
    networkRequest.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    if (!request.token.trimmed().isEmpty()) {
        networkRequest.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(request.token).toUtf8());
    }
    if (m_resumeOffset > 0) {
        networkRequest.setRawHeader("Range", QStringLiteral("bytes=%1-").arg(m_resumeOffset).toUtf8());
    }

    m_elapsed.start();
    m_reply = m_network.get(networkRequest);
    emit activeChanged();
    setPhase(QStringLiteral("downloading"));
    setStatusLine(m_resumeOffset > 0
        ? QStringLiteral("Resuming download: %1").arg(m_filename)
        : QStringLiteral("Downloading: %1").arg(m_filename));
    appendDiagnostic(QStringLiteral("start repo=%1 file=%2 offset=%3").arg(m_repoId, m_filename, QString::number(m_resumeOffset)));
    emit progressChanged();

    connect(m_reply, &QNetworkReply::metaDataChanged, this, [this]() {
        if (!m_reply || m_resumeOffset <= 0) {
            return;
        }
        const int statusCode = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode == 200) {
            m_resumeOffset = 0;
            if (m_file.isOpen()) {
                m_file.resize(0);
                m_file.seek(0);
            }
            appendDiagnostic(QStringLiteral("server ignored range request, restarting from 0"));
        }
    });

    connect(m_reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        m_receivedBytes = m_resumeOffset + qMax<qint64>(0, received);
        m_totalBytes = total > 0 ? (m_resumeOffset + total) : 0;
        updateSpeedSample();
        updateEta();
        emit progressChanged();
    });

    connect(m_reply, &QIODevice::readyRead, this, [this]() {
        if (!m_reply) {
            return;
        }
        m_file.write(m_reply->readAll());
    });

    connect(m_reply, &QNetworkReply::finished, this, [this]() {
        if (!m_reply) {
            return;
        }

        m_file.write(m_reply->readAll());
        m_file.flush();
        m_file.close();

        const auto error = m_reply->error();
        const QString errorText = m_reply->errorString();
        m_reply->deleteLater();
        m_reply = nullptr;
        emit activeChanged();

        if (error != QNetworkReply::NoError) {
            setPhase(QStringLiteral("failed"));
            if (m_cancelRequested && error == QNetworkReply::OperationCanceledError) {
                setStatusLine(QStringLiteral("Download canceled. Partial file kept for resume."));
                appendDiagnostic(QStringLiteral("canceled by user"));
                emit failed(m_statusLine);
                m_cancelRequested = false;
                return;
            }
            setStatusLine(QStringLiteral("Download failed: %1").arg(errorText));
            appendDiagnostic(QStringLiteral("network error: %1").arg(errorText));
            emit failed(m_statusLine);
            return;
        }

        finalizeSuccess();
    });
}

void QtHttpDownloadBackend::cancel() {
    if (!isActive()) {
        return;
    }
    m_cancelRequested = true;
    m_reply->abort();
    if (m_file.isOpen()) {
        m_file.flush();
        m_file.close();
    }
    setPhase(QStringLiteral("canceling"));
    setStatusLine(QStringLiteral("Canceling download..."));
    appendDiagnostic(QStringLiteral("cancel requested"));
    emit progressChanged();
}

void QtHttpDownloadBackend::finalizeSuccess() {
    QFile::remove(m_localPath);
    if (!QFile::rename(m_partialPath, m_localPath)) {
        setPhase(QStringLiteral("failed"));
        setStatusLine(QStringLiteral("Download finished, but could not move the final file."));
        appendDiagnostic(QStringLiteral("rename failed from %1 to %2").arg(m_partialPath, m_localPath));
        emit failed(m_statusLine);
        return;
    }

    m_receivedBytes = QFileInfo(m_localPath).size();
    m_totalBytes = m_receivedBytes;
    m_speedBytesPerSec = 0.0;
    m_etaSeconds = 0;
    setPhase(QStringLiteral("completed"));
    setStatusLine(QStringLiteral("Download completed: %1").arg(m_localPath));
    appendDiagnostic(QStringLiteral("completed path=%1").arg(m_localPath));
    emit progressChanged();
    emit finishedSuccessfully(m_localPath);
}

} // namespace ide::adapters::modelhub
