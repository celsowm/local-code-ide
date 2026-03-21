#pragma once

#include <QString>

namespace ide::services::interfaces {

struct TerminalResult {
    int exitCode = -1;
    QString command;
    QString output;
};

class ITerminalBackend {
public:
    virtual ~ITerminalBackend() = default;
    virtual TerminalResult run(const QString& workingDirectory, const QString& command) = 0;
    virtual QString name() const = 0;
};

} // namespace ide::services::interfaces
