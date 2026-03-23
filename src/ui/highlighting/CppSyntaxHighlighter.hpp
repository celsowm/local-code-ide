#pragma once

#include <QRegularExpression>
#include <QString>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <vector>

namespace ide::ui::highlighting {

class CppSyntaxHighlighter final : public QSyntaxHighlighter {
public:
    struct DiagnosticRange {
        int lineStart = 1;
        int columnStart = 1;
        int lineEnd = 1;
        int columnEnd = 1;
        QString severity;
    };

    explicit CppSyntaxHighlighter(QTextDocument* parent = nullptr);
    void setDiagnostics(std::vector<DiagnosticRange> diagnostics);
    void setLexicalHighlightingEnabled(bool enabled);

protected:
    void highlightBlock(const QString& text) override;

private:
    struct Rule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    std::vector<Rule> m_rules;
    QTextCharFormat m_multiLineCommentFormat;
    QRegularExpression m_commentStart;
    QRegularExpression m_commentEnd;
    std::vector<DiagnosticRange> m_diagnostics;
    bool m_lexicalHighlightingEnabled = true;
};

} // namespace ide::ui::highlighting
