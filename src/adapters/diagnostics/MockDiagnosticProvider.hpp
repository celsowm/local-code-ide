#pragma once

#include "services/interfaces/IDiagnosticProvider.hpp"

namespace ide::adapters::diagnostics {

class MockDiagnosticProvider final : public ide::services::interfaces::IDiagnosticProvider {
public:
    std::vector<ide::services::interfaces::Diagnostic> analyze(const QString& filePath, const QString& text) override;
    QString name() const override;
};

} // namespace ide::adapters::diagnostics
