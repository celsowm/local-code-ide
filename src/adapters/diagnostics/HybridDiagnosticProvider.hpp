#pragma once

#include "services/interfaces/IDiagnosticProvider.hpp"

#include <memory>
#include <vector>

namespace ide::adapters::diagnostics {

class HybridDiagnosticProvider final : public ide::services::interfaces::IDiagnosticProvider {
public:
    HybridDiagnosticProvider();
    explicit HybridDiagnosticProvider(std::vector<std::unique_ptr<ide::services::interfaces::IDiagnosticProvider>> providers);

    std::vector<ide::services::interfaces::Diagnostic> analyze(const QString& filePath, const QString& text) override;
    QString name() const override;

    void addProvider(std::unique_ptr<ide::services::interfaces::IDiagnosticProvider> provider);

private:
    std::vector<std::unique_ptr<ide::services::interfaces::IDiagnosticProvider>> m_providers;
};

} // namespace ide::adapters::diagnostics
