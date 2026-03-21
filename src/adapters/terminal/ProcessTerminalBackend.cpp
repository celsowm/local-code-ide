#include "adapters/terminal/ProcessTerminalBackend.hpp"

#include <QDir>
#include <QProcess>

namespace ide::adapters::terminal {
using TerminalResult = ide::services::interfaces::TerminalResult;

TerminalResult ProcessTerminalBackend::run(const QString& workingDirectory, const QString& command) {
    TerminalResult result;
    result.command = command;

    QProcess process;
    process.setWorkingDirectory(workingDirectory.isEmpty() ? QDir::currentPath() : workingDirectory);

#ifdef Q_OS_WIN
    process.start("cmd", {"/C", command});
#else
    process.start("/bin/sh", {"-lc", command});
#endif

    if (!process.waitForStarted(1000)) {
        result.output = "Falha ao iniciar processo.";
        return result;
    }

    process.waitForFinished(15000);
    result.exitCode = process.exitCode();
    result.output =
        QString::fromUtf8(process.readAllStandardOutput()) +
        QString::fromUtf8(process.readAllStandardError());

    if (result.output.trimmed().isEmpty()) {
        result.output = QString("[sem saída] exit=%1").arg(result.exitCode);
    }

    return result;
}

QString ProcessTerminalBackend::name() const {
    return "Process Terminal Backend";
}

} // namespace ide::adapters::terminal
