#pragma once

#include "adapters/modelhub/IFileDownloadBackend.hpp"

#include <QElapsedTimer>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QQueue>

namespace ide::adapters::modelhub {

class QtHttpDownloadBackend final : public IFileDownloadBackend {
    Q_OBJECT
public:
    explicit QtHttpDownloadBackend(QObject* parent = nullptr);

    QString backendName() const override;
    bool isAvailable() const override;
    bool isActive() const override;
    void start(const DownloadRequest& request) override;
    void cancel() override;

    qint64 receivedBytes() const;
    qint64 totalBytes() const;
    double speedBytesPerSec() const;
    qint64 etaSeconds() const;
    QString phase() const;
    QString statusLine() const;
    QString diagnosticsText() const;

private:
    struct SpeedSample {
        qint64 receivedBytes = 0;
        qint64 millis = 0;
    };

    static QString repoCacheDirName(const QString& repoId);
    static QString snapshotTargetPath(const QString& hubCacheDir, const QString& repoId, const QString& filename);
    void setPhase(const QString& value);
    void setStatusLine(const QString& value);
    void appendDiagnostic(const QString& line);
    void resetState();
    void updateSpeedSample();
    void updateEta();
    void finalizeSuccess();

    QString m_repoId;
    QString m_filename;
    QString m_localPath;
    QString m_partialPath;
    QString m_statusLine;
    QString m_phase;
    QString m_diagnosticsText;
    qint64 m_receivedBytes = 0;
    qint64 m_totalBytes = 0;
    qint64 m_resumeOffset = 0;
    qint64 m_etaSeconds = -1;
    double m_speedBytesPerSec = 0.0;
    bool m_cancelRequested = false;
    QQueue<SpeedSample> m_speedSamples;
    QElapsedTimer m_elapsed;
    QFile m_file;
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
};

} // namespace ide::adapters::modelhub
