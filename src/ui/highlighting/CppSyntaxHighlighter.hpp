#pragma once

#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <vector>

namespace ide::ui::highlighting {

class CppSyntaxHighlighter final : public QSyntaxHighlighter {
public:
    explicit CppSyntaxHighlighter(QTextDocument* parent = nullptr);

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
};

} // namespace ide::ui::highlighting
