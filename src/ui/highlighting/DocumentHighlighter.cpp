#include "ui/highlighting/DocumentHighlighter.hpp"

#include <QQuickTextDocument>
#include <QTextDocument>
#include <QVariantMap>

namespace ide::ui::highlighting {

DocumentHighlighter::DocumentHighlighter(QObject* parent)
    : QObject(parent) {}

QObject* DocumentHighlighter::textDocument() const {
    return m_textDocument;
}

void DocumentHighlighter::setTextDocument(QObject* textDocument) {
    if (textDocument == m_textDocument) {
        return;
    }

    m_textDocument = textDocument;
    rebuildHighlighter();
    emit textDocumentChanged();
}

QString DocumentHighlighter::language() const {
    return m_language;
}

void DocumentHighlighter::setLanguage(const QString& language) {
    if (language == m_language) {
        return;
    }

    m_language = language;
    rebuildHighlighter();
    emit languageChanged();
}

QVariantList DocumentHighlighter::diagnostics() const {
    return m_diagnostics;
}

void DocumentHighlighter::setDiagnostics(const QVariantList& diagnostics) {
    const bool changed = (m_diagnostics != diagnostics);
    if (changed) {
        m_diagnostics = diagnostics;
    }
    if (m_cppHighlighter) {
        std::vector<CppSyntaxHighlighter::DiagnosticRange> ranges;
        ranges.reserve(static_cast<std::size_t>(m_diagnostics.size()));
        for (const QVariant& item : m_diagnostics) {
            const QVariantMap row = item.toMap();
            CppSyntaxHighlighter::DiagnosticRange range;
            range.lineStart = row.value("lineStart").toInt();
            range.columnStart = row.value("columnStart").toInt();
            range.lineEnd = row.value("lineEnd").toInt();
            range.columnEnd = row.value("columnEnd").toInt();
            range.severity = row.value("severity").toString();
            ranges.push_back(std::move(range));
        }
        m_cppHighlighter->setDiagnostics(std::move(ranges));
    }
    if (changed) {
        emit diagnosticsChanged();
    }
}

void DocumentHighlighter::rebuildHighlighter() {
    m_cppHighlighter.reset();

    auto* quickDocument = qobject_cast<QQuickTextDocument*>(m_textDocument);
    if (!quickDocument) {
        return;
    }

    QTextDocument* document = quickDocument->textDocument();
    if (!document) {
        return;
    }

    m_cppHighlighter = std::make_unique<CppSyntaxHighlighter>(document);
    const bool lexicalEnabled = (m_language == "cpp" || m_language == "c" || m_language == "rust");
    m_cppHighlighter->setLexicalHighlightingEnabled(lexicalEnabled);
    setDiagnostics(m_diagnostics);
}

} // namespace ide::ui::highlighting
