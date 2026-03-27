#include "ui/highlighting/SyntaxHighlighterFactory.hpp"

#include "ui/highlighting/RegexSyntaxHighlighter.hpp"
#include "ui/highlighting/TreeSitterSyntaxHighlighter.hpp"

#include <QSet>

namespace ide::ui::highlighting {

std::unique_ptr<SyntaxHighlighterBackend> DefaultSyntaxHighlighterFactory::create(
    const QString& normalizedLanguage,
    QTextDocument* document) const {
    if (normalizedLanguage == QStringLiteral("python")) {
        return std::make_unique<TreeSitterSyntaxHighlighter>(TreeSitterSyntaxHighlighter::Language::Python, document);
    }
    if (normalizedLanguage == QStringLiteral("c")) {
        return std::make_unique<TreeSitterSyntaxHighlighter>(TreeSitterSyntaxHighlighter::Language::C, document);
    }
    if (normalizedLanguage == QStringLiteral("cpp")) {
        return std::make_unique<TreeSitterSyntaxHighlighter>(TreeSitterSyntaxHighlighter::Language::Cpp, document);
    }
    if (normalizedLanguage == QStringLiteral("rust")) {
        return std::make_unique<TreeSitterSyntaxHighlighter>(TreeSitterSyntaxHighlighter::Language::Rust, document);
    }

    auto regex = std::make_unique<RegexSyntaxHighlighter>(document);
    static const QSet<QString> lexicalLanguages = {
        QStringLiteral("python"),
        QStringLiteral("cpp"),
        QStringLiteral("c"),
        QStringLiteral("rust"),
        QStringLiteral("typescript"),
        QStringLiteral("javascript"),
        QStringLiteral("qml"),
        QStringLiteral("json"),
        QStringLiteral("yaml"),
        QStringLiteral("toml"),
        QStringLiteral("ini"),
        QStringLiteral("powershell"),
        QStringLiteral("markdown")
    };
    regex->setLexicalHighlightingEnabled(lexicalLanguages.contains(normalizedLanguage));
    return regex;
}

} // namespace ide::ui::highlighting
