#include "adapters/diagnostics/LocalSyntaxDiagnosticProvider.hpp"

#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QStringList>
#include <QtGlobal>

namespace ide::adapters::diagnostics {
using Diagnostic = ide::services::interfaces::Diagnostic;

namespace {
Diagnostic makeDiag(const QString& filePath,
                    int lineStart,
                    int columnStart,
                    int lineEnd,
                    int columnEnd,
                    QString message,
                    Diagnostic::Severity severity,
                    QString source,
                    QString code = {}) {
    Diagnostic diag;
    diag.filePath = filePath;
    diag.lineStart = qMax(1, lineStart);
    diag.columnStart = qMax(1, columnStart);
    diag.lineEnd = qMax(diag.lineStart, lineEnd);
    diag.columnEnd = qMax(1, columnEnd);
    diag.message = std::move(message);
    diag.severity = severity;
    diag.source = std::move(source);
    diag.code = std::move(code);
    return diag;
}

int lineForOffset(const QString& text, int offset) {
    int line = 1;
    const int boundedOffset = qMax(0, qMin(offset, text.size()));
    for (int i = 0; i < boundedOffset; ++i) {
        if (text.at(i) == QChar('\n')) {
            ++line;
        }
    }
    return line;
}

int columnForOffset(const QString& text, int offset) {
    const int boundedOffset = qMax(0, qMin(offset, text.size()));
    int column = 1;
    for (int i = boundedOffset - 1; i >= 0; --i) {
        if (text.at(i) == QChar('\n')) {
            break;
        }
        ++column;
    }
    return column;
}

bool isCodeLike(const QString& suffix) {
    return suffix == "rs" || suffix == "cpp" || suffix == "cxx" || suffix == "cc" || suffix == "c"
        || suffix == "h" || suffix == "hpp" || suffix == "hh" || suffix == "py" || suffix == "js"
        || suffix == "ts" || suffix == "tsx" || suffix == "jsx" || suffix == "qml" || suffix == "ps1";
}

void addBraceChecks(const QString& filePath,
                    const QStringList& lines,
                    std::vector<Diagnostic>& diagnostics,
                    const QString& source) {
    int balance = 0;
    for (int i = 0; i < lines.size(); ++i) {
        const QString& line = lines.at(i);
        balance += line.count('{');
        balance -= line.count('}');
        if (balance < 0) {
            diagnostics.push_back(makeDiag(
                filePath, i + 1, 1, i + 1, qMax(1, line.size()), "Unmatched closing brace.",
                Diagnostic::Severity::Error, source, "local.unmatchedClosingBrace"));
            balance = 0;
        }
    }
    if (balance > 0) {
        diagnostics.push_back(makeDiag(
            filePath, lines.size(), 1, lines.size(), 1, "One or more opening braces are not closed.",
            Diagnostic::Severity::Error, source, "local.unmatchedOpeningBrace"));
    }
}

void addJsonChecks(const QString& filePath, const QString& text, std::vector<Diagnostic>& diagnostics) {
    QJsonParseError error;
    QJsonDocument::fromJson(text.toUtf8(), &error);
    if (error.error == QJsonParseError::NoError) {
        return;
    }
    const int line = lineForOffset(text, static_cast<int>(error.offset));
    const int column = columnForOffset(text, static_cast<int>(error.offset));
    diagnostics.push_back(makeDiag(
        filePath,
        line,
        column,
        line,
        column + 1,
        QStringLiteral("Invalid JSON: %1").arg(error.errorString()),
        Diagnostic::Severity::Error,
        QStringLiteral("local-json"),
        QStringLiteral("local.json.parseError")));
}

void addIniTomlYamlChecks(const QString& filePath,
                          const QStringList& lines,
                          const QString& suffix,
                          std::vector<Diagnostic>& diagnostics) {
    const QString source = suffix == "toml" ? QStringLiteral("local-toml")
                                            : (suffix == "yaml" || suffix == "yml")
                                                  ? QStringLiteral("local-yaml")
                                                  : QStringLiteral("local-settings");
    for (int i = 0; i < lines.size(); ++i) {
        const QString raw = lines.at(i);
        const QString line = raw.trimmed();
        if (line.isEmpty() || line.startsWith('#') || line.startsWith(';')) {
            continue;
        }

        if ((suffix == "ini" || suffix == "conf") && line.startsWith('[') && !line.endsWith(']')) {
            diagnostics.push_back(makeDiag(
                filePath, i + 1, 1, i + 1, qMax(1, raw.size()),
                "Section header must end with ']'.", Diagnostic::Severity::Error, source,
                QStringLiteral("local.settings.invalidSection")));
        }

        if ((suffix == "toml" || suffix == "ini" || suffix == "conf")
            && !line.startsWith('[') && !line.contains('=') && !line.contains(':')) {
            diagnostics.push_back(makeDiag(
                filePath, i + 1, 1, i + 1, qMax(1, raw.size()),
                "Expected a key/value assignment.", Diagnostic::Severity::Warning, source,
                QStringLiteral("local.settings.assignmentExpected")));
        }

        if ((suffix == "yaml" || suffix == "yml") && raw.startsWith('\t')) {
            diagnostics.push_back(makeDiag(
                filePath, i + 1, 1, i + 1, 2,
                "YAML indentation should use spaces, not tabs.", Diagnostic::Severity::Warning, source,
                QStringLiteral("local.yaml.tabIndent")));
        }
    }
}

void addMarkdownChecks(const QString& filePath, const QStringList& lines, std::vector<Diagnostic>& diagnostics) {
    int previousHeading = 0;
    for (int i = 0; i < lines.size(); ++i) {
        const QString& line = lines.at(i);
        if (line.startsWith('#')) {
            int level = 0;
            while (level < line.size() && line.at(level) == '#') {
                ++level;
            }
            if (previousHeading > 0 && level > previousHeading + 1) {
                diagnostics.push_back(makeDiag(
                    filePath, i + 1, 1, i + 1, level,
                    QStringLiteral("Heading level jumps from H%1 to H%2.").arg(previousHeading).arg(level),
                    Diagnostic::Severity::Info, QStringLiteral("local-markdown"),
                    QStringLiteral("local.markdown.headingJump")));
            }
            previousHeading = level;
        }

        const int linkOpen = line.count("[");
        const int linkClose = line.count("]");
        const int parenOpen = line.count("(");
        const int parenClose = line.count(")");
        if (linkOpen != linkClose || parenOpen != parenClose) {
            diagnostics.push_back(makeDiag(
                filePath, i + 1, 1, i + 1, qMax(1, line.size()),
                "Possible malformed link or parenthesis pair.", Diagnostic::Severity::Warning,
                QStringLiteral("local-markdown"), QStringLiteral("local.markdown.linkShape")));
        }
    }
}
} // namespace

std::vector<Diagnostic> LocalSyntaxDiagnosticProvider::analyze(const QString& filePath, const QString& text) {
    std::vector<Diagnostic> diagnostics;
    const QStringList lines = text.split('\n');
    const QString suffix = QFileInfo(filePath).suffix().toLower();
    const QString source = QStringLiteral("local-syntax");

    for (int i = 0; i < lines.size(); ++i) {
        const QString& line = lines[i];

        if (line.contains("TODO")) {
            diagnostics.push_back(makeDiag(
                filePath, i + 1, line.indexOf("TODO") + 1, i + 1, line.indexOf("TODO") + 5,
                "TODO item still pending in this file.", Diagnostic::Severity::Warning, source,
                QStringLiteral("local.todo")));
        }
        if (line.contains("FIXME")) {
            diagnostics.push_back(makeDiag(
                filePath, i + 1, line.indexOf("FIXME") + 1, i + 1, line.indexOf("FIXME") + 6,
                "FIXME marker found.", Diagnostic::Severity::Warning, source, QStringLiteral("local.fixme")));
        }
        if (line.contains("using namespace std")) {
            diagnostics.push_back(makeDiag(
                filePath, i + 1, line.indexOf("using namespace std") + 1, i + 1,
                line.indexOf("using namespace std") + 20,
                "Avoid `using namespace std` in global scope.", Diagnostic::Severity::Info, source,
                QStringLiteral("local.cpp.usingNamespaceStd")));
        }
    }

    if (suffix == "json" || suffix == "jsonc" || suffix == "json5") {
        addJsonChecks(filePath, text, diagnostics);
    } else if (suffix == "yaml" || suffix == "yml" || suffix == "toml" || suffix == "ini" || suffix == "conf") {
        addIniTomlYamlChecks(filePath, lines, suffix, diagnostics);
    } else if (suffix == "md" || suffix == "markdown" || suffix == "rst"
               || QFileInfo(filePath).fileName().toLower().startsWith("readme")
               || QFileInfo(filePath).fileName().toLower().startsWith("license")) {
        addMarkdownChecks(filePath, lines, diagnostics);
    }

    if (isCodeLike(suffix) || suffix == "qml" || suffix == "ps1" || suffix == "json" || suffix == "toml") {
        addBraceChecks(filePath, lines, diagnostics, source);
    }

    return diagnostics;
}

QString LocalSyntaxDiagnosticProvider::name() const {
    return "Local Syntax Provider";
}

} // namespace ide::adapters::diagnostics
