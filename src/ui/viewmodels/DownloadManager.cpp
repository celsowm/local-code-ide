#include "ui/viewmodels/DownloadManager.hpp"

#include "adapters/modelhub/HuggingFaceFileDownloader.hpp"

namespace ide::ui::viewmodels {

DownloadManager::DownloadManager(std::unique_ptr<ide::adapters::modelhub::HuggingFaceFileDownloader> downloader, QObject* parent)
    : QObject(parent)
    , m_downloader(std::move(downloader)) {
    
    connect(m_downloader.get(), &ide::adapters::modelhub::HuggingFaceFileDownloader::statusChanged, this, [this]() {
        emit statusChanged();
    });
    connect(m_downloader.get(), &ide::adapters::modelhub::HuggingFaceFileDownloader::progressChanged, this, [this]() {
        emit progressChanged();
    });
    connect(m_downloader.get(), &ide::adapters::modelhub::HuggingFaceFileDownloader::activeChanged, this, [this]() {
        emit activeChanged();
    });
    connect(m_downloader.get(), &ide::adapters::modelhub::HuggingFaceFileDownloader::diagnosticsChanged, this, [this]() {
        emit diagnosticsChanged();
    });
    connect(m_downloader.get(), &ide::adapters::modelhub::HuggingFaceFileDownloader::finishedSuccessfully, this, [this](const QString& localPath) {
        m_downloadedPath = localPath;
        emit downloadedPathChanged();
    });
    connect(m_downloader.get(), &ide::adapters::modelhub::HuggingFaceFileDownloader::failed, this, [this](const QString&) {
        emit progressChanged();
    });
}

QString DownloadManager::status() const {
    return m_downloader->statusLine();
}

double DownloadManager::progress() const {
    const auto total = m_downloader->totalBytes();
    if (total <= 0) {
        return 0.0;
    }
    return static_cast<double>(m_downloader->receivedBytes()) / static_cast<double>(total);
}

QString DownloadManager::speedText() const {
    const double bytesPerSec = m_downloader->speedBytesPerSec();
    if (bytesPerSec <= 0.0) {
        return QStringLiteral("—");
    }
    const double kb = bytesPerSec / 1024.0;
    if (kb < 1024.0) {
        return QStringLiteral("%1 KB/s").arg(QString::number(kb, 'f', kb >= 100.0 ? 0 : 1));
    }
    const double mb = kb / 1024.0;
    return QStringLiteral("%1 MB/s").arg(QString::number(mb, 'f', mb >= 100.0 ? 0 : 1));
}

QString DownloadManager::etaText() const {
    const qint64 seconds = m_downloader->etaSeconds();
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

QString DownloadManager::phase() const {
    return m_downloader->phase();
}

QString DownloadManager::diagnostics() const {
    return m_downloader->diagnosticsText();
}

bool DownloadManager::active() const {
    return m_downloader->isActive();
}

QString DownloadManager::downloadedPath() const {
    return m_downloadedPath;
}

QString DownloadManager::progressDetails() const {
    return m_downloader->progressDetails();
}

void DownloadManager::startDownload(const QString& repoId, const QString& filePath, const QString& targetDir) {
    m_downloader->startDownload(repoId, filePath, targetDir);
}

void DownloadManager::cancelDownload() {
    m_downloader->cancel();
}

} // namespace ide::ui::viewmodels
