#include "services/DiagnosticService.hpp"

namespace ide::services {

DiagnosticService::DiagnosticService(std::unique_ptr<interfaces::IDiagnosticProvider> provider)
    : m_provider(std::move(provider)) {}

std::vector<interfaces::Diagnostic> DiagnosticService::refresh(const QString& filePath, const QString& text) {
    return m_provider->analyze(filePath, text);
}

QString DiagnosticService::providerName() const {
    return m_provider->name();
}

} // namespace ide::services
