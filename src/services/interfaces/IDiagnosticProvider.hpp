#pragma once

#include <QString>
#include <vector>

namespace ide::services::interfaces {

struct Diagnostic {
    enum class Severity {
        Info,
        Warning,
        Error
    };

    QString filePath;
    int lineStart = 1;
    int columnStart = 1;
    int lineEnd = 1;
    int columnEnd = 1;
    QString message;
    Severity severity = Severity::Info;
    QString source;
    QString code;
};

class IDiagnosticProvider {
public:
    virtual ~IDiagnosticProvider() = default;
    virtual std::vector<Diagnostic> analyze(const QString& filePath, const QString& text) = 0;
    virtual QString name() const = 0;
};

} // namespace ide::services::interfaces
