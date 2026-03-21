#pragma once

#include <QObject>
#include <QSettings>
#include <QString>

namespace ide::services {

class LocalModelRuntimeService final : public QObject {
    Q_OBJECT
public:
    explicit LocalModelRuntimeService(QString llamaBaseUrl, int contextSize = 8192, QObject* parent = nullptr);

    bool hasActiveModel() const;
    QString activeRepoId() const;
    QString activeRemotePath() const;
    QString activeLocalPath() const;
    QString activeDisplayName() const;
    QString statusLine() const;
    QString baseUrl() const;
    void setBaseUrl(const QString& value);
    int contextSize() const;
    void setContextSize(int value);
    QString launchCommand(const QString& executable = QStringLiteral("llama-server"),
                          const QString& extraArguments = {},
                          const QString& overrideBaseUrl = {}) const;

    void setActiveModel(const QString& repoId, const QString& remotePath, const QString& localPath);
    void clearActiveModel();

signals:
    void activeModelChanged();
    void runtimeConfigChanged();

private:
    void load();
    void save();
    QString normalizedBaseUrl(QString url) const;

    QSettings m_settings;
    QString m_llamaBaseUrl;
    int m_contextSize = 8192;
    QString m_activeRepoId;
    QString m_activeRemotePath;
    QString m_activeLocalPath;
};

} // namespace ide::services
