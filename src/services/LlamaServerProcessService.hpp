#pragma once

#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>

namespace ide::services {

class LlamaServerProcessService final : public QObject {
    Q_OBJECT
public:
    explicit LlamaServerProcessService(QString baseUrl,
                                       QString executablePath = QStringLiteral("llama-server"),
                                       QString extraArguments = {},
                                       QObject* parent = nullptr);

    QString executablePath() const;
    void setExecutablePath(const QString& value);

    QString extraArguments() const;
    void setExtraArguments(const QString& value);

    QString baseUrl() const;
    void setBaseUrl(const QString& value);

    QString statusLine() const;
    QString logs() const;
    bool isRunning() const;
    bool isStarting() const;
    bool isHealthy() const;

    Q_INVOKABLE void startWithModel(const QString& modelPath, int contextSize);
    Q_INVOKABLE void stop();
    Q_INVOKABLE void probeNow();
    Q_INVOKABLE void clearLogs();

signals:
    void executablePathChanged();
    void extraArgumentsChanged();
    void baseUrlChanged();
    void statusChanged();
    void logsChanged();
    void runningChanged();
    void startingChanged();
    void healthyChanged();

private:
    QString normalizeBaseUrl(QString value) const;
    void appendLog(const QString& chunk);
    void setStatusLine(const QString& value);
    void setHealthy(bool value);
    void beginProbe(const QUrl& url, bool allowFallback);
    void handleProbeFinished(QNetworkReply* reply, bool allowFallback);
    QStringList launchArguments(const QString& modelPath, int contextSize) const;

    QProcess m_process;
    QTimer m_probeTimer;
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_probeReply;
    QString m_executablePath;
    QString m_extraArguments;
    QString m_baseUrl;
    QString m_statusLine;
    QString m_logs;
    bool m_running = false;
    bool m_starting = false;
    bool m_healthy = false;
};

} // namespace ide::services
