#include "ui/highlighting/CppSyntaxHighlighter.hpp"

#include <QBrush>
#include <QColor>
#include <QStringList>
#include <QtGlobal>

namespace ide::ui::highlighting {

namespace {
QTextCharFormat makeFormat(const QString& hexColor, bool bold = false, bool italic = false) {
    QTextCharFormat format;
    format.setForeground(QColor(hexColor));
    format.setFontWeight(bold ? QFont::Bold : QFont::Normal);
    format.setFontItalic(italic);
    return format;
}
} // namespace

CppSyntaxHighlighter::CppSyntaxHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent)
    , m_commentStart(QRegularExpression("/\\*"))
    , m_commentEnd(QRegularExpression("\\*/")) {
    const auto keywordFormat = makeFormat("#569CD6", true);
    const auto typeFormat = makeFormat("#4EC9B0");
    const auto numberFormat = makeFormat("#B5CEA8");
    const auto stringFormat = makeFormat("#CE9178");
    const auto commentFormat = makeFormat("#6A9955", false, true);
    const auto preprocessorFormat = makeFormat("#C586C0");

    const QStringList keywords = {
        "alignas", "alignof", "auto", "bool", "break", "case", "catch", "class", "const",
        "constexpr", "continue", "default", "delete", "do", "else", "enum", "explicit", "extern",
        "false", "for", "friend", "if", "inline", "mutable", "namespace", "new", "noexcept",
        "nullptr", "operator", "private", "protected", "public", "return", "signed", "sizeof",
        "static", "struct", "switch", "template", "this", "throw", "true", "try", "typename",
        "union", "unsigned", "using", "virtual", "void", "while"
    };

    for (const QString& keyword : keywords) {
        m_rules.push_back({QRegularExpression(QString("\\b%1\\b").arg(keyword)), keywordFormat});
    }

    const QStringList builtinTypes = {
        "int", "float", "double", "char", "long", "short", "std", "size_t", "QString", "QObject"
    };

    for (const QString& type : builtinTypes) {
        m_rules.push_back({QRegularExpression(QString("\\b%1\\b").arg(type)), typeFormat});
    }

    m_rules.push_back({QRegularExpression("\"[^\"]*\""), stringFormat});
    m_rules.push_back({QRegularExpression("'[^']*'"), stringFormat});
    m_rules.push_back({QRegularExpression("//[^\\n]*"), commentFormat});
    m_rules.push_back({QRegularExpression("#[^\\n]*"), commentFormat});
    m_rules.push_back({QRegularExpression("\\b[0-9]+\\b"), numberFormat});
    m_rules.push_back({QRegularExpression("^\\s*#[^\\n]*"), preprocessorFormat});

    m_multiLineCommentFormat = commentFormat;
}

void CppSyntaxHighlighter::setDiagnostics(std::vector<DiagnosticRange> diagnostics) {
    m_diagnostics = std::move(diagnostics);
    rehighlight();
}

void CppSyntaxHighlighter::setLexicalHighlightingEnabled(bool enabled) {
    if (m_lexicalHighlightingEnabled == enabled) {
        return;
    }
    m_lexicalHighlightingEnabled = enabled;
    rehighlight();
}

void CppSyntaxHighlighter::highlightBlock(const QString& text) {
    if (m_lexicalHighlightingEnabled) {
        for (const auto& rule : m_rules) {
            auto matchIterator = rule.pattern.globalMatch(text);
            while (matchIterator.hasNext()) {
                const auto match = matchIterator.next();
                setFormat(match.capturedStart(), match.capturedLength(), rule.format);
            }
        }

        setCurrentBlockState(0);

        int startIndex = 0;
        if (previousBlockState() != 1) {
            startIndex = text.indexOf(m_commentStart);
        }

        while (startIndex >= 0) {
            const auto endMatch = m_commentEnd.match(text, startIndex);
            int endIndex = endMatch.capturedStart();
            int commentLength = 0;

            if (endIndex == -1) {
                setCurrentBlockState(1);
                commentLength = text.length() - startIndex;
            } else {
                commentLength = endIndex - startIndex + endMatch.capturedLength();
            }

            setFormat(startIndex, commentLength, m_multiLineCommentFormat);
            startIndex = text.indexOf(m_commentStart, startIndex + commentLength);
        }
    } else {
        setCurrentBlockState(0);
    }

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
