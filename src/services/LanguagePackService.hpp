#pragma once

#include <QObject>
#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QPointer>
#include <QProcess>
#include <QString>
#include <QStringList>

namespace ide::services {

class LanguagePackService final : public QObject {
    Q_OBJECT
public:
    struct ServerSpec {
        bool ready = false;
        QString command;
        QStringList args;
        QString source;
        QString statusLine;
    };

    explicit LanguagePackService(bool allowExternalInstall = false,
                                 bool allowPathFallback = false,
                                 QObject* parent = nullptr);

    QString languageForFilePath(const QString& filePath) const;
    ServerSpec ensureServerReady(const QString& languageId, bool autoInstall = false);
    QString statusLineForLanguage(const QString& languageId) const;
    QString activeServerLabel(const QString& languageId) const;

signals:
    void languageStatusChanged(const QString& languageId, const QString& statusLine);
    void languageToastRequested(const QString& message);

private:
    struct PackDefinition {
        QString id;
        QString version;
        QStringList languages;
        QString bundledWindowsRelativePath;
        QString bundledLinuxRelativePath;
        QStringList defaultArgs;
        QStringList pathCandidates;
        QString installCommandWindows;
        QString installCommandLinux;
    };

    struct InstallState {
        bool installing = false;
        bool attempted = false;
        QString status;
        QPointer<QProcess> process;
    };

    bool loadManifest();
    QString manifestPath() const;
    QString appBundledPathFor(const PackDefinition& pack) const;
    QString developerOverrideCommand(const QString& languageId) const;
    QStringList developerOverrideArgs(const QString& languageId) const;
    QString findCandidateCommand(const PackDefinition& pack) const;
    PackDefinition findPackForLanguage(const QString& languageId) const;
    void startInstall(const QString& languageId, const PackDefinition& pack);
    QString normalizedLanguageId(const QString& languageId) const;

    bool m_manifestLoaded = false;
    bool m_allowExternalInstall = false;
    bool m_allowPathFallback = false;
    QList<PackDefinition> m_packs;
    QHash<QString, InstallState> m_installStates;
    QHash<QString, QString> m_statusByLanguage;
};

} // namespace ide::services
