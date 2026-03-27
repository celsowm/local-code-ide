#include "ui/highlighting/TreeSitterSyntaxHighlighter.hpp"

#include <QDebug>
#include <QTextDocument>

extern "C" {
#include <tree_sitter/api.h>
const TSLanguage* tree_sitter_python(void);
const TSLanguage* tree_sitter_c(void);
const TSLanguage* tree_sitter_cpp(void);
const TSLanguage* tree_sitter_rust(void);
}

namespace ide::ui::highlighting {

namespace {
QTextCharFormat makeFormat(const QString& hexColor, bool bold = false, bool italic = false) {
    QTextCharFormat format;
    format.setForeground(QColor(hexColor));
    format.setFontWeight(bold ? QFont::Bold : QFont::Normal);
    format.setFontItalic(italic);
    return format;
}

uint32_t byteOffsetForUtf16Index(const QString& text, int utf16Index) {
    const int clamped = qBound(0, utf16Index, text.size());
    return static_cast<uint32_t>(text.left(clamped).toUtf8().size());
}

TSPoint pointForUtf16Index(const QString& text, int utf16Index) {
    const int clamped = qBound(0, utf16Index, text.size());
    const QString prefix = text.left(clamped);
    const int line = prefix.count('\n');
    const QString lastLine = prefix.section('\n', -1);
    return TSPoint{static_cast<uint32_t>(line), static_cast<uint32_t>(lastLine.toUtf8().size())};
}
} // namespace

TreeSitterSyntaxHighlighter::TreeSitterSyntaxHighlighter(Language language, QTextDocument* parent)
    : SyntaxHighlighterBackend(parent)
    , m_numberPattern(QStringLiteral("\\b([0-9]+(?:\\.[0-9]+)?)\\b"))
    , m_language(language) {
    initializeFormats();
    initializeRegexFallback();
    initializeTreeSitter();
    connectDocumentSignals();
}

TreeSitterSyntaxHighlighter::~TreeSitterSyntaxHighlighter() {
    if (m_tree) {
        ts_tree_delete(m_tree);
        m_tree = nullptr;
    }
    if (m_parser) {
        ts_parser_delete(m_parser);
        m_parser = nullptr;
    }
}

void TreeSitterSyntaxHighlighter::initializeFormats() {
    m_keywordFormat = makeFormat("#569CD6", true);
    m_typeFormat = makeFormat("#4EC9B0");
    m_numberFormat = makeFormat("#B5CEA8");
    m_stringFormat = makeFormat("#CE9178");
    m_commentFormat = makeFormat("#6A9955", false, true);
    m_decoratorFormat = makeFormat("#C586C0");
    m_functionFormat = makeFormat("#DCDCAA");
    m_classFormat = makeFormat("#4EC9B0", true);
}

void TreeSitterSyntaxHighlighter::initializeRegexFallback() {
    QStringList keywords;
    QStringList builtins;
    if (m_language == Language::Python) {
        keywords = {
            "and", "as", "assert", "async", "await", "break", "class", "continue",
            "def", "del", "elif", "else", "except", "False", "finally", "for", "from",
            "global", "if", "import", "in", "is", "lambda", "match", "None", "nonlocal",
            "not", "or", "pass", "raise", "return", "True", "try", "while", "with", "yield"
        };
        builtins = {"str", "list", "dict", "set", "tuple", "bytes", "object", "int", "float", "bool"};
    } else if (m_language == Language::Cpp) {
        keywords = {
            "alignas", "alignof", "auto", "bool", "break", "case", "catch", "char", "class",
            "const", "constexpr", "continue", "default", "delete", "do", "double", "else",
            "enum", "explicit", "extern", "false", "float", "for", "friend", "if", "inline",
            "int", "long", "namespace", "new", "noexcept", "nullptr", "operator", "private",
            "protected", "public", "register", "return", "short", "signed", "sizeof", "static",
            "struct", "switch", "template", "this", "throw", "true", "try", "typedef", "typename",
            "union", "unsigned", "using", "virtual", "void", "volatile", "while"
        };
        builtins = {"int", "float", "double", "char", "long", "short", "bool", "void", "size_t"};
    } else {
        keywords = {
            "as", "async", "await", "break", "const", "continue", "crate", "dyn", "else",
            "enum", "extern", "false", "fn", "for", "if", "impl", "in", "let", "loop", "match",
            "mod", "move", "mut", "pub", "ref", "return", "self", "Self", "static", "struct",
            "super", "trait", "true", "type", "unsafe", "use", "where", "while"
        };
        builtins = {
            "i8", "i16", "i32", "i64", "i128", "isize",
            "u8", "u16", "u32", "u64", "u128", "usize",
            "f32", "f64", "bool", "char", "str"
        };
    }

    for (const QString& keyword : keywords) {
        m_keywordPatterns.emplace_back(QStringLiteral("\\b%1\\b").arg(QRegularExpression::escape(keyword)));
    }

    for (const QString& type : builtins) {
        m_builtinPatterns.emplace_back(QStringLiteral("\\b%1\\b").arg(QRegularExpression::escape(type)));
    }
}

void TreeSitterSyntaxHighlighter::initializeTreeSitter() {
    m_parser = ts_parser_new();
    if (!m_parser) {
        if (!m_loggedInitFailure) {
            qWarning() << "Tree-sitter parser allocation failed; falling back to regex highlighting.";
            m_loggedInitFailure = true;
        }
        m_treeSitterReady = false;
        return;
    }
    const TSLanguage* language = nullptr;
    if (m_language == Language::Python) {
        language = tree_sitter_python();
    } else if (m_language == Language::C) {
        language = tree_sitter_c();
    } else if (m_language == Language::Cpp) {
        language = tree_sitter_cpp();
    } else {
        language = tree_sitter_rust();
    }
    if (!language || !ts_parser_set_language(m_parser, language)) {
        if (!m_loggedInitFailure) {
            qWarning() << "Tree-sitter language init failed; falling back to regex highlighting.";
            m_loggedInitFailure = true;
        }
        m_treeSitterReady = false;
        return;
    }
    m_treeSitterReady = true;
}

void TreeSitterSyntaxHighlighter::connectDocumentSignals() {
    if (!document()) {
        return;
    }
    m_cachedText = document()->toPlainText();
    connect(document(), &QTextDocument::contentsChange, this, &TreeSitterSyntaxHighlighter::onContentsChanged);
    fullReparse();
}

void TreeSitterSyntaxHighlighter::onContentsChanged(int position, int charsRemoved, int charsAdded) {
    if (!m_treeSitterReady || !document() || !m_parser) {
        rehighlight();
        return;
    }

    const QString oldText = m_cachedText;
    const QString newText = document()->toPlainText();

    if (m_tree) {
        TSInputEdit edit;
        edit.start_byte = byteOffsetForUtf16Index(oldText, position);
        edit.old_end_byte = byteOffsetForUtf16Index(oldText, position + charsRemoved);
        edit.new_end_byte = byteOffsetForUtf16Index(newText, position + charsAdded);
        edit.start_point = pointForUtf16Index(oldText, position);
        edit.old_end_point = pointForUtf16Index(oldText, position + charsRemoved);
        edit.new_end_point = pointForUtf16Index(newText, position + charsAdded);
        ts_tree_edit(m_tree, &edit);
    }

    const QByteArray utf8 = newText.toUtf8();
    TSTree* newTree = ts_parser_parse_string(m_parser, m_tree, utf8.constData(), static_cast<uint32_t>(utf8.size()));
    if (newTree) {
        if (m_tree) {
            ts_tree_delete(m_tree);
        }
        m_tree = newTree;
        m_cachedText = newText;
        m_spansByLine.clear();
        collectTreeSpans(ts_tree_root_node(m_tree));
    }
    rehighlight();
}

void TreeSitterSyntaxHighlighter::fullReparse() {
    if (!m_treeSitterReady || !m_parser || !document()) {
        m_spansByLine.clear();
        rehighlight();
        return;
    }
    m_cachedText = document()->toPlainText();
    const QByteArray utf8 = m_cachedText.toUtf8();
    TSTree* newTree = ts_parser_parse_string(m_parser, nullptr, utf8.constData(), static_cast<uint32_t>(utf8.size()));
    if (!newTree) {
        if (!m_loggedInitFailure) {
            qWarning() << "Tree-sitter parse failed; falling back to regex highlighting.";
            m_loggedInitFailure = true;
        }
        m_treeSitterReady = false;
        m_spansByLine.clear();
        rehighlight();
        return;
    }
    if (m_tree) {
        ts_tree_delete(m_tree);
    }
    m_tree = newTree;
    m_spansByLine.clear();
    collectTreeSpans(ts_tree_root_node(m_tree));
    rehighlight();
}

void TreeSitterSyntaxHighlighter::collectTreeSpans(TSNode root) {
    if (ts_node_is_null(root)) {
        return;
    }
    collectNodeRecursive(root);
}

void TreeSitterSyntaxHighlighter::collectNodeRecursive(TSNode node) {
    if (ts_node_is_null(node)) {
        return;
    }

    const QString type = QString::fromUtf8(ts_node_type(node));
    applyLanguageSpecificNodeRules(node, type);

    const uint32_t childCount = ts_node_child_count(node);
    for (uint32_t i = 0; i < childCount; ++i) {
        collectNodeRecursive(ts_node_child(node, i));
    }
}

void TreeSitterSyntaxHighlighter::applyLanguageSpecificNodeRules(TSNode node, const QString& type) {
    if (type.contains(QStringLiteral("comment"))) {
        addNodeSpan(node, TokenKind::Comment);
        return;
    }

    if (type.contains(QStringLiteral("string")) || type == QStringLiteral("char_literal")
        || type == QStringLiteral("raw_string_literal")) {
        addNodeSpan(node, TokenKind::String);
        return;
    }

    if (type == QStringLiteral("integer") || type == QStringLiteral("float")
        || type == QStringLiteral("number_literal") || type == QStringLiteral("float_literal")) {
        addNodeSpan(node, TokenKind::Number);
        return;
    }

    if (m_language == Language::Python) {
        if (type == QStringLiteral("decorator")) {
            addNodeSpan(node, TokenKind::Decorator);
        } else if (type == QStringLiteral("function_definition")) {
            TSNode nameNode = ts_node_child_by_field_name(node, "name", 4);
            if (!ts_node_is_null(nameNode)) {
                addNodeSpan(nameNode, TokenKind::FunctionName);
            }
        } else if (type == QStringLiteral("class_definition")) {
            TSNode nameNode = ts_node_child_by_field_name(node, "name", 4);
            if (!ts_node_is_null(nameNode)) {
                addNodeSpan(nameNode, TokenKind::ClassName);
            }
        }
        return;
    }

    if (type == QStringLiteral("primitive_type") || type == QStringLiteral("primitive_type_identifier")) {
        addNodeSpan(node, TokenKind::BuiltinType);
    }
    maybeMarkFunctionName(node, type);
    maybeMarkClassLikeName(node, type);
}

void TreeSitterSyntaxHighlighter::maybeMarkFunctionName(TSNode node, const QString& type) {
    if (m_language == Language::Rust) {
        if (type != QStringLiteral("function_item")) {
            return;
        }
        TSNode nameNode = ts_node_child_by_field_name(node, "name", 4);
        if (!ts_node_is_null(nameNode)) {
            addNodeSpan(nameNode, TokenKind::FunctionName);
        }
        return;
    }

    if (type == QStringLiteral("function_definition")) {
        TSNode declarator = ts_node_child_by_field_name(node, "declarator", 10);
        if (ts_node_is_null(declarator)) {
            return;
        }

        TSNode identifier = findFirstNamedDescendantByType(declarator, QStringLiteral("identifier"));
        if (!ts_node_is_null(identifier)) {
            addNodeSpan(identifier, TokenKind::FunctionName);
        }
    }
}

void TreeSitterSyntaxHighlighter::maybeMarkClassLikeName(TSNode node, const QString& type) {
    if (m_language == Language::Rust) {
        if (type == QStringLiteral("struct_item") || type == QStringLiteral("enum_item")
            || type == QStringLiteral("trait_item") || type == QStringLiteral("type_item")) {
            TSNode nameNode = ts_node_child_by_field_name(node, "name", 4);
            if (!ts_node_is_null(nameNode)) {
                addNodeSpan(nameNode, TokenKind::ClassName);
            }
        }
        return;
    }

    if (type == QStringLiteral("class_specifier") || type == QStringLiteral("struct_specifier")
        || type == QStringLiteral("union_specifier") || type == QStringLiteral("enum_specifier")) {
        TSNode nameNode = ts_node_child_by_field_name(node, "name", 4);
        if (!ts_node_is_null(nameNode)) {
            addNodeSpan(nameNode, TokenKind::ClassName);
        }
    }
}

TSNode TreeSitterSyntaxHighlighter::findFirstNamedDescendantByType(TSNode node, const QString& targetType) const {
    if (ts_node_is_null(node)) {
        TSNode nullNode{};
        return nullNode;
    }
    const QString nodeType = QString::fromUtf8(ts_node_type(node));
    if (nodeType == targetType) {
        return node;
    }
    const uint32_t count = ts_node_named_child_count(node);
    for (uint32_t i = 0; i < count; ++i) {
        TSNode child = ts_node_named_child(node, i);
        TSNode found = findFirstNamedDescendantByType(child, targetType);
        if (!ts_node_is_null(found)) {
            return found;
        }
    }
    TSNode nullNode{};
    return nullNode;
}

void TreeSitterSyntaxHighlighter::addNodeSpan(TSNode node, TokenKind kind) {
    const TSPoint start = ts_node_start_point(node);
    const TSPoint end = ts_node_end_point(node);
    const int startRow = static_cast<int>(start.row);
    const int endRow = static_cast<int>(end.row);

    if (startRow == endRow) {
        addLineSpan(startRow, static_cast<int>(start.column), static_cast<int>(end.column), kind);
        return;
    }

    for (int row = startRow; row <= endRow; ++row) {
        const int startCol = (row == startRow) ? static_cast<int>(start.column) : 0;
        const int endCol = (row == endRow) ? static_cast<int>(end.column) : (1 << 30);
        addLineSpan(row, startCol, endCol, kind);
    }
}

void TreeSitterSyntaxHighlighter::addLineSpan(int row, int startByteCol, int endByteCol, TokenKind kind) {
    if (row < 0 || endByteCol <= startByteCol) {
        return;
    }
    auto& spans = m_spansByLine[row];
    spans.push_back(ByteSpan{startByteCol, endByteCol, kind});
}

void TreeSitterSyntaxHighlighter::applyRegexKeywords(const QString& text) {
    for (const auto& pattern : m_keywordPatterns) {
        auto it = pattern.globalMatch(text);
        while (it.hasNext()) {
            const auto match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), m_keywordFormat);
        }
    }
    for (const auto& pattern : m_builtinPatterns) {
        auto it = pattern.globalMatch(text);
        while (it.hasNext()) {
            const auto match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), m_typeFormat);
        }
    }
    auto numberIt = m_numberPattern.globalMatch(text);
    while (numberIt.hasNext()) {
        const auto match = numberIt.next();
        setFormat(match.capturedStart(), match.capturedLength(), m_numberFormat);
    }
}

void TreeSitterSyntaxHighlighter::applyTreeSitterSpans(const QString& text, int blockNumber) {
    const auto spans = m_spansByLine.value(blockNumber);
    if (spans.empty()) {
        return;
    }
    const int lineByteLen = text.toUtf8().size();
    for (const auto& span : spans) {
        const int startByte = qBound(0, span.startByteCol, lineByteLen);
        const int endByte = qBound(0, span.endByteCol, lineByteLen);
        if (endByte <= startByte) {
            continue;
        }
        const int startIdx = utf16IndexFromUtf8ByteColumn(text, startByte);
        const int endIdx = utf16IndexFromUtf8ByteColumn(text, endByte);
        const int length = qMax(1, endIdx - startIdx);
        setFormat(startIdx, length, formatForKind(span.kind));
    }
}

int TreeSitterSyntaxHighlighter::utf16IndexFromUtf8ByteColumn(const QString& lineText, int byteColumn) const {
    const QByteArray utf8 = lineText.toUtf8();
    const int clamped = qBound(0, byteColumn, utf8.size());
    return QString::fromUtf8(utf8.constData(), clamped).size();
}

QTextCharFormat TreeSitterSyntaxHighlighter::formatForKind(TokenKind kind) const {
    switch (kind) {
    case TokenKind::Keyword:
        return m_keywordFormat;
    case TokenKind::BuiltinType:
        return m_typeFormat;
    case TokenKind::String:
        return m_stringFormat;
    case TokenKind::Number:
        return m_numberFormat;
    case TokenKind::Comment:
        return m_commentFormat;
    case TokenKind::Decorator:
        return m_decoratorFormat;
    case TokenKind::FunctionName:
        return m_functionFormat;
    case TokenKind::ClassName:
        return m_classFormat;
    }
    return m_keywordFormat;
}

void TreeSitterSyntaxHighlighter::highlightBlock(const QString& text) {
    applyRegexKeywords(text);

    if (!m_treeSitterReady) {
        static const QRegularExpression pyCommentPattern(QStringLiteral("#[^\\n]*"));
        static const QRegularExpression cLikeCommentPattern(QStringLiteral("//[^\\n]*"));
        static const QRegularExpression string1(QStringLiteral("\"[^\"]*\""));
        static const QRegularExpression string2(QStringLiteral("'[^']*'"));
        const QRegularExpression& commentPattern = (m_language == Language::Python)
            ? pyCommentPattern
            : cLikeCommentPattern;
        auto cIt = commentPattern.globalMatch(text);
        while (cIt.hasNext()) {
            auto m = cIt.next();
            setFormat(m.capturedStart(), m.capturedLength(), m_commentFormat);
        }
        auto s1 = string1.globalMatch(text);
        while (s1.hasNext()) {
            auto m = s1.next();
            setFormat(m.capturedStart(), m.capturedLength(), m_stringFormat);
        }
        auto s2 = string2.globalMatch(text);
        while (s2.hasNext()) {
            auto m = s2.next();
            setFormat(m.capturedStart(), m.capturedLength(), m_stringFormat);
        }
        applyDiagnosticsOverlay(text);
        return;
    }

    applyTreeSitterSpans(text, currentBlock().blockNumber());
    applyDiagnosticsOverlay(text);
}

} // namespace ide::ui::highlighting
