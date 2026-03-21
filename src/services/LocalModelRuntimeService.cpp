#include "services/LocalModelRuntimeService.hpp"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QUrl>
#include <QStringList>
#include <QtGlobal>
#include <utility>

namespace ide::services {

namespace {
QString settingsFilePath() {
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (base.isEmpty()) {
        base = QDir::homePath() + QStringLiteral("/.localcodeide");
    }
    QDir().mkpath(base);
    return QDir(base).filePath(QStringLiteral("runtime.ini"));
}
}

LocalModelRuntimeService::LocalModelRuntimeService(QString llamaBaseUrl, int contextSize, QObject* parent)
    : QObject(parent)
    , m_settings(settingsFilePath(), QSettings::IniFormat)
    , m_llamaBaseUrl(normalizedBaseUrl(std::move(llamaBaseUrl)))
    , m_contextSize(contextSize > 0 ? contextSize : 8192) {
    load();
}

bool LocalModelRuntimeService::hasActiveModel() const {
    return !m_activeLocalPath.trimmed().isEmpty();
}

QString LocalModelRuntimeService::activeRepoId() const {
    return m_activeRepoId;
}

QString LocalModelRuntimeService::activeRemotePath() const {
    return m_activeRemotePath;
}

QString LocalModelRuntimeService::activeLocalPath() const {
    return m_activeLocalPath;
}

QString LocalModelRuntimeService::activeDisplayName() const {
    if (m_activeLocalPath.isEmpty()) {
        return QStringLiteral("No active local GGUF");
    }
    return QFileInfo(m_activeLocalPath).fileName();
}

QString LocalModelRuntimeService::statusLine() const {
    QStringList parts;
    if (!hasActiveModel()) {
        parts << QStringLiteral("Nenhum GGUF ativo selecionado.");
    } else {
        QString text = QStringLiteral("Ativo: %1").arg(activeDisplayName());
        if (!m_activeRepoId.isEmpty()) {
            text += QStringLiteral(" · %1").arg(m_activeRepoId);
        }
        text += QStringLiteral(" · %1").arg(m_activeLocalPath);
        parts << text;
    }
    parts << QStringLiteral("ctx=%1").arg(QString::number(m_contextSize));
    parts << normalizedBaseUrl(m_llamaBaseUrl);
    return parts.join(QStringLiteral(" · "));
}

QString LocalModelRuntimeService::baseUrl() const {
    return m_llamaBaseUrl;
}

void LocalModelRuntimeService::setBaseUrl(const QString& value) {
    const QString normalized = normalizedBaseUrl(value);
    if (m_llamaBaseUrl == normalized) {
        return;
    }
    m_llamaBaseUrl = normalized;
    save();
    emit runtimeConfigChanged();
}

int LocalModelRuntimeService::contextSize() const {
    return m_contextSize;
}

void LocalModelRuntimeService::setContextSize(int value) {
    const int normalized = qBound(1024, value, 131072);
    if (m_contextSize == normalized) {
        return;
    }
    m_contextSize = normalized;
    save();
    emit runtimeConfigChanged();
}

QString LocalModelRuntimeService::launchCommand(const QString& executable, const QString& extraArguments, const QString& overrideBaseUrl) const {
    const QUrl url(normalizedBaseUrl(overrideBaseUrl.trimmed().isEmpty() ? m_llamaBaseUrl : overrideBaseUrl.trimmed()));
    const QString host = url.host().isEmpty() ? QStringLiteral("127.0.0.1") : url.host();
    const int port = url.port(8080);
    const QString modelPath = hasActiveModel() ? m_activeLocalPath : QStringLiteral("/caminho/model.gguf");

    QString command = QStringLiteral("%1 -m "%2" -c %3 --host %4 --port %5")
        .arg(executable.trimmed().isEmpty() ? QStringLiteral("llama-server") : executable.trimmed(),
             modelPath,
             QString::number(m_contextSize),
             host,
             QString::number(port));
    if (!extraArguments.trimmed().isEmpty()) {
        command += QStringLiteral(" ") + extraArguments.trimmed();
    }
    return command;
}

void LocalModelRuntimeService::setActiveModel(const QString& repoId, const QString& remotePath, const QString& localPath) {
    const QString normalizedLocal = QFileInfo(localPath).absoluteFilePath();
    if (normalizedLocal.isEmpty()) {
        return;
    }

    m_activeRepoId = repoId.trimmed();
    m_activeRemotePath = remotePath.trimmed();
    m_activeLocalPath = normalizedLocal;
    save();
    emit activeModelChanged();
}

void LocalModelRuntimeService::clearActiveModel() {
    if (!hasActiveModel()) {
        return;
    }
    m_activeRepoId.clear();
    m_activeRemotePath.clear();
    m_activeLocalPath.clear();
    save();
    emit activeModelChanged();
}

void LocalModelRuntimeService::load() {
    const QString persistedBaseUrl = m_settings.value(QStringLiteral("runtime/baseUrl")).toString();
    if (!persistedBaseUrl.trimmed().isEmpty()) {
        m_llamaBaseUrl = normalizedBaseUrl(persistedBaseUrl);
    }
    const int persistedContext = m_settings.value(QStringLiteral("runtime/contextSize"), m_contextSize).toInt();
    m_contextSize = qBound(1024, persistedContext, 131072);
    m_activeRepoId = m_settings.value(QStringLiteral("runtime/activeRepoId")).toString();
    m_activeRemotePath = m_settings.value(QStringLiteral("runtime/activeRemotePath")).toString();
    m_activeLocalPath = m_settings.value(QStringLiteral("runtime/activeLocalPath")).toString();
}

void LocalModelRuntimeService::save() {
    m_settings.setValue(QStringLiteral("runtime/baseUrl"), m_llamaBaseUrl);
    m_settings.setValue(QStringLiteral("runtime/contextSize"), m_contextSize);
    m_settings.setValue(QStringLiteral("runtime/activeRepoId"), m_activeRepoId);
    m_settings.setValue(QStringLiteral("runtime/activeRemotePath"), m_activeRemotePath);
    m_settings.setValue(QStringLiteral("runtime/activeLocalPath"), m_activeLocalPath);
    m_settings.sync();
}

QString LocalModelRuntimeService::normalizedBaseUrl(QString url) const {
    url = url.trimmed();
    if (url.endsWith('/')) {
        url.chop(1);
    }
    return url.isEmpty() ? QStringLiteral("http://127.0.0.1:8080") : url;
}

} // namespace ide::services
