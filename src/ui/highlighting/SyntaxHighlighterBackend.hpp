#pragma once

#include <QSyntaxHighlighter>
#include <QString>
#include <vector>

namespace ide::ui::highlighting {

class SyntaxHighlighterBackend : public QSyntaxHighlighter {
public:
    struct DiagnosticRange {
        int lineStart = 1;
        int columnStart = 1;
        int lineEnd = 1;
        int columnEnd = 1;
        QString severity;
    };

    explicit SyntaxHighlighterBackend(QTextDocument* parent = nullptr);
    ~SyntaxHighlighterBackend() override = default;

    void setDiagnostics(std::vector<DiagnosticRange> diagnostics);

protected:
    void applyDiagnosticsOverlay(const QString& text);
    std::vector<DiagnosticRange> m_diagnostics;
};

} // namespace ide::ui::highlighting
