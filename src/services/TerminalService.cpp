#include "services/TerminalService.hpp"

namespace ide::services {

TerminalService::TerminalService(std::unique_ptr<interfaces::ITerminalBackend> backend)
    : m_backend(std::move(backend)) {}

interfaces::TerminalResult TerminalService::run(const QString& workingDirectory, const QString& command) {
    return m_backend ? m_backend->run(workingDirectory, command) : interfaces::TerminalResult{};
}

QString TerminalService::backendName() const {
    return m_backend ? m_backend->name() : "No Terminal Backend";
}

} // namespace ide::services
