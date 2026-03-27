#pragma once

#include "ui/highlighting/SyntaxHighlighterBackend.hpp"

#include <QRegularExpression>
#include <QString>
#include <QTextCharFormat>
#include <vector>

namespace ide::ui::highlighting {

class RegexSyntaxHighlighter final : public SyntaxHighlighterBackend {
public:
    explicit RegexSyntaxHighlighter(QTextDocument* parent = nullptr);
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
    bool m_lexicalHighlightingEnabled = true;
};

} // namespace ide::ui::highlighting
