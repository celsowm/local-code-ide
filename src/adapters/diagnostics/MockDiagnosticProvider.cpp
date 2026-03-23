#include "adapters/diagnostics/MockDiagnosticProvider.hpp"

#include <QStringList>

namespace ide::adapters::diagnostics {
using Diagnostic = ide::services::interfaces::Diagnostic;

namespace {
Diagnostic makeDiag(const QString& filePath,
                    int lineStart,
                    int columnStart,
                    QString message,
                    Diagnostic::Severity severity,
                    QString code = {}) {
    Diagnostic diag;
    diag.filePath = filePath;
    diag.lineStart = lineStart;
    diag.columnStart = columnStart;
    diag.lineEnd = lineStart;
    diag.columnEnd = columnStart + 1;
    diag.message = std::move(message);
    diag.severity = severity;
    diag.source = QStringLiteral("mock-diagnostics");
    diag.code = std::move(code);
    return diag;
}
} // namespace

std::vector<Diagnostic> MockDiagnosticProvider::analyze(const QString& filePath, const QString& text) {
    std::vector<Diagnostic> diagnostics;
    const QStringList lines = text.split('\n');

    int braceBalance = 0;
    for (int i = 0; i < lines.size(); ++i) {
        const QString& line = lines[i];

        if (line.contains("TODO")) {
            diagnostics.push_back(makeDiag(filePath, i + 1, line.indexOf("TODO") + 1, "TODO item still pending.", Diagnostic::Severity::Warning, QStringLiteral("mock.todo")));
        }
        if (line.contains("FIXME")) {
            diagnostics.push_back(makeDiag(filePath, i + 1, line.indexOf("FIXME") + 1, "FIXME marker found.", Diagnostic::Severity::Warning, QStringLiteral("mock.fixme")));
        }
        if (line.contains("unsafe")) {
            diagnostics.push_back(makeDiag(filePath, i + 1, line.indexOf("unsafe") + 1, "Suspicious unsafe usage detected by mock provider.", Diagnostic::Severity::Info, QStringLiteral("mock.unsafe")));
        }

        braceBalance += line.count('{');
        braceBalance -= line.count('}');
        if (braceBalance < 0) {
            diagnostics.push_back(makeDiag(filePath, i + 1, 1, "Unmatched closing brace.", Diagnostic::Severity::Error, QStringLiteral("mock.unmatchedClosingBrace")));
            braceBalance = 0;
        }
    }

    if (braceBalance > 0) {
        diagnostics.push_back(makeDiag(filePath, lines.size(), 1, "One or more opening braces are not closed.", Diagnostic::Severity::Error, QStringLiteral("mock.unmatchedOpeningBrace")));
    }

    if (diagnostics.empty()) {
        diagnostics.push_back(makeDiag(filePath, 1, 1, "No critical issues found by mock provider.", Diagnostic::Severity::Info, QStringLiteral("mock.none")));
    }

    return diagnostics;
}

QString MockDiagnosticProvider::name() const {
    return "Mock Diagnostic Provider";
}

} // namespace ide::adapters::diagnostics
