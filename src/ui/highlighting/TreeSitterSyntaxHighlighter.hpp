#pragma once

#include "ui/highlighting/SyntaxHighlighterBackend.hpp"

#include <QHash>
#include <QRegularExpression>
#include <QTextCharFormat>
#include <QStringList>
#include <vector>

struct TSParser;
struct TSTree;
struct TSNode;

namespace ide::ui::highlighting {

class TreeSitterSyntaxHighlighter final : public SyntaxHighlighterBackend {
public:
    enum class Language {
        Python,
        C,
        Cpp,
        Rust
    };

    explicit TreeSitterSyntaxHighlighter(Language language, QTextDocument* parent = nullptr);
    ~TreeSitterSyntaxHighlighter() override;

protected:
    void highlightBlock(const QString& text) override;

private:
    enum class TokenKind {
        Keyword,
        BuiltinType,
        String,
        Number,
        Comment,
        Decorator,
        FunctionName,
        ClassName
    };

    struct ByteSpan {
        int startByteCol = 0;
        int endByteCol = 0;
        TokenKind kind = TokenKind::Keyword;
    };

    void initializeFormats();
    void initializeRegexFallback();
    void initializeTreeSitter();
    void connectDocumentSignals();

    void onContentsChanged(int position, int charsRemoved, int charsAdded);
    void fullReparse();

    void collectTreeSpans(TSNode root);
    void collectNodeRecursive(TSNode node);
    void applyLanguageSpecificNodeRules(TSNode node, const QString& type);
    void maybeMarkFunctionName(TSNode node, const QString& type);
    void maybeMarkClassLikeName(TSNode node, const QString& type);
    TSNode findFirstNamedDescendantByType(TSNode node, const QString& targetType) const;
    void addNodeSpan(TSNode node, TokenKind kind);
    void addLineSpan(int row, int startByteCol, int endByteCol, TokenKind kind);

    void applyRegexKeywords(const QString& text);
    void applyTreeSitterSpans(const QString& text, int blockNumber);
    int utf16IndexFromUtf8ByteColumn(const QString& lineText, int byteColumn) const;

    QTextCharFormat formatForKind(TokenKind kind) const;

    TSParser* m_parser = nullptr;
    TSTree* m_tree = nullptr;
    bool m_treeSitterReady = false;
    bool m_loggedInitFailure = false;
    QString m_cachedText;
    Language m_language;

    QHash<int, std::vector<ByteSpan>> m_spansByLine;

    QTextCharFormat m_keywordFormat;
    QTextCharFormat m_typeFormat;
    QTextCharFormat m_numberFormat;
    QTextCharFormat m_stringFormat;
    QTextCharFormat m_commentFormat;
    QTextCharFormat m_decoratorFormat;
    QTextCharFormat m_functionFormat;
    QTextCharFormat m_classFormat;

    std::vector<QRegularExpression> m_keywordPatterns;
    std::vector<QRegularExpression> m_builtinPatterns;
    QRegularExpression m_numberPattern;
};

} // namespace ide::ui::highlighting
