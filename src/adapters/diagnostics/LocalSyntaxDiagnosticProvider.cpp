#include "adapters/diagnostics/LocalSyntaxDiagnosticProvider.hpp"

#include <QStringList>

namespace ide::adapters::diagnostics {
using Diagnostic = ide::services::interfaces::Diagnostic;

namespace {
Diagnostic makeDiag(int line, int column, QString message, Diagnostic::Severity severity) {
    Diagnostic diag;
    diag.line = line;
    diag.column = column;
    diag.message = std::move(message);
    diag.severity = severity;
    return diag;
}
} // namespace

std::vector<Diagnostic> LocalSyntaxDiagnosticProvider::analyze(const QString&, const QString& text) {
    std::vector<Diagnostic> diagnostics;
    const QStringList lines = text.split('\n');

    int braceBalance = 0;
    for (int i = 0; i < lines.size(); ++i) {
        const QString& line = lines[i];

        if (line.contains("TODO")) {
            diagnostics.push_back(makeDiag(i + 1, line.indexOf("TODO") + 1, "TODO pendente neste arquivo.", Diagnostic::Severity::Warning));
        }
        if (line.contains("FIXME")) {
            diagnostics.push_back(makeDiag(i + 1, line.indexOf("FIXME") + 1, "FIXME encontrado.", Diagnostic::Severity::Warning));
        }
        if (line.contains("using namespace std")) {
            diagnostics.push_back(makeDiag(i + 1, line.indexOf("using namespace std") + 1, "Evite `using namespace std` no escopo global.", Diagnostic::Severity::Info));
        }

        braceBalance += line.count('{');
        braceBalance -= line.count('}');
        if (braceBalance < 0) {
            diagnostics.push_back(makeDiag(i + 1, 1, "Chave de fechamento sem abertura correspondente.", Diagnostic::Severity::Error));
            braceBalance = 0;
        }
    }

    if (braceBalance > 0) {
        diagnostics.push_back(makeDiag(lines.size(), 1, "Uma ou mais chaves não foram fechadas.", Diagnostic::Severity::Error));
    }

    return diagnostics;
}

QString LocalSyntaxDiagnosticProvider::name() const {
    return "Local Syntax Provider";
}

} // namespace ide::adapters::diagnostics
