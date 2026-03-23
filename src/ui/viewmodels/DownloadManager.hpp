#pragma once

#include <QObject>
#include <QString>
#include <memory>

namespace ide::adapters::modelhub {
class HuggingFaceFileDownloader;
}

namespace ide::ui::viewmodels {

class DownloadManager final : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString speedText READ speedText NOTIFY progressChanged)
    Q_PROPERTY(QString etaText READ etaText NOTIFY progressChanged)
    Q_PROPERTY(QString phase READ phase NOTIFY progressChanged)
    Q_PROPERTY(QString diagnostics READ diagnostics NOTIFY diagnosticsChanged)
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)
    Q_PROPERTY(QString downloadedPath READ downloadedPath NOTIFY downloadedPathChanged)

public:
    explicit DownloadManager(std::unique_ptr<ide::adapters::modelhub::HuggingFaceFileDownloader> downloader, QObject* parent = nullptr);

    QString status() const;
    double progress() const;
    QString speedText() const;
    QString etaText() const;
    QString phase() const;
    QString diagnostics() const;
    bool active() const;
    QString downloadedPath() const;

    QString progressDetails() const;

    Q_INVOKABLE void startDownload(const QString& repoId, const QString& filePath, const QString& targetDir);
    Q_INVOKABLE void cancelDownload();

signals:
    void statusChanged();
    void progressChanged();
    void activeChanged();
    void diagnosticsChanged();
    void downloadedPathChanged();

private:
    std::unique_ptr<ide::adapters::modelhub::HuggingFaceFileDownloader> m_downloader;
    QString m_downloadedPath;
};

} // namespace ide::ui::viewmodels
