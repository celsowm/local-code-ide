#pragma once

#include "services/interfaces/IAiBackend.hpp"

namespace ide::adapters::ai {

class MockAiBackend final : public ide::services::interfaces::IAiBackend {
public:
    ide::services::interfaces::AiResponse complete(const ide::services::interfaces::AiRequest& request) override;
    QString name() const override;
    QString statusLine() const override;
    bool isAvailable() const override;
};

} // namespace ide::adapters::ai
