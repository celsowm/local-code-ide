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

    int line = 1;
    int column = 1;
    QString message;
    Severity severity = Severity::Info;
};

class IDiagnosticProvider {
public:
    virtual ~IDiagnosticProvider() = default;
    virtual std::vector<Diagnostic> analyze(const QString& filePath, const QString& text) = 0;
    virtual QString name() const = 0;
};

} // namespace ide::services::interfaces
