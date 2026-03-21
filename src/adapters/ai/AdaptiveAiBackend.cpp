#include "adapters/ai/AdaptiveAiBackend.hpp"
#include "adapters/ai/LlamaCppServerBackend.hpp"

#include <QFileInfo>
#include <QStringList>

namespace ide::adapters::ai {

AdaptiveAiBackend::AdaptiveAiBackend(ide::services::LocalModelRuntimeService* runtimeService,
                                     ide::services::LlamaServerProcessService* serverService,
                                     int timeoutMs)
    : m_runtimeService(runtimeService)
    , m_serverService(serverService)
    , m_timeoutMs(timeoutMs) {}

ide::services::interfaces::AiResponse AdaptiveAiBackend::complete(const ide::services::interfaces::AiRequest& request) {
    if (m_serverService && m_serverService->isHealthy()) {
        const QString modelName = m_runtimeService && m_runtimeService->hasActiveModel()
            ? QFileInfo(m_runtimeService->activeLocalPath()).fileName()
            : QString();
        LlamaCppServerBackend backend(m_serverService->baseUrl(), modelName, m_timeoutMs);
        return backend.complete(request);
    }

    auto fallback = m_mockBackend.complete(request);
    QStringList hints;
    hints << QStringLiteral("Assistente ainda não conectado ao runtime local.");
    if (m_runtimeService && m_runtimeService->hasActiveModel()) {
        hints << QStringLiteral("GGUF ativo: %1").arg(m_runtimeService->activeDisplayName());
    } else {
        hints << QStringLiteral("Nenhum GGUF ativo selecionado.");
    }
    if (m_serverService) {
        hints << QStringLiteral("Server: %1").arg(m_serverService->statusLine());
    }
    fallback.content = hints.join(QStringLiteral("\n")) + QStringLiteral("\n\n") + fallback.content;
    return fallback;
}

QString AdaptiveAiBackend::name() const {
    return QStringLiteral("Adaptive Local Runtime");
}

QString AdaptiveAiBackend::statusLine() const {
    const QString model = m_runtimeService && m_runtimeService->hasActiveModel()
        ? m_runtimeService->activeDisplayName()
        : QStringLiteral("no-active-gguf");
    if (m_serverService && m_serverService->isHealthy()) {
        return QStringLiteral("online · %1 · %2").arg(m_serverService->baseUrl(), model);
    }
    if (m_serverService && (m_serverService->isRunning() || m_serverService->isStarting())) {
        return QStringLiteral("starting · %1 · %2").arg(m_serverService->baseUrl(), model);
    }
    return QStringLiteral("offline · start local server to bind chat · %1").arg(model);
}

bool AdaptiveAiBackend::isAvailable() const {
    return m_runtimeService || m_serverService;
}

} // namespace ide::adapters::ai
