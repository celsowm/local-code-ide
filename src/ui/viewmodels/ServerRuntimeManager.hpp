#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <memory>

namespace ide::services {
class LocalModelRuntimeService;
class LlamaServerProcessService;
}

namespace ide::ui::viewmodels {

class ServerRuntimeManager final : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString statusLine READ statusLine NOTIFY statusChanged)
    Q_PROPERTY(QString logs READ logs NOTIFY logsChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(bool starting READ starting NOTIFY startingChanged)
    Q_PROPERTY(bool healthy READ healthy NOTIFY healthyChanged)
    Q_PROPERTY(QString executablePath READ executablePath WRITE setExecutablePath NOTIFY executablePathChanged)
    Q_PROPERTY(QString extraArguments READ extraArguments WRITE setExtraArguments NOTIFY extraArgumentsChanged)
    Q_PROPERTY(QString baseUrl READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged)

    Q_PROPERTY(bool hasActiveModel READ hasActiveModel NOTIFY activeModelChanged)
    Q_PROPERTY(int contextSize READ contextSize WRITE setContextSize NOTIFY runtimeConfigChanged)

public:
    explicit ServerRuntimeManager(std::unique_ptr<ide::services::LlamaServerProcessService> serverService,
                                   ide::services::LocalModelRuntimeService* runtimeService,
                                   QObject* parent = nullptr);

    QString statusLine() const;
    QString logs() const;
    bool running() const;
    bool starting() const;
    bool healthy() const;

    QString executablePath() const;
    void setExecutablePath(const QString& value);

    QString extraArguments() const;
    void setExtraArguments(const QString& value);

    QString baseUrl() const;
    void setBaseUrl(const QString& value);

    bool hasActiveModel() const;
    int contextSize() const;
    void setContextSize(int value);

    Q_INVOKABLE void startServer(const QString& modelPath);
    Q_INVOKABLE void stopServer();
    Q_INVOKABLE void probeServer();
    Q_INVOKABLE void clearServerLogs();

    QString generateLaunchCommand(const QString& modelPath, const QString& extraArgs, const QString& baseUrl) const;
    QString currentLaunchCommand(const QString& executable, const QString& extraArgs, const QString& baseUrl) const;

signals:
    void statusChanged();
    void logsChanged();
    void runningChanged();
    void startingChanged();
    void healthyChanged();
    void executablePathChanged();
    void extraArgumentsChanged();
    void baseUrlChanged();
    void activeModelChanged();
    void runtimeConfigChanged();

private:
    std::unique_ptr<ide::services::LlamaServerProcessService> m_serverService;
    ide::services::LocalModelRuntimeService* m_runtimeService;
};

} // namespace ide::ui::viewmodels
