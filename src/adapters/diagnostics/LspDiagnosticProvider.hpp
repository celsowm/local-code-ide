#pragma once

#include "adapters/diagnostics/LspClient.hpp"
#include "services/interfaces/IDiagnosticProvider.hpp"

#include <memory>

namespace ide::adapters::diagnostics {

class LspDiagnosticProvider final : public ide::services::interfaces::IDiagnosticProvider {
public:
    explicit LspDiagnosticProvider(std::shared_ptr<LspClient> client);

    std::vector<ide::services::interfaces::Diagnostic> analyze(const QString& filePath, const QString& text) override;
    QString name() const override;
    QString statusLine() const;

private:
    std::shared_ptr<LspClient> m_client;
};

} // namespace ide::adapters::diagnostics
