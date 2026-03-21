#include "adapters/diagnostics/HybridDiagnosticProvider.hpp"

namespace ide::adapters::diagnostics {
using Diagnostic = ide::services::interfaces::Diagnostic;

HybridDiagnosticProvider::HybridDiagnosticProvider() = default;

HybridDiagnosticProvider::HybridDiagnosticProvider(std::vector<std::unique_ptr<ide::services::interfaces::IDiagnosticProvider>> providers)
    : m_providers(std::move(providers)) {}

std::vector<Diagnostic> HybridDiagnosticProvider::analyze(const QString& filePath, const QString& text) {
    std::vector<Diagnostic> result;

    for (const auto& provider : m_providers) {
        const auto diagnostics = provider->analyze(filePath, text);
        result.insert(result.end(), diagnostics.begin(), diagnostics.end());
    }

    return result;
}

QString HybridDiagnosticProvider::name() const {
    QStringList names;
    names.reserve(static_cast<qsizetype>(m_providers.size()));
    for (const auto& provider : m_providers) {
        names.push_back(provider->name());
    }
    return names.isEmpty() ? "No Diagnostic Providers" : names.join(" + ");
}

void HybridDiagnosticProvider::addProvider(std::unique_ptr<ide::services::interfaces::IDiagnosticProvider> provider) {
    m_providers.push_back(std::move(provider));
}

} // namespace ide::adapters::diagnostics
