#include "ui/highlighting/DocumentHighlighter.hpp"

#include <QQuickTextDocument>
#include <QTextDocument>

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

    if (m_language == "cpp" || m_language == "c" || m_language == "rust") {
        m_cppHighlighter = std::make_unique<CppSyntaxHighlighter>(document);
    }
}

} // namespace ide::ui::highlighting
