#include "adapters/diagnostics/LspDiagnosticProvider.hpp"

namespace ide::adapters::diagnostics {

LspDiagnosticProvider::LspDiagnosticProvider(std::shared_ptr<LspClient> client)
    : m_client(std::move(client)) {}

std::vector<ide::services::interfaces::Diagnostic> LspDiagnosticProvider::analyze(const QString& filePath, const QString& text) {
    if (!m_client) {
        return {};
    }
    return m_client->publishAndCollect(filePath, text);
}

QString LspDiagnosticProvider::name() const {
    return "LSP Diagnostic Provider";
}

QString LspDiagnosticProvider::statusLine() const {
    if (!m_client) {
        return "disabled";
    }
    return m_client->statusLine();
}

} // namespace ide::adapters::diagnostics
