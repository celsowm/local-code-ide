#pragma once

#include "services/interfaces/ITerminalBackend.hpp"

namespace ide::adapters::terminal {

class ProcessTerminalBackend final : public ide::services::interfaces::ITerminalBackend {
public:
    ide::services::interfaces::TerminalResult run(const QString& workingDirectory,
                                                  const QString& command) override;
    QString name() const override;
};

} // namespace ide::adapters::terminal
