
#pragma once

#include "adapters/modelhub/IFileDownloadBackend.hpp"

#include <QDateTime>
#include <QFile>
#include <QString>
#include <QObject>
#include <memory>
#include <vector>

namespace ide::adapters::modelhub {

class HuggingFaceFileDownloader final : public QObject {
    Q_OBJECT
public:
    explicit HuggingFaceFileDownloader(QString token = {}, QObject* parent = nullptr);

    bool isActive() const;
    QString localPath() const;
    QString statusLine() const;
    qint64 receivedBytes() const;
    qint64 totalBytes() const;
    double speedBytesPerSec() const;
    qint64 etaSeconds() const;
    QString phase() const;
    QString progressDetails() const;
    QString diagnosticsText() const;
    QString backendName() const;
    bool hfCliAvailable() const;

    Q_INVOKABLE void startDownload(const QString& repoId, const QString& filename, const QString& localDir);
    Q_INVOKABLE void cancel();

signals:
    void activeChanged();
    void progressChanged();
    void statusChanged();
    void diagnosticsChanged();
    void finishedSuccessfully(const QString& localPath);
    void failed(const QString& message);

private:
    enum class Strategy {
        Auto,
        QtOnly,
        HfOnly
    };

    static Strategy parseStrategy();
    static QString appLogPath();
    void resetState();
    void setStatusLine(const QString& message);
    void setPhase(const QString& value);
    void setProgressDetails();
    void appendDiagnostic(const QString& message);
    void appendRuntimeLog(const QString& level, const QString& message) const;
    void beginBackendAttempt(std::size_t index);
    void handleBackendFailure(const QString& message);
    void handleBackendSuccess(const QString& localPath);
    void bindBackendSignals(IFileDownloadBackend* backend);
    static QString formatBytes(qint64 bytes);
    static QString formatRate(double bytesPerSec);
    static QString formatEta(qint64 seconds);
    static QString locateDownloadedFile(const QString& cacheDir, const QString& repoId, const QString& filename);

    QString m_token;
    QString m_repoId;
    QString m_filename;
    QString m_cacheDir;
    QString m_localPath;
    QString m_statusLine;
    QString m_phase = QStringLiteral("idle");
    QString m_progressDetails;
    QString m_diagnosticsText;
    QString m_backendName;
    QString m_lastBackendDiagnosticLine;
    qint64 m_receivedBytes = 0;
    qint64 m_totalBytes = 0;
    qint64 m_etaSeconds = -1;
    double m_speedBytesPerSec = 0.0;
    bool m_cancelRequested = false;
    Strategy m_strategy = Strategy::Auto;
    std::vector<IFileDownloadBackend*> m_attemptOrder;
    std::size_t m_activeAttempt = 0;
    std::unique_ptr<IFileDownloadBackend> m_qtBackend;
    std::unique_ptr<IFileDownloadBackend> m_hfBackend;
    IFileDownloadBackend* m_activeBackend = nullptr;
};

} // namespace ide::adapters::modelhub
