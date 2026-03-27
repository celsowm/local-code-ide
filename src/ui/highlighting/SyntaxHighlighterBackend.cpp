#include "ui/highlighting/SyntaxHighlighterBackend.hpp"

#include <QColor>
#include <QtGlobal>

namespace ide::ui::highlighting {

SyntaxHighlighterBackend::SyntaxHighlighterBackend(QTextDocument* parent)
    : QSyntaxHighlighter(parent) {
}

void SyntaxHighlighterBackend::setDiagnostics(std::vector<DiagnosticRange> diagnostics) {
    m_diagnostics = std::move(diagnostics);
    rehighlight();
}

void SyntaxHighlighterBackend::applyDiagnosticsOverlay(const QString& text) {
    const int currentLine = currentBlock().blockNumber() + 1;
    for (const auto& diag : m_diagnostics) {
        if (currentLine < diag.lineStart || currentLine > diag.lineEnd) {
            continue;
        }

        int startCol = 1;
        int endCol = qMax(2, text.size() + 1);
        if (currentLine == diag.lineStart) {
            startCol = qMax(1, diag.columnStart);
        }
        if (currentLine == diag.lineEnd) {
            endCol = qMax(startCol + 1, diag.columnEnd);
        }

        const int startIndex = qMax(0, startCol - 1);
        const int endIndex = qMin(text.size(), qMax(startIndex + 1, endCol - 1));
        const int length = qMax(1, endIndex - startIndex);

        QTextCharFormat overlay = format(startIndex);
        if (diag.severity == "error") {
            overlay.setUnderlineStyle(QTextCharFormat::WaveUnderline);
            overlay.setUnderlineColor(QColor("#f14c4c"));
        } else if (diag.severity == "warning") {
            overlay.setUnderlineStyle(QTextCharFormat::WaveUnderline);
            overlay.setUnderlineColor(QColor("#d7ba7d"));
        } else {
            overlay.setUnderlineStyle(QTextCharFormat::DotLine);
            overlay.setUnderlineColor(QColor("#4fc1ff"));
        }
        setFormat(startIndex, length, overlay);
    }
}

} // namespace ide::ui::highlighting
