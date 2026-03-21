#pragma once

#include "adapters/ai/MockAiBackend.hpp"
#include "services/LlamaServerProcessService.hpp"
#include "services/LocalModelRuntimeService.hpp"
#include "services/interfaces/IAiBackend.hpp"

#include <QPointer>

namespace ide::adapters::ai {

class AdaptiveAiBackend final : public ide::services::interfaces::IAiBackend {
public:
    AdaptiveAiBackend(ide::services::LocalModelRuntimeService* runtimeService,
                      ide::services::LlamaServerProcessService* serverService,
                      int timeoutMs = 30000);

    ide::services::interfaces::AiResponse complete(const ide::services::interfaces::AiRequest& request) override;
    QString name() const override;
    QString statusLine() const override;
    bool isAvailable() const override;

private:
    QPointer<ide::services::LocalModelRuntimeService> m_runtimeService;
    QPointer<ide::services::LlamaServerProcessService> m_serverService;
    int m_timeoutMs = 30000;
    MockAiBackend m_mockBackend;
};

} // namespace ide::adapters::ai
