#pragma once

#include "services/interfaces/ITerminalBackend.hpp"

#include <memory>

namespace ide::services {

class TerminalService {
public:
    explicit TerminalService(std::unique_ptr<interfaces::ITerminalBackend> backend);

    interfaces::TerminalResult run(const QString& workingDirectory, const QString& command);
    QString backendName() const;

private:
    std::unique_ptr<interfaces::ITerminalBackend> m_backend;
};

} // namespace ide::services
