#include "adapters/ai/AdaptiveAiBackend.hpp"
#include "adapters/ai/LlamaCppServerBackend.hpp"

#include <QFileInfo>
#include <QMetaObject>
#include <QStringList>
#include <QThread>
#include <utility>

namespace ide::adapters::ai {
namespace {
template <typename T, typename Fn>
T invokeOnObjectThread(QObject* object, T fallback, Fn&& fn) {
    if (!object) {
        return fallback;
    }
    if (object->thread() == QThread::currentThread()) {
        return fn();
    }
    T value = fallback;
    const bool ok = QMetaObject::invokeMethod(
        object,
        [&value, fn = std::forward<Fn>(fn)]() mutable {
            value = fn();
        },
        Qt::BlockingQueuedConnection
    );
    return ok ? value : fallback;
}
} // namespace

AdaptiveAiBackend::AdaptiveAiBackend(ide::services::LocalModelRuntimeService* runtimeService,
                                     ide::services::LlamaServerProcessService* serverService,
                                     int timeoutMs)
    : m_runtimeService(runtimeService)
    , m_serverService(serverService)
    , m_timeoutMs(timeoutMs) {}

ide::services::interfaces::AiResponse AdaptiveAiBackend::complete(const ide::services::interfaces::AiRequest& request) {
    auto* server = m_serverService.data();
    auto* runtime = m_runtimeService.data();
    const bool serverHealthy = invokeOnObjectThread<bool>(server, false, [server]() {
        return server && server->isHealthy();
    });

    if (serverHealthy) {
        const bool hasActiveModel = invokeOnObjectThread<bool>(runtime, false, [runtime]() {
            return runtime && runtime->hasActiveModel();
        });
        const QString modelName = hasActiveModel
            ? QFileInfo(invokeOnObjectThread<QString>(runtime, QString{}, [runtime]() {
                  return runtime ? runtime->activeLocalPath() : QString{};
              })).fileName()
            : QString();
        const QString baseUrl = invokeOnObjectThread<QString>(server, QStringLiteral("http://127.0.0.1:8080"), [server]() {
            return server ? server->baseUrl() : QStringLiteral("http://127.0.0.1:8080");
        });
        LlamaCppServerBackend backend(baseUrl, modelName, m_timeoutMs);
        return backend.complete(request);
    }

    auto fallback = m_mockBackend.complete(request);
    QStringList hints;
    hints << QStringLiteral("Assistente ainda não conectado ao runtime local.");
    const bool hasModel = invokeOnObjectThread<bool>(runtime, false, [runtime]() {
        return runtime && runtime->hasActiveModel();
    });
    if (hasModel) {
        hints << QStringLiteral("GGUF ativo: %1").arg(invokeOnObjectThread<QString>(runtime, QString{}, [runtime]() {
            return runtime ? runtime->activeDisplayName() : QString{};
        }));
    } else {
        hints << QStringLiteral("Nenhum GGUF ativo selecionado.");
    }
    if (server) {
        hints << QStringLiteral("Server: %1").arg(invokeOnObjectThread<QString>(server, QString{}, [server]() {
            return server ? server->statusLine() : QString{};
        }));
    }
    fallback.content = hints.join(QStringLiteral("\n")) + QStringLiteral("\n\n") + fallback.content;
    return fallback;
}

QString AdaptiveAiBackend::name() const {
    return QStringLiteral("Adaptive Local Runtime");
}

QString AdaptiveAiBackend::statusLine() const {
    auto* runtime = m_runtimeService.data();
    auto* server = m_serverService.data();
    const bool hasModel = invokeOnObjectThread<bool>(runtime, false, [runtime]() {
        return runtime && runtime->hasActiveModel();
    });
    const QString model = hasModel
        ? invokeOnObjectThread<QString>(runtime, QStringLiteral("no-active-gguf"), [runtime]() {
            return runtime ? runtime->activeDisplayName() : QStringLiteral("no-active-gguf");
        })
        : QStringLiteral("no-active-gguf");
    const bool healthy = invokeOnObjectThread<bool>(server, false, [server]() {
        return server && server->isHealthy();
    });
    if (healthy) {
        const QString baseUrl = invokeOnObjectThread<QString>(server, QStringLiteral("http://127.0.0.1:8080"), [server]() {
            return server ? server->baseUrl() : QStringLiteral("http://127.0.0.1:8080");
        });
        return QStringLiteral("online · %1 · %2").arg(baseUrl, model);
    }
    const bool starting = invokeOnObjectThread<bool>(server, false, [server]() {
        return server && (server->isRunning() || server->isStarting());
    });
    if (starting) {
        const QString baseUrl = invokeOnObjectThread<QString>(server, QStringLiteral("http://127.0.0.1:8080"), [server]() {
            return server ? server->baseUrl() : QStringLiteral("http://127.0.0.1:8080");
        });
        return QStringLiteral("starting · %1 · %2").arg(baseUrl, model);
    }
    return QStringLiteral("offline · start local server to bind chat · %1").arg(model);
}

bool AdaptiveAiBackend::isAvailable() const {
    return m_runtimeService || m_serverService;
}

} // namespace ide::adapters::ai
