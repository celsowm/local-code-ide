#include "ui/viewmodels/ServerRuntimeManager.hpp"

#include "services/LocalModelRuntimeService.hpp"
#include "services/LlamaServerProcessService.hpp"

#include <QFileInfo>
#include <QUrl>

namespace ide::ui::viewmodels {

ServerRuntimeManager::ServerRuntimeManager(std::unique_ptr<ide::services::LlamaServerProcessService> serverService,
                                            ide::services::LocalModelRuntimeService* runtimeService,
                                            QObject* parent)
    : QObject(parent)
    , m_serverService(std::move(serverService))
    , m_runtimeService(runtimeService) {
    
    if (m_runtimeService) {
        connect(m_runtimeService, &ide::services::LocalModelRuntimeService::activeModelChanged, this, [this]() {
            emit activeModelChanged();
            emit runtimeConfigChanged();
        });
        connect(m_runtimeService, &ide::services::LocalModelRuntimeService::runtimeConfigChanged, this, [this]() {
            emit runtimeConfigChanged();
        });
    }

    if (m_serverService) {
        connect(m_serverService.get(), &ide::services::LlamaServerProcessService::statusChanged, this, [this]() {
            emit statusChanged();
        });
        connect(m_serverService.get(), &ide::services::LlamaServerProcessService::logsChanged, this, [this]() {
            emit logsChanged();
        });
        connect(m_serverService.get(), &ide::services::LlamaServerProcessService::runningChanged, this, [this]() {
            emit runningChanged();
        });
        connect(m_serverService.get(), &ide::services::LlamaServerProcessService::startingChanged, this, [this]() {
            emit startingChanged();
        });
        connect(m_serverService.get(), &ide::services::LlamaServerProcessService::healthyChanged, this, [this]() {
            emit healthyChanged();
        });
        connect(m_serverService.get(), &ide::services::LlamaServerProcessService::executablePathChanged, this, [this]() {
            emit executablePathChanged();
        });
        connect(m_serverService.get(), &ide::services::LlamaServerProcessService::extraArgumentsChanged, this, [this]() {
            emit extraArgumentsChanged();
        });
        connect(m_serverService.get(), &ide::services::LlamaServerProcessService::baseUrlChanged, this, [this]() {
            emit baseUrlChanged();
        });
    }
}

QString ServerRuntimeManager::statusLine() const {
    return m_serverService ? m_serverService->statusLine() : QStringLiteral("Server unavailable.");
}

QString ServerRuntimeManager::logs() const {
    return m_serverService ? m_serverService->logs() : QString();
}

bool ServerRuntimeManager::running() const {
    return m_serverService && m_serverService->isRunning();
}

bool ServerRuntimeManager::starting() const {
    return m_serverService && m_serverService->isStarting();
}

bool ServerRuntimeManager::healthy() const {
    return m_serverService && m_serverService->isHealthy();
}

QString ServerRuntimeManager::executablePath() const {
    return m_serverService ? m_serverService->executablePath() : QStringLiteral("llama-server");
}

void ServerRuntimeManager::setExecutablePath(const QString& value) {
    if (m_serverService) {
        m_serverService->setExecutablePath(value);
    }
}

QString ServerRuntimeManager::extraArguments() const {
    return m_serverService ? m_serverService->extraArguments() : QString();
}

void ServerRuntimeManager::setExtraArguments(const QString& value) {
    if (m_serverService) {
        m_serverService->setExtraArguments(value);
    }
}

QString ServerRuntimeManager::baseUrl() const {
    return m_serverService ? m_serverService->baseUrl() : QStringLiteral("http://127.0.0.1:8080");
}

void ServerRuntimeManager::setBaseUrl(const QString& value) {
    if (m_serverService) {
        m_serverService->setBaseUrl(value);
    }
}

bool ServerRuntimeManager::hasActiveModel() const {
    return m_runtimeService && m_runtimeService->hasActiveModel();
}

int ServerRuntimeManager::contextSize() const {
    return m_runtimeService ? m_runtimeService->contextSize() : 8192;
}

void ServerRuntimeManager::setContextSize(int value) {
    if (m_runtimeService) {
        m_runtimeService->setContextSize(value);
    }
}

void ServerRuntimeManager::startServer(const QString& modelPath) {
    if (m_serverService && m_runtimeService) {
        const QFileInfo info(modelPath);
        m_runtimeService->setActiveModel(info.completeBaseName(), info.fileName(), modelPath);
        m_serverService->startWithModel(modelPath, m_runtimeService->contextSize());
    }
}

void ServerRuntimeManager::stopServer() {
    if (m_serverService) {
        m_serverService->stop();
    }
}

void ServerRuntimeManager::probeServer() {
    if (m_serverService) {
        m_serverService->probeNow();
    }
}

void ServerRuntimeManager::clearServerLogs() {
    if (m_serverService) {
        m_serverService->clearLogs();
    }
}

QString ServerRuntimeManager::generateLaunchCommand(const QString& modelPath, const QString& extraArgs, const QString& baseUrlStr) const {
    const QString executable = executablePath();
    const QUrl baseUrl(baseUrlStr);
    const QString host = baseUrl.host().isEmpty() ? QStringLiteral("127.0.0.1") : baseUrl.host();
    const int port = baseUrl.port(8080);
    const QFileInfo info(modelPath);
    return QStringLiteral("%1 -m \"%2\" -c %3 --host %4 --port %5%6")
        .arg(executable,
             info.absoluteFilePath(),
             QString::number(contextSize()),
             host,
             QString::number(port),
             extraArgs.trimmed().isEmpty() ? QString() : QStringLiteral(" ") + extraArgs.trimmed());
}

QString ServerRuntimeManager::currentLaunchCommand(const QString& executable, const QString& extraArgs, const QString& baseUrlStr) const {
    if (m_runtimeService && m_runtimeService->hasActiveModel()) {
        return m_runtimeService->launchCommand(executable, extraArgs, baseUrlStr);
    }
    const QUrl baseUrl(baseUrlStr);
    const QString host = baseUrl.host().isEmpty() ? QStringLiteral("127.0.0.1") : baseUrl.host();
    const int port = baseUrl.port(8080);
    return QStringLiteral("%1 -m /path/model.gguf -c %2 --host %3 --port %4%5")
        .arg(executable,
             QString::number(contextSize()),
             host,
             QString::number(port),
             extraArgs.trimmed().isEmpty() ? QString() : QStringLiteral(" ") + extraArgs.trimmed());
}

} // namespace ide::ui::viewmodels
