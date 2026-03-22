#pragma once

#include "adapters/modelhub/IFileDownloadBackend.hpp"

#include <QProcess>
#include <QStringList>

namespace ide::adapters::modelhub {

class HfCliDownloadBackend final : public IFileDownloadBackend {
    Q_OBJECT
public:
    explicit HfCliDownloadBackend(QObject* parent = nullptr);

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
    static QStringList parseLines(const QString& chunk, QString& carry);
    static QString repoCacheDirName(const QString& repoId);
    static QString locateDownloadedFile(const QString& cacheDir, const QString& repoId, const QString& filename);
    void handleChunk(const QString& chunk, bool stderrStream);
    void setPhase(const QString& value);
    void setStatusLine(const QString& value);
    void appendDiagnostic(const QString& line);
    void resetState();

    QProcess m_process;
    QString m_repoId;
    QString m_filename;
    QString m_cacheDir;
    QString m_lastDetectedPath;
    QString m_statusLine;
    QString m_phase;
    QString m_diagnosticsText;
    QString m_stdoutCarry;
    QString m_stderrCarry;
    bool m_available = false;
    bool m_cancelRequested = false;
};

} // namespace ide::adapters::modelhub
